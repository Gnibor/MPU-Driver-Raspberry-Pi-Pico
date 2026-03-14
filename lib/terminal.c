/**
 * @file terminal.c
 * @brief The rendering engine for the portable terminal.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "terminal.h"
#include "ansi-esc.h"
#include "rp_pico.h"

extern sidebar_item_t sidebar_config[];
extern const int sidebar_count;
extern command_t command_config[];
extern const int command_count;
volatile bool term_busy;

static char cmd_buffer[64] = {0};
static int  cmd_idx = 0;
static char history_buffer[64] = {0}; // Speichert den letzten erfolgreichen Befehl
static char last_cmd[64] = {0};       // Hilfspuffer für die Fehlermeldung

void term_render_cmd_line();


void term_init() {
	printf(ANSI_BUF_ALT ANSI_CLR ANSI_HOME);
	printf("\033[%d;%dr", TERM_LOG_START, TERM_LOG_END);
	// Vertical Line
	for(int i=1; i<TERM_CMD_ROW; i++) printf("\033[%d;%dH|", i, TERM_SIDEBAR_COL - 2);
	// Horizontal Line
	printf("\033[%d;1H", TERM_CMD_ROW);
	for(int i=0; i<TERM_WIDTH; i++) printf("-");
	// Gehe zur Command-Zeile (z.B. 27) und schreibe den Prompt
	printf("\033[%d;1H" ANSI_BOLD "COMMAND" ANSI_RESET " > ", TERM_CMD_ROW+1);
	printf(ANSI_HIDE_CUR);
	fflush(stdout);
}

void term_update_sidebar() {
	while(term_busy) { tight_loop_contents(); }
	term_busy=true;
	printf("\033[s"); // Save cursor
	for(int i=0; i < sidebar_count; i++) {
		printf("\033[%d;%dH%-5s", i + 2, TERM_SIDEBAR_COL, sidebar_config[i].label);
		sidebar_item_t item = sidebar_config[i];
		if(item.type == TYPE_INT)    printf("%d %s  ", *(int16_t*)item.value_ptr, item.unit);
		if(item.type == TYPE_FLOAT)  printf("%9.2f %s  ", *(float*)item.value_ptr, item.unit);
		if(item.type == TYPE_STRING) printf("%s  ", (char*)item.value_ptr);
		printf("\033[K"); // Clear rest of line
	}
	for(int i=1; i<TERM_CMD_ROW; i++){printf("\033[%d;%dH|", i, TERM_SIDEBAR_COL - 2);}
	printf("\033[u"); // Restore cursor
	fflush(stdout);
	term_busy=false;
}

void term_scroll_log() {
	// 1. Speichere wo wir gerade sind (z.B. Sidebar oder Cmd-Line)
	//printf("\033[s"); 

	// 2. Gehe in die LETZTE Zeile der Log-Region (z.B. 25)
	printf("\033[%d;1H", TERM_LOG_END);

	// 4. Springe wieder in Zeile 25 (da der Cursor jetzt in 26 stünde)
	printf("\033[%d;1H", TERM_LOG_END);

	// 5. Zurück zur alten Position
	//printf("\033[u");
	fflush(stdout);
}

void term_handle_input(key_t key) {
	if(key == KEY_NONE) return;

	if(key == KEY_ENTER) {
		bool found = false;

		strncpy(last_cmd, cmd_buffer, sizeof(last_cmd));
		// Simple Command Parser
		for(int i=0; i<command_count; i++) {
			if(strcmp(cmd_buffer, command_config[i].name) == 0) {
				strncpy(history_buffer, cmd_buffer, sizeof(history_buffer));
				LOG_I("");
				command_config[i].func(NULL);
				found = true;
				break;
			}
		}
		if (!found) {
			LOG_E("Command: \"%s\" does not exist!", last_cmd);
		}
		memset(cmd_buffer, 0, sizeof(cmd_buffer));
		cmd_idx = 0;
	}else if (key == KEY_UP) { // Pfeiltaste Hoch (Pico SDK / ANSI Code Check)
		if (strlen(history_buffer) > 0) {
			strncpy(cmd_buffer, history_buffer, sizeof(cmd_buffer));
			cmd_idx = strlen(cmd_buffer);
		}
	}else if (key >= 32 && key <= 126 && cmd_idx < 63) {
		cmd_buffer[cmd_idx++] = (char)key;
	}

	printf("\033[s\033[%d;1H" ANSI_CLR_LINE "COMMAND > %s\033[u", TERM_CMD_ROW+1, cmd_buffer);
	fflush(stdout);
}

void cmd_help_auto(const char* args) {
	LOG_I(ANSI_BOLD ANSI_CYAN "--- AVAILABLE COMMANDS ---" ANSI_RESET);
	for (int i = 0; i < command_count; i++) {
		// Syntax-Highlighting: Befehl in Gelb, Hilfe in Dim/Weiß
		LOG_I(ANSI_YELLOW " %-10s " ANSI_RESET ANSI_DIM "-> %s" ANSI_RESET, 
				command_config[i].name, 
				command_config[i].help);
	}
	LOG_I(ANSI_BOLD ANSI_CYAN "--------------------------" ANSI_RESET);
}

