/**
 * @file terminal.c
 * @brief Core rendering engine implementation for the portable TUI.
 *
 * This file contains the logic for rendering the 3-zone terminal layout,
 * managing dynamic sidebar registrations, and handling the core UI state.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ansi-esc.h"
#include "terminal.h"

/* --- External Configuration --- */
/** @brief Link to the command registry defined in terminal_cfg.c */
extern command_t command_config[];
/** @brief Total number of available commands */
extern const int command_count;

/** @brief Global drawing lock to prevent UART collisions between logs and sidebar updates. */
volatile bool term_busy = false;

/* --- Dynamic Sidebar Storage --- */
/** @brief Internal storage for active sidebar telemetry items. */
static sidebar_item_t sidebar_dynamic_config[TERM_SIDEBAR_MAX];
/** @brief Current count of registered sidebar items. */
static int sidebar_dynamic_count = 0;

/* --- Internal State Buffers --- */
/** @brief Input buffer for the current command line. */
static char cmd_buffer[TERM_CMD_MAX_LEN] = {0};
/** @brief Current length of the string in the command buffer. */
static int  cmd_idx = 0;
/** @brief Ring buffer for command history. */
static char history[TERM_MAX_HISTORY][TERM_CMD_MAX_LEN] = {0};
/** @brief Current write position in the history ring buffer. */
static int  hist_write_idx = 0;
/** @brief Current browse position in the history (-1 when not browsing). */
static int  hist_view_idx  = -1;
/** @brief Current visual cursor position within the command buffer. */
static int cursor_pos = 0;

/* --- Private Prototypes --- */
static void term_render_cmd_line();
static void term_autocomplete();
static void _handle_execute(void);
static void _handle_history(key_t key);
static void _handle_edit(key_t key);

/**
 * @brief Registers a variable to be displayed in the sidebar at runtime.
 *
 * @param label The string identifier to display.
 * @param ptr Pointer to the data variable.
 * @param type Data type for formatting (INT, FLOAT, STRING).
 * @param unit Physical unit to append (e.g., "g").
 * @return true if successfully registered, false if sidebar is full.
 */
bool term_sidebar_register(const char* label, void* ptr, data_type_t type, const char* unit){
	if (sidebar_dynamic_count >= TERM_SIDEBAR_MAX) return false;

	/* Copy metadata to the dynamic storage slot */
	sidebar_dynamic_config[sidebar_dynamic_count].label = label;
	sidebar_dynamic_config[sidebar_dynamic_count].value_ptr = ptr;
	sidebar_dynamic_config[sidebar_dynamic_count].type = type;
	sidebar_dynamic_config[sidebar_dynamic_count].unit = unit;

	sidebar_dynamic_count++;
	return true;
}

/**
 * @brief Prints a professional startup banner using CMake metadata.
 *
 * Uses the LOG_T macro to ensure correct positioning in the log region.
 */
void term_print_banner(){
	LOG_T("%s=== %s ===%s", ANSI_BOLD, APP_NAME, ANSI_RESET);
	LOG_T("%sVersion: %s%s", ANSI_DIM, APP_VERSION, ANSI_RESET);
	LOG_T("%sAuthor:  %s%s", ANSI_DIM, APP_AUTHOR, ANSI_RESET);
	LOG_T("%sBuild:   %s %s%s", ANSI_DIM, __DATE__, __TIME__, ANSI_RESET);
	LOG_T("Type %shelp%s to list all available commands.", ANSI_YELLOW, ANSI_RESET);
	LOG_T("─────────────────────────────────────────────");
}

/**
 * @brief Initializes the terminal screen, sets scroll regions and draws UI frame.
 *
 * Switches to the alternative buffer and draws the static ASCII borders.
 */
void term_init(){
	/* Enter Alt Buffer, Clear Screen, Move to Home */
	printf(ANSI_BUF_ALT ANSI_CLR ANSI_HOME);
	/* Define top scrolling region for logs (Rows 1 to 35) */
	printf("\033[%d;%dr", TERM_LOG_START, TERM_LOG_END);

	/* Draw vertical separator for the sidebar */
	for(int i=1; i<TERM_CMD_ROW; i++){
		printf("\033[%d;%dH│", i, TERM_SIDEBAR_COL - 2);
	}

	/* Draw horizontal separator above the command line */
	printf("\033[%d;1H", TERM_CMD_ROW);
	for(int i=0; i<TERM_WIDTH; i++){
		/* Add a 'T-junction' character where the vertical line hits the horizontal line */
		if(i == TERM_SIDEBAR_COL -3) printf("┴");
		else printf("─");
	}

	/* Hide hardware cursor during setup to prevent artifacts */
	printf(ANSI_HIDE_CUR);
	term_render_cmd_line();
	fflush(stdout);
	term_print_banner();
}

