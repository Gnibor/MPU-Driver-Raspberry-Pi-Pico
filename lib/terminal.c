/**
 * @file terminal.c
 * @brief Core rendering engine for the portable TUI.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ansi-esc.h"
#include "terminal.h"

/* --- External Configuration --- */
extern command_t command_config[];
extern const int command_count;

/** @brief Global drawing lock to prevent UART collisions. */
volatile bool term_busy = false;

/* --- Dynamic Sidebar Storage --- */
static sidebar_item_t sidebar_dynamic_config[TERM_SIDEBAR_MAX];
static int sidebar_dynamic_count = 0;

/* --- Internal State Buffers --- */
static char cmd_buffer[TERM_CMD_MAX_LEN] = {0};
static int  cmd_idx = 0;
static char history[TERM_MAX_HISTORY][TERM_CMD_MAX_LEN] = {0};
static int  hist_write_idx = 0;
static int  hist_view_idx  = -1;
static int cursor_pos = 0; /**< Current visual cursor position in the buffer. */
/* --- Private Prototypes --- */
static void term_render_cmd_line();
static void term_autocomplete();
static void _handle_execute(void);
static void _handle_history(key_t key);
static void _handle_edit(key_t key);

/**
 * @brief Registers a variable to be displayed in the sidebar at runtime.
 */
bool term_sidebar_register(const char* label, void* ptr, data_type_t type, const char* unit) {
	if (sidebar_dynamic_count >= TERM_SIDEBAR_MAX) return false;
	sidebar_dynamic_config[sidebar_dynamic_count].label = label;
	sidebar_dynamic_config[sidebar_dynamic_count].value_ptr = ptr;
	sidebar_dynamic_config[sidebar_dynamic_count].type = type;
	sidebar_dynamic_config[sidebar_dynamic_count].unit = unit;
	sidebar_dynamic_count++;
	return true;
}

/**
 * @brief Prints a professional startup banner using CMake metadata.
 */
void term_print_banner() {
	LOG_T("%s=== %s ===%s", ANSI_BOLD, APP_NAME, ANSI_RESET);
	LOG_T("%sVersion: %s%s", ANSI_DIM, APP_VERSION, ANSI_RESET);
	LOG_T("%sAuthor:  %s%s", ANSI_DIM, APP_AUTHOR, ANSI_RESET);
	LOG_T("%sBuild:   %s %s%s", ANSI_DIM, __DATE__, __TIME__, ANSI_RESET);
	LOG_T("Type %shelp%s to list all available commands.", ANSI_YELLOW, ANSI_RESET);
	LOG_T("─────────────────────────────────────────────");
}

/**
 * @brief Initializes the terminal screen, sets scroll regions and draws UI frame.
 */
void term_init() {
	printf(ANSI_BUF_ALT ANSI_CLR ANSI_HOME);
	printf("\033[%d;%dr", TERM_LOG_START, TERM_LOG_END);

	/* Draw static UI elements */
	for(int i=1; i<TERM_CMD_ROW; i++) {
		printf("\033[%d;%dH│", i, TERM_SIDEBAR_COL - 2);
	}
	printf("\033[%d;1H", TERM_CMD_ROW);
	for(int i=0; i<TERM_WIDTH; i++){
		if(i == TERM_SIDEBAR_COL -3) printf("┴");
		else printf("─");
	}

	printf(ANSI_HIDE_CUR);
	term_render_cmd_line();
	fflush(stdout);
	term_print_banner();
}

/** @brief Beams the cursor back to the current editing position in the command line. */
void term_focus_cmd() {
	// "COMMAND > " (11 chars) + cursor_pos
	printf("\033[%d;%dH", TERM_CMD_ROW + 1, 11 + cursor_pos);
	printf("\033[?25h"); // Cursor explizit einschalten
	fflush(stdout);
}

/**
 * @brief Refreshes the dynamic sidebar values.
 */
void term_update_sidebar() {
	if (term_busy) return;
	term_busy = true;

	printf("\033[s"); /* Save cursor */

	/* Redraw vertical separator in case of log overflows */
	for(int i=1; i<TERM_CMD_ROW; i++){
		printf("\033[%d;%dH│", i, TERM_SIDEBAR_COL - 2);
		printf("\033[K"); /* Clear line to the right */
	}
	for(int i=0; i < sidebar_dynamic_count; i++) {
		printf("\033[%d;%dH", i + 2, TERM_SIDEBAR_COL);

		sidebar_item_t item = sidebar_dynamic_config[i];
		printf("%-10s: ", item.label);

		if(item.type == TYPE_INT)    printf("%d %s", *(int16_t*)item.value_ptr, item.unit);
		if(item.type == TYPE_FLOAT)  printf("%9.2f %s", *(float*)item.value_ptr, item.unit);
		if(item.type == TYPE_STRING) printf("%s", (char*)item.value_ptr);
	}

	printf("\033[u"); /* Restore cursor */
	fflush(stdout);
	term_focus_cmd();
	term_busy = false;
}

