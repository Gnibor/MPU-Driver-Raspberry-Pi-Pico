/**
 * @file terminal_cfg.c
 * @brief Project-specific configuration and command implementations for the TUI.
 *
 * This file acts as the glue logic between the MPU60X0 hardware driver and the
 * terminal engine. It defines the available telemetry variables and shell commands.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ansi-esc.h"
#include "mpu60x0.h"
#include "terminal.h"

/** @brief External reference to the automatic help generator in terminal.c. */
extern void cmd_help_auto(const char* args);

/* --- Watch Dictionary: All variables that CAN be added to the sidebar --- */
/**
 * @brief Lookup table for variables available for sidebar registration.
 * Pointers are initialized to NULL and linked at runtime in term_cfg_init().
 */
static watchable_t watch_dictionary[] = {
	{"acc_x", NULL, TYPE_FLOAT, "g"},
	{"acc_y", NULL, TYPE_FLOAT, "g"},
	{"acc_z", NULL, TYPE_FLOAT, "g"},
	{"gyro_x",NULL, TYPE_FLOAT, "°/s"},
	{"gyro_y",NULL, TYPE_FLOAT, "°/s"},
	{"gyro_z",NULL, TYPE_FLOAT, "°/s"},
	{"temp",  NULL, TYPE_FLOAT, "C"}
};
/** @brief Total number of entries in the watchable dictionary. */
#define WATCH_DICT_COUNT (sizeof(watch_dictionary) / sizeof(watchable_t))

/* --- Command Implementations --- */

/** @brief Command: status -> Displays the current online status of the MPU6050. */
void cmd_mpu_status(const char* a){
	(void)a;
	LOG_T("MPU6050 is %sONLINE%s", ANSI_GREEN, ANSI_RESET);
}

/**
 * @brief Command: calib [x/y/z] [samples]
 * Logic: Iterates through arguments to find an axis and/or a numeric sample count.
 */