/**
 * @brief Positions the terminal cursor to the current editing spot in the prompt.
 *
 * Synchronizes the visual cursor with the logical cursor_pos index.
 */
void term_focus_cmd(){
	/* Offset by 11 chars to account for the "COMMAND > " prompt prefix */
	printf("\033[%d;%dH", TERM_CMD_ROW + 1, 12 + cursor_pos);
	/* Enable blinking cursor for the user */
	printf("\033[?25h");
	fflush(stdout);
}

/**
 * @brief Refreshes all registered sidebar telemetry values.
 *
 * Iterates through dynamic_config and prints values using ANSI positioning.
 */
void term_update_sidebar(){
	if (term_busy) return;
	term_busy = true;

	/* Save current cursor position before moving to the sidebar */
	printf("\033[s");

	/* Refresh vertical line and clear potential log overflows to the right */
	for(int i=1; i<TERM_CMD_ROW; i++){
		printf("\033[%d;%dH│", i, TERM_SIDEBAR_COL - 2);
		if(i<sidebar_dynamic_count) printf("\033[K");
	}

	/* Render all active telemetry items */
	for(int i=0; i < sidebar_dynamic_count; i++){
		printf("\033[%d;%dH", i + 2, TERM_SIDEBAR_COL);

		sidebar_item_t item = sidebar_dynamic_config[i];
		printf("%-10s: ", item.label);

		/* Cast and print based on the registered data type */
		if(item.type == TYPE_INT)    printf("%d %s", *(int16_t*)item.value_ptr, item.unit);
		if(item.type == TYPE_FLOAT)  printf("%9.2f %s", *(float*)item.value_ptr, item.unit);
		if(item.type == TYPE_STRING) printf("%s", (char*)item.value_ptr);
	}

	/* Restore cursor position and re-anchor to command line */
	printf("\033[u");
	fflush(stdout);
	term_focus_cmd();
	term_busy = false;
}

/**
 * @brief Removes an item from the sidebar by its label name.
 *
 * @param label The label of the item to remove.
 * @return true if an item was found and removed, false otherwise.
 */
bool term_sidebar_remove(const char* label){
	if (label == NULL) return false;

	for (int i = 0; i < sidebar_dynamic_count; i++){
		if (strcmp(sidebar_dynamic_config[i].label, label) == 0){
			/* Shift subsequent items left to maintain a continuous array */
			for (int j = i; j < sidebar_dynamic_count - 1; j++){
				sidebar_dynamic_config[j] = sidebar_dynamic_config[j + 1];
			}
			sidebar_dynamic_count--;

			/* Erase the now empty last row in the sidebar area */
			printf("\033[%d;%dH\033[K", sidebar_dynamic_count + 2, TERM_SIDEBAR_COL);
			return true;
		}
	}
	return false;
}

/**
 * @brief Moves the cursor to the bottom of the log scrolling region.
 */
void term_scroll_log(){
	printf("\033[%d;2H", TERM_LOG_END);
	fflush(stdout);
}

/**
 * @brief Internal handler to process and execute the current command buffer.
 *
 * Validates command names, manages history storage, and dispatches to functions.
 */
static void _handle_execute(void){
	if (cmd_idx == 0) return;

	/* Create a copy for tokenization so the history remains intact */
	char temp_buf[TERM_CMD_MAX_LEN];
	strncpy(temp_buf, cmd_buffer, TERM_CMD_MAX_LEN - 1);
	char *cmd_name = strtok(temp_buf, " ");

	int found_idx = -1;
	/* Search the registry for a matching command name */
	for (int i = 0; i < command_count; i++){
		if (strcmp(cmd_name, command_config[i].name) == 0){
			found_idx = i;
			break;
		}
	}

	if (found_idx != -1){
		/* Save to history if the command is valid and not a duplicate of the last entry */
		int last = (hist_write_idx + TERM_MAX_HISTORY - 1) % TERM_MAX_HISTORY;
		if (strcmp(cmd_buffer, history[last]) != 0){
			strncpy(history[hist_write_idx], cmd_buffer, TERM_CMD_MAX_LEN - 1);
			hist_write_idx = (hist_write_idx + 1) % TERM_MAX_HISTORY;
		}

		/* Provide visual feedback for execution using the terminal log macro */
		LOG_T(" ");
		LOG_T(">> %s%s%s...", ANSI_ITALIC, cmd_buffer, ANSI_RESET);

		/* Split command from arguments in the buffer */
		strtok(cmd_buffer, " ");
		char *args = strtok(NULL, "");
		/* Dispatch to the registered command function */
		command_config[found_idx].func(args);
	} else {
		/* Report error for unrecognized commands */
		LOG_E("Command: \"%s\" unknown!", cmd_name);
	}

	/* Reset command line state for next input */
	hist_view_idx = -1;
	memset(cmd_buffer, 0, sizeof(cmd_buffer));
	cmd_idx = 0;
	cursor_pos = 0;
}