/**
 * @brief Removes an item from the sidebar by its label name.
 * @return true if found and removed, false otherwise.
 */
bool term_sidebar_remove(const char* label) {
	if (label == NULL) return false;

	for (int i = 0; i < sidebar_dynamic_count; i++) {
		if (strcmp(sidebar_dynamic_config[i].label, label) == 0) {
			/* Shift remaining items to the left to close the gap */
			for (int j = i; j < sidebar_dynamic_count - 1; j++) {
				sidebar_dynamic_config[j] = sidebar_dynamic_config[j + 1];
			}
			sidebar_dynamic_count--;

			/* Clear the last row on the terminal to avoid ghosting */
			printf("\033[%d;%dH\033[K", sidebar_dynamic_count + 2, TERM_SIDEBAR_COL);
			return true;
		}
	}
	return false;
}

/**
 * @brief Prepare for log scroll.
 */
void term_scroll_log() {
	printf("\033[%d;1H", TERM_LOG_END);
	fflush(stdout);
}

/**
 * @brief Processes the current buffer: validates, executes, and saves to history.
 */
static void _handle_execute(void) {
	if (cmd_idx == 0) return;

	/* Temporary buffer for validation to keep original for history */
	char temp_buf[TERM_CMD_MAX_LEN];
	strncpy(temp_buf, cmd_buffer, TERM_CMD_MAX_LEN - 1);
	char *cmd_name = strtok(temp_buf, " ");

	int found_idx = -1;
	for (int i = 0; i < command_count; i++) {
		if (strcmp(cmd_name, command_config[i].name) == 0) {
			found_idx = i;
			break;
		}
	}

	if (found_idx != -1) {
		/* Save to history if valid and not a duplicate of the last entry */
		int last = (hist_write_idx + TERM_MAX_HISTORY - 1) % TERM_MAX_HISTORY;
		if (strcmp(cmd_buffer, history[last]) != 0) {
			strncpy(history[hist_write_idx], cmd_buffer, TERM_CMD_MAX_LEN - 1);
			hist_write_idx = (hist_write_idx + 1) % TERM_MAX_HISTORY;
		}

		LOG_T("Execute: %s%s%s...", ANSI_ITALIC, cmd_buffer, ANSI_RESET);

		/* Tokenize real buffer and pass args to function */
		strtok(cmd_buffer, " "); 
		char *args = strtok(NULL, "");
		command_config[found_idx].func(args);
	} else {
		LOG_E("Command: \"%s\" unknown!", cmd_name);
	}

	/* Reset command line state */
	hist_view_idx = -1;
	memset(cmd_buffer, 0, sizeof(cmd_buffer));
	cmd_idx = 0;
	cursor_pos = 0;
}

/**
 * @brief Browses through stored command history.
 */
static void _handle_history(key_t key) {
	if (key == KEY_UP) {
		int next = (hist_view_idx == -1) ? (hist_write_idx + TERM_MAX_HISTORY - 1) % TERM_MAX_HISTORY : (hist_view_idx + TERM_MAX_HISTORY - 1) % TERM_MAX_HISTORY;
		if (strlen(history[next]) > 0) hist_view_idx = next;
	} else {
		if (hist_view_idx != -1) {
			hist_view_idx = (hist_view_idx + 1) % TERM_MAX_HISTORY;
			if (hist_view_idx == hist_write_idx || strlen(history[hist_view_idx]) == 0) hist_view_idx = -1;
		}
	}

	if (hist_view_idx != -1) strncpy(cmd_buffer, history[hist_view_idx], TERM_CMD_MAX_LEN - 1);
	else memset(cmd_buffer, 0, sizeof(cmd_buffer));

	cmd_idx = (int)strlen(cmd_buffer);
	cursor_pos = cmd_idx; // Jump to end of string when recalling history
}

/**
 * @brief Manages character insertion and deletions at the current cursor position.
 */
