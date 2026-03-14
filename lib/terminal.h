/**
 * @file terminal.h
 * @brief Portable TUI (Terminal User Interface) Engine for Microcontrollers.
 *
 * This engine provides a structured 3-zone layout:
 * 1. A scrolling log area for system messages and telemetry.
 * 2. A dynamic sidebar for real-time sensor data.
 * 3. A persistent command-line interface with history and autocomplete.
 *
 * Designed for ANSI-compatible serial terminals (Neovim, Minicom, PuTTY).
 *
 * @author Robin
 * @version 1.2.3
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stdbool.h>
#include "rp_pico.h"

/**
 * @name UI Geometry Definitions
 * @{
 */
#define TERM_WIDTH        130  /**< Total character width of the terminal. */
#define TERM_SIDEBAR_COL  90   /**< Starting column for the static telemetry sidebar. */
#define TERM_LOG_START    1    /**< First row of the scrolling log region. */
#define TERM_LOG_END      35   /**< Last row of the scrolling log region. */
#define TERM_CMD_ROW      36   /**< Row index for the persistent command-line prompt. */
#define TERM_MAX_HISTORY  10   /**< Number of commands to store in history. */
#define TERM_CMD_MAX_LEN  64   /**< Maximum length per command string. */
#define TERM_SIDEBAR_MAX  20   /**< Maximum number of dynamic sidebar items. */
/** @} */

/** @brief Global drawing lock to prevent overlapping output between logs and sidebar. */
extern volatile bool term_busy;

/* --- Log Macro Overrides --- */

#undef DEBUG_ENABLED
/** @brief Global debug toggle. Set to 1 to enable LOG_D output. */
#define DEBUG_ENABLED 0

#undef LOG_I
/** @brief Compact Information Log. No timestamp, cyan [INFO] prefix. */
#define LOG_I(fmt, ...) do { \
	term_busy = true; \
	/* 1. Position and Scroll */ \
	printf("\033[%d;2H\n\033[%d;2H", TERM_LOG_END, TERM_LOG_END); \
	/* 2. Print Prefix */ \
	printf(ANSI_CYAN "[INFO] " ANSI_RESET); \
	/* 3. Print your message using the declared 'fmt' */ \
	printf(fmt, ##__VA_ARGS__); \
	/* 5. Anchor cursor back to command line */ \
	term_focus_cmd(); \
	term_busy = false; \
} while(0)

#undef LOG_W
/** @brief Warning Log. Uses pico_log with timestamp and yellow formatting. */
#define LOG_W(...) do { \
	term_busy = true; \
	/* 1. Jump to log region (bottom row of scroll area) */ \
	printf("\033[%d;1H", TERM_LOG_END); \
	/* 3. Go back to row TERM_LOG_END to write */ \
	printf("\033[%d;1H", TERM_LOG_END); \
	/* 4. Actual Log-Print */ \
	pico_log(LOG_WARN, __VA_ARGS__); \
	/* 5. Anchor cursor back to command line */ \
	term_focus_cmd(); \
	term_busy = false; \
} while(0)

#undef LOG_E
/** @brief Error Log. Uses pico_log with timestamp and red formatting. */
#define LOG_E(...) do { \
	term_busy = true; \
	/* 1. Jump to log region (bottom row of scroll area) */ \
	printf("\033[%d;1H", TERM_LOG_END); \
	/* 3. Go back to row TERM_LOG_END to write */ \
	printf("\033[%d;1H", TERM_LOG_END); \
	/* 4. Actual Log-Print */ \
	pico_log(LOG_ERROR, __VA_ARGS__); \
	/* 5. Anchor cursor back to command line */ \
	term_focus_cmd(); \
	term_busy = false; \
} while(0)

#if DEBUG_ENABLED
#undef LOG_D
/** @brief Debug Log. Active only if DEBUG_ENABLED is set to 1. */
#define LOG_D(...) do { \
	term_busy = true; \
	/* 1. Jump to log region (bottom row of scroll area) */ \
	printf("\033[%d;1H", TERM_LOG_END); \
	/* 3. Go back to row TERM_LOG_END to write */ \
	printf("\033[%d;1H", TERM_LOG_END); \
	/* 4. Actual Log-Print */ \
	pico_log(LOG_DEBUG, __VA_ARGS__); \
	/* 5. Anchor cursor back to command line */ \
	term_focus_cmd(); \
	term_busy = false; \
} while(0)
#endif

/**
 * @brief Telemetry/Terminal log macro.
 * Used for UI feedback (e.g., "Command executed", "Sidebar updated").
 */
#undef LOG_T
#define LOG_T(fmt, ...) do { \
	term_busy = true; \
	printf("\033[%d;2H\n\033[%d;2H", TERM_LOG_END, TERM_LOG_END); \
	printf(fmt, ##__VA_ARGS__); \
	term_focus_cmd(); \
	term_busy = false; \
} while(0)

/**
 * @brief Supported data types for sidebar telemetry items.
 * Used for correct pointer casting during render.
 */
typedef enum {
	TYPE_INT,    /**< 16-bit signed integer (int16_t). */
	TYPE_FLOAT,  /**< 32-bit floating point. */
	TYPE_STRING  /**< Null-terminated character string. */
} data_type_t;

/**
 * @brief Structure defining a single telemetry entry in the sidebar.
 */
typedef struct {
	const char* label;   /**< Descriptive label shown in sidebar. */
	void* value_ptr;     /**< Pointer to the live variable. */
	data_type_t type;    /**< Data type for formatting. */
	const char* unit;    /**< Physical unit (e.g., "g", "°/s"). */
} sidebar_item_t;

/**
 * @brief Helper structure for the dictionary of watchable variables.
 */
typedef struct {
	const char* name;    /**< Variable name used for the 'add' command. */
	void* ptr;           /**< Address of the variable in memory. */
	data_type_t type;    /**< Data type for sidebar registration. */
	const char* unit;    /**< Physical unit for sidebar registration. */
} watchable_t;

/**
 * @brief Structure defining a shell command.
 */
typedef struct {
	const char* name;                /**< The string that triggers the command. */
	void (*func)(const char* args);  /**< Function pointer to implementation. */
	const char* help;                /**< Short description for the help list. */
	const char* usage;               /**< Detailed argument syntax info. */
} command_t;

/* --- Public API --- */

/** @brief Forces the terminal cursor back to the active command line position. */
void term_focus_cmd(void);

/** @brief Links hardware variables to the internal UI watch-dictionary. */
void term_cfg_init(void);

/** @brief Clears screen, defines scroll regions and draws the static UI frame. */
void term_init(void);

/** @brief Refreshes all values in the dynamic telemetry sidebar. */
void term_update_sidebar(void);

/** @brief Manually prepares the cursor for a log entry within the scroll region. */
void term_scroll_log(void);

/**
 * @brief Non-blocking handler for UART/USB keyboard input.
 * @param key The key code received from the hardware driver.
 */
void term_handle_input(key_t key);

/** @brief Displays the startup banner containing metadata from CMake. */
void term_print_banner(void);

/**
 * @brief Parses the command string to find key=value pairs.
 * @return Pointer to the value string, or NULL if key is not found.
 */
const char* term_get_arg(const char* args, const char* key);

/**
 * @brief Registers a variable to be displayed in the sidebar at runtime.
 * @return true if successful, false if sidebar limit is reached.
 */
bool term_sidebar_register(const char* label, void* ptr, data_type_t type, const char* unit);

/** @brief Removes an item from the sidebar based on its label name. */
bool term_sidebar_remove(const char* label);

#endif // TERMINAL_H