/**
 * @brief Browses through stored command history using a ring buffer.
 *
 * Maps UP/DOWN keys to historical command entries.
 * @param key The key pressed (KEY_UP or KEY_DOWN).
 */
static void _handle_history(key_t key){
	if (key == KEY_UP){
		/* Navigate back into the past */
		int next = (hist_view_idx == -1) ? (hist_write_idx + TERM_MAX_HISTORY - 1) % TERM_MAX_HISTORY : (hist_view_idx + TERM_MAX_HISTORY - 1) % TERM_MAX_HISTORY;
		if (strlen(history[next]) > 0) hist_view_idx = next;
	} else {
		/* Navigate forward towards the present */
		if (hist_view_idx != -1){
			hist_view_idx = (hist_view_idx + 1) % TERM_MAX_HISTORY;
			/* Reset to empty prompt if we reach the write index or an empty slot */
			if (hist_view_idx == hist_write_idx || strlen(history[hist_view_idx]) == 0) hist_view_idx = -1;
		}
	}

	/* Copy historical command to buffer or clear if no entry selected */
	if (hist_view_idx != -1) strncpy(cmd_buffer, history[hist_view_idx], TERM_CMD_MAX_LEN - 1);
	else memset(cmd_buffer, 0, sizeof(cmd_buffer));

	/* Sync indexes and move cursor to the end of the recalled command */
	cmd_idx = (int)strlen(cmd_buffer);
	cursor_pos = cmd_idx;
}

/**
 * @brief Manages character insertion and deletions at the current cursor position.
 *
 * Performs buffer shifting to allow in-line editing.
 * @param key The key pressed (Backspace, Delete, or Alphanumeric).
 */
static void _handle_edit(key_t key){
	/* BACKSPACE: Delete character to the left of the cursor */
	if (key == KEY_BACKSPACE && cursor_pos > 0){
		/* Shift characters left to fill the gap */
		for (int i = cursor_pos - 1; i < cmd_idx; i++) cmd_buffer[i] = cmd_buffer[i + 1];
		cmd_idx--; cursor_pos--;
		cmd_buffer[cmd_idx] = '\0';
	}
	/* DELETE: Delete character directly at the cursor position */
	else if (key == KEY_DELETE && cursor_pos < cmd_idx){
		/* Shift following characters left */
		for (int i = cursor_pos; i < cmd_idx; i++) cmd_buffer[i] = cmd_buffer[i + 1];
		cmd_idx--;
		cmd_buffer[cmd_idx] = '\0';
	}
	/* CHARACTER INPUT: Insert printable ASCII characters */
	else if (key >= 32 && key <= 126 && cmd_idx < TERM_CMD_MAX_LEN - 2){
		/* Shift subsequent characters right to make space for the new character */
		for (int i = cmd_idx; i > cursor_pos; i--) cmd_buffer[i] = cmd_buffer[i - 1];
		cmd_buffer[cursor_pos] = (char)key;
		cmd_idx++; cursor_pos++;
		cmd_buffer[cmd_idx] = '\0';
	}
}

/**
 * @brief Main logic router for keyboard processing.
 *
 * Maps logical keys to specific handler functions (history, editing, execution).
 * @param key The key code received from the hardware driver.
 */
void term_handle_input(key_t key){
	if (key == KEY_NONE) return;

	switch (key){
		case KEY_ENTER:     _handle_execute(); break;
		case KEY_UP:
		case KEY_DOWN:      _handle_history(key); break;
		case KEY_LEFT:      if (cursor_pos > 0) cursor_pos--; break;
		case KEY_RIGHT:     if (cursor_pos < cmd_idx) cursor_pos++; break;
		case KEY_TAB:       term_autocomplete(); cursor_pos = cmd_idx; break;
		case KEY_HOME:      cursor_pos = 0; break;
		case KEY_END:       cursor_pos = cmd_idx; break;

				    /* All other printable or editing keys go to the buffer editor */
		case KEY_BACKSPACE:
		case KEY_DELETE:
		default:            _handle_edit(key); break;
	}

	/* Redraw the updated command line */
	term_render_cmd_line();
}