static void _handle_edit(key_t key) {
	if (key == KEY_BACKSPACE && cursor_pos > 0) {
		/* Shift chars left to overwrite the char before cursor */
		for (int i = cursor_pos - 1; i < cmd_idx; i++) cmd_buffer[i] = cmd_buffer[i + 1];
		cmd_idx--; cursor_pos--;
		cmd_buffer[cmd_idx] = '\0';
	} 
	else if (key == KEY_DELETE && cursor_pos < cmd_idx) {
		/* Shift chars left to overwrite the char at cursor */
		for (int i = cursor_pos; i < cmd_idx; i++) cmd_buffer[i] = cmd_buffer[i + 1];
		cmd_idx--;
		cmd_buffer[cmd_idx] = '\0';
	} 
	else if (key >= 32 && key <= 126 && cmd_idx < TERM_CMD_MAX_LEN - 1) {
		/* Shift chars right to make space for new character */
		for (int i = cmd_idx; i > cursor_pos; i--) cmd_buffer[i] = cmd_buffer[i - 1];
		cmd_buffer[cursor_pos] = (char)key;
		cmd_idx++; cursor_pos++;
		cmd_buffer[cmd_idx] = '\0';
	}
}

/**
 * @brief Main entry point for keyboard processing.
 */
void term_handle_input(key_t key) {
	if (key == KEY_NONE) return;

	switch (key) {
		case KEY_ENTER:     _handle_execute(); break;
		case KEY_UP:
		case KEY_DOWN:      _handle_history(key); break;
		case KEY_LEFT:      if (cursor_pos > 0) cursor_pos--; break;
		case KEY_RIGHT:     if (cursor_pos < cmd_idx) cursor_pos++; break;
		case KEY_TAB:       term_autocomplete(); cursor_pos = cmd_idx; break;
		case KEY_HOME:      cursor_pos = 0; break;
		case KEY_END:       cursor_pos = cmd_idx; break;

				    /* Direct edit keys and alphanumeric input */
		case KEY_BACKSPACE:
		case KEY_DELETE:
		default:            _handle_edit(key); break;
	}

	term_render_cmd_line();
}

static void term_render_cmd_line() {
	printf("\033[s"); /* Save UI cursor */

	/* Clear line and print prompt + buffer */
	printf("\033[%d;1H" ANSI_CLR_LINE, TERM_CMD_ROW + 1);
	printf(ANSI_BOLD "COMMAND" ANSI_RESET " > %s", cmd_buffer);

	/* Move cursor exactly to the editing position */
	/* "COMMAND > " is 10 chars, so pos is 11 + cursor_pos */
	printf("\033[%d;%dH", TERM_CMD_ROW + 1, 11 + cursor_pos);

	/* Show the cursor while typing */
	printf("\033[?25h"); 

	fflush(stdout);
}

const char* term_get_arg(const char* args, const char* key) {
	if (!args || !key) return NULL;
	const char* p = strstr(args, key);
	while (p) {
		size_t klen = strlen(key);
		if ((p == args || *(p - 1) == ' ') && *(p + klen) == '=') return p + klen + 1;
		p = strstr(p + 1, key);
	}
	return NULL;
}

static void term_autocomplete() {
	if (cmd_idx == 0) return;
	int matches = 0, last = -1;
	for (int i = 0; i < command_count; i++) {
		if (strncmp(cmd_buffer, command_config[i].name, cmd_idx) == 0) {
			matches++; last = i;
		}
	}
	if (matches == 1) {
		strncpy(cmd_buffer, command_config[last].name, TERM_CMD_MAX_LEN - 1);
		cmd_idx = (int)strlen(cmd_buffer);
		if (cmd_idx < TERM_CMD_MAX_LEN - 1) { cmd_buffer[cmd_idx++] = ' '; cmd_buffer[cmd_idx] = '\0'; }
	} else if (matches > 1) {
		LOG_T("%sMatches: %s", ANSI_CYAN, ANSI_RESET);
		for (int i = 0; i < command_count; i++) {
			if (strncmp(cmd_buffer, command_config[i].name, cmd_idx) == 0) printf("%s%s  %s", ANSI_YELLOW, command_config[i].name, ANSI_RESET);
		}
		printf("\n");
	}
}

void cmd_help_auto(const char* args) {
	if (!args || strlen(args) == 0) {
		LOG_T("%s--- AVAILABLE COMMANDS ---%s", ANSI_BOLD ANSI_CYAN, ANSI_RESET);
		for (int i = 0; i < command_count; i++) LOG_T("%s %-10s %s%s-> %s%s", ANSI_YELLOW, command_config[i].name, ANSI_RESET, ANSI_DIM, command_config[i].help, ANSI_RESET);
	} else {
		for (int i = 0; i < command_count; i++) {
			if (strcmp(args, command_config[i].name) == 0) {
				LOG_T("%sHELP: %s%s", ANSI_BOLD ANSI_YELLOW, command_config[i].name, ANSI_RESET);
				LOG_T("Description: %s", command_config[i].help);
				if (command_config[i].usage) LOG_T("Usage:       %s%s %s%s", ANSI_CYAN, command_config[i].name, command_config[i].usage, ANSI_RESET);
				return;
			}
		}
		LOG_E("Help: Command '%s' not found.", args);
	}
}