void cmd_mpu_calibrate(const char* a){
	uint8_t samples = 50;           // Default fallback
	mpu_sensor_t axis = MPU_ACCEL; // Default fallback (Gravity on Z)

	if (a != NULL) {
		char buf[TERM_CMD_MAX_LEN];
		strncpy(buf, a, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0'; // Safety null-termination

		char *token = strtok(buf, " ");
		while (token != NULL) {
			/* 1. Check for Axis Identifier */
			if (strcmp(token, "x") == 0)      axis = MPU_ACCEL_X;
			else if (strcmp(token, "y") == 0) axis = MPU_ACCEL_Y;
			else if (strcmp(token, "z") == 0) axis = MPU_ACCEL_Z;

			/* 2. Check if the first character is a digit (indicates a number) */
			else if (token[0] >= '0' && token[0] <= '9') {
				samples = (uint8_t)atoi(token);
			}

			token = strtok(NULL, " ");
		}
	}

	/* Formatting for the feedback log */
	char axis_label = (axis == MPU_ACCEL_X) ? 'X' : (axis == MPU_ACCEL_Y ? 'Y' : 'Z');

	if(axis == MPU_ACCEL) LOG_T("Calibrating offsets %sWITHOUT%s gravity compensation...", ANSI_CYAN, ANSI_RESET);
	else LOG_T("Calibrating offsets with %s1g Gravity on %c-Axis%s (%u samples)...",
			ANSI_YELLOW, axis_label, ANSI_RESET, samples);

	/* Call the driver with selected axis and samples */
	mpu_calibrate((axis | MPU_GYRO), samples);

	LOG_T("Calibration %sfinished%s.", ANSI_GREEN, ANSI_RESET);
}

/** @brief Command: reset -> Performs a full hardware reset and recovery of the sensor. */
void cmd_reset(const char* a){
	(void)a;
	term_busy = true; // Lock UI updates during reset procedure

	LOG_W("Resetting MPU...");
	sleep_ms(200); // Wait for pending I2C operations to clear

	if(mpu_reset(MPU_RESET_ALL)){
		LOG_T("Reset successful.");
	} else {
		LOG_E("Reset failed - Bus stuck?");
	}

	term_busy = false;
}

/** @brief Command: sleep [on/off/temp=on/off] -> Manages power modes of the sensor. */
void cmd_sleep(const char* a){
	if (a == NULL){
		LOG_W("Usage: sleep [on/off] OR temp=[on/off]");
		return;
	}
	term_busy = true;

	const char* tmp_val = term_get_arg(a, "temp");

	/* Check for direct 'on'/'off' shorthand (affects entire device) */
	if (strcmp(a, "on") == 0){
		mpu_sleep(MPU_SLEEP_DEVICE_ON);
		LOG_T("Sleep mode: Device %sON%s", ANSI_YELLOW, ANSI_RESET);
	}else if (strcmp(a, "off") == 0){
		mpu_sleep(MPU_SLEEP_ALL_OFF);
		LOG_T("Sleep mode: Device %sOFF%s", ANSI_GREEN, ANSI_RESET);
	}else if (tmp_val){
		/* Handle temp=on/off specific arguments */
		if (strcmp(tmp_val, "on") == 0){
			mpu_sleep(MPU_SLEEP_TEMP_ON);
			LOG_T("Sleep mode: Temp %sON%s", ANSI_YELLOW, ANSI_RESET);
		} else if (strcmp(tmp_val, "off") == 0){
			mpu_sleep(MPU_SLEEP_TEMP_OFF);
			LOG_T("Sleep mode: Temp %sOFF%s", ANSI_GREEN, ANSI_RESET);
		}
	} else {
		LOG_E("Invalid argument. Use [on/off] OR temp=[on/off]");
	}
	term_busy = false;
}

/** @brief Command: clear -> Erases the log area and restores the visual UI frame. */
void cmd_clear_logs(const char* a){
	(void)a;
	term_busy = true;
	/* Erase rows 1 to 35 using ANSI Escape codes */
	for (int i = TERM_LOG_START; i <= TERM_LOG_END; i++){
		printf("\033[%d;1H\033[K", i);
	}
	/* Restore the vertical separator line */
	for (int i = 1; i < TERM_CMD_ROW; i++){
		printf("\033[%d;%dH|", i, TERM_SIDEBAR_COL - 2);
	}
	LOG_T("Logs cleared.");
	term_busy = false;
}

/** @brief Command: uptime -> Displays system runtime in HH:MM:SS:mmm format. */
void cmd_uptime(const char* a){
	(void)a;
	uint32_t t = to_ms_since_boot(get_absolute_time());
	uint32_t ms = t % 1000;
	uint32_t s  = (t / 1000) % 60;
	uint32_t m  = (t / 60000) % 60;
	uint32_t h  = (t / 3600000);
	LOG_T("System Uptime: %s%02d:%02d:%02d:%03d%s", ANSI_BRIGHT_CYAN, h, m, s, ms, ANSI_RESET);
}

/** @brief Command: add <name1> <name2> ... -> Bulk-adds variables to the sidebar. */
void cmd_sidebar_add(const char* a){
	if (a == NULL){
		LOG_W("Usage: add <name1> <name2> ...");
		return;
	}

	/* Use a local buffer as strtok modifies the source string */
	char buf[TERM_CMD_MAX_LEN];
	strncpy(buf, a, sizeof(buf) - 1);

	char *name = strtok(buf, " ");
	while (name != NULL){
		bool found = false;
		/* Iterate through dictionary to find variable address */
		for (uint8_t i = 0; i < WATCH_DICT_COUNT; i++){
			if (strcmp(name, watch_dictionary[i].name) == 0){
				if (term_sidebar_register(watch_dictionary[i].name,
							watch_dictionary[i].ptr,
							watch_dictionary[i].type,
							watch_dictionary[i].unit)){
					LOG_T("Added: %s", name);
				} else {
					LOG_E("Sidebar full!");
				}
				found = true;
				break;
			}
		}
		if (!found) LOG_E("Unknown variable: %s", name);

		name = strtok(NULL, " "); /* Fetch next token */
	}
}

/** @brief Command: rem <name1> <name2> ... -> Bulk-removes variables from the sidebar. */
void cmd_sidebar_remove_bulk(const char* a){
	if (a == NULL){
		LOG_W("Usage: rem <name1> <name2> ...");
		return;
	}

	char buf[TERM_CMD_MAX_LEN];
	strncpy(buf, a, sizeof(buf) - 1);

	char *name = strtok(buf, " ");
	while (name != NULL){
		if (term_sidebar_remove(name)){
			LOG_T("Removed: %s", name);
		} else {
			LOG_E("Not active: %s", name);
		}
		name = strtok(NULL, " ");
	}
}

/** @brief Command: list -> Lists all available variables in the watch_dictionary. */
void cmd_list_vars(const char* a){
	(void)a;
	LOG_T("%sAvailable variables for sidebar:%s", ANSI_BOLD, ANSI_RESET);
	for (uint8_t i = 0; i < WATCH_DICT_COUNT; i++){
		LOG_T(" - %s%-10s%s [%s]", ANSI_YELLOW, watch_dictionary[i].name, ANSI_RESET, watch_dictionary[i].unit);
	}
}

/** @brief Command: banner -> Redraws the startup info banner. */
void cmd_print_banner(const char* a){ (void)a; term_print_banner(); }

/** @brief Command: init -> Re-initializes the TUI and returns to dashboard. */
void cmd_term_init(const char* a) {
    (void)a;
    term_init();
    LOG_T("Terminal UI restored.");
}

/* --- Shell Command Registry --- */
/**
 * @brief Global command table mapping shell names to C functions.
 */
command_t command_config[] = {
	{"help",   cmd_help_auto,      "List all commands", "[command]"},
	{"list",   cmd_list_vars,      "List watchable variables", NULL},
	{"add",    cmd_sidebar_add,    "Add variable to sidebar", "<var> <var> <var> ..."},
	{"rem",    cmd_sidebar_remove_bulk,    "Remove variable from sidebar", "<var> <var> <var> ..."},
	{"clear",  cmd_clear_logs,     "Clear the log area", NULL},
	{"uptime", cmd_uptime,         "Show system uptime", NULL},
	{"status", cmd_mpu_status,     "Show MPU status", NULL},
	{"calib",  cmd_mpu_calibrate,  "Start calibration", "[x/y/z] [samples]"},
	{"sleep",  cmd_sleep,          "Set sleep mode", "on/off/temp=on/off"},
	{"banner", cmd_print_banner,   "Print startup banner", NULL},
	{"init", cmd_term_init, "Return to TUI dashboard mode",  NULL},
	{"reset",  cmd_reset,          "Perform system reboot", NULL}
};
/** @brief Total count of registered shell commands. */
const int command_count = sizeof(command_config) / sizeof(command_t);

/**
 * @brief Runtime initialization of the terminal configuration.
 *
 * Links the pointers in watch_dictionary to the global g_mpu struct members.
 * Must be called before term_init().
 */
void term_cfg_init(){
	/* Link dictionary pointers to the hardware struct at runtime */
	watch_dictionary[0].ptr = (void*)&(g_mpu->v.accel.g.x);
	watch_dictionary[1].ptr = (void*)&(g_mpu->v.accel.g.y);
	watch_dictionary[2].ptr = (void*)&(g_mpu->v.accel.g.z);
	watch_dictionary[3].ptr = (void*)&(g_mpu->v.gyro.dps.x);
	watch_dictionary[4].ptr = (void*)&(g_mpu->v.gyro.dps.y);
	watch_dictionary[5].ptr = (void*)&(g_mpu->v.gyro.dps.z);
	watch_dictionary[6].ptr = (void*)&(g_mpu->v.temp.celsius);

	/* Register default items to show immediately on boot */
	term_sidebar_register("Status", (void*)"ACTIVE", TYPE_STRING, "");
}