/**
 * @brief Renders the entire command line prompt and manages hardware cursor visibility.
 */
static void term_render_cmd_line(){
	printf("\033[s"); /* Save current terminal cursor (e.g., in sidebar) */

	/* Jump to prompt row, clear it, and print current buffer content */
	printf("\033[%d;2H" ANSI_CLR_LINE, TERM_CMD_ROW + 1);
	printf(ANSI_BOLD "COMMAND" ANSI_RESET " > ");

	fwrite(cmd_buffer, 1, cmd_idx, stdout);
	/* Precisely position the hardware cursor to the logical edit spot */
	/* Account for prompt string length "COMMAND > " (11 chars) */
	printf("\033[%d;%dH", TERM_CMD_ROW + 1, 12 + cursor_pos);

	/* Re-enable blinking cursor for input visibility */
	printf("\033[?25h");

	fflush(stdout);
}

/**
 * @brief Logic to extract value strings from key=value argument pairs.
 *
 * @param args The raw argument string.
 * @param key The key to search for.
 * @return Pointer to the value string, or NULL if not found.
 */
const char* term_get_arg(const char* args, const char* key){
	if (!args || !key) return NULL;
	const char* p = strstr(args, key);
	while (p){
		size_t klen = strlen(key);
		/* Ensure key is preceded by space/start and followed by '=' */
		if ((p == args || *(p - 1) == ' ') && *(p + klen) == '=') return p + klen + 1;
		p = strstr(p + 1, key);
	}
	return NULL;
}

/**
 * @brief Performs command completion or lists possible matches.
 *
 * Triggered by the TAB key. Completes unique matches or lists multiple options.
 */
static void term_autocomplete(){
	if (cmd_idx == 0) return;
	int matches = 0, last = -1;
	/* Count matches in the command configuration */
	for (int i = 0; i < command_count; i++){
		if (strncmp(cmd_buffer, command_config[i].name, cmd_idx) == 0){
			matches++; last = i;
		}
	}
	if (matches == 1){
		/* Exactly one match: perform full completion */
		strncpy(cmd_buffer, command_config[last].name, TERM_CMD_MAX_LEN - 1);
		cmd_idx = (int)strlen(cmd_buffer);
		/* Append a convenience space if buffer allows */
		if (cmd_idx < TERM_CMD_MAX_LEN - 1){ cmd_buffer[cmd_idx++] = ' '; cmd_buffer[cmd_idx] = '\0'; }
	} else if (matches > 1){
		/* Multiple matches: list all commands starting with the current input */
		LOG_T("%sMatches: %s", ANSI_CYAN, ANSI_RESET);
		for (int i = 0; i < command_count; i++){
			if (strncmp(cmd_buffer, command_config[i].name, cmd_idx) == 0) printf("%s%s  %s", ANSI_YELLOW, command_config[i].name, ANSI_RESET);
		}
		printf("\n");
	}
}

/**
 * @brief Generates an automated help list or detailed command information.
 *
 * @param args If NULL, lists all commands. If command name, show usage/details.
 */
void cmd_help_auto(const char* args){
	if (!args || strlen(args) == 0){
		/* General help listing */
		LOG_T("%s--- AVAILABLE COMMANDS ---%s", ANSI_BOLD ANSI_CYAN, ANSI_RESET);
		for (int i = 0; i < command_count; i++) LOG_T("%s %-10s %s%s-> %s%s", ANSI_YELLOW, command_config[i].name, ANSI_RESET, ANSI_DIM, command_config[i].help, ANSI_RESET);
	} else {
		/* Detail view for a specific command */
		for (int i = 0; i < command_count; i++){
			if (strcmp(args, command_config[i].name) == 0){
				LOG_T("%sHELP: %s%s", ANSI_BOLD ANSI_YELLOW, command_config[i].name, ANSI_RESET);
				LOG_T("Description: %s", command_config[i].help);
				if (command_config[i].usage) LOG_T("Usage:       %s%s %s%s", ANSI_CYAN, command_config[i].name, command_config[i].usage, ANSI_RESET);
				return;
			}
		}
		LOG_E("Help: Command '%s' not found.", args);
	}
}
