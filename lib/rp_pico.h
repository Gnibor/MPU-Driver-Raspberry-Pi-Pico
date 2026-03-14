#ifndef RP_PICO_H
#define RP_PICO_H
#include "hardware/i2c.h"
#define DEBUG_ENABLED 0
#define LOG_NEW_LINE 1

/**
 * @brief Enumeration of supported keyboard inputs.
 * 
 * Includes standard ASCII characters and translated escape sequences 
 * for navigation and editing keys.
 */
typedef enum {
	KEY_NONE      = 0,   /**< No key pressed or timeout. */
	KEY_TAB       = 9,   /**< Tabulator key for autocomplete. */
	KEY_ENTER     = 13,  /**< Enter / Carriage Return key. */
	KEY_ESC       = 27,  /**< Escape key or start of ANSI sequence. */
	KEY_SPACE     = 32,  /**< Space bar. */

	/* Extended Keys (mapped from ANSI escape sequences) */
	KEY_UP        = 128, /**< Arrow Up - Previous history item. */
	KEY_DOWN      = 129, /**< Arrow Down - Next history item. */
	KEY_RIGHT     = 130, /**< Arrow Right - Move cursor right. */
	KEY_LEFT      = 131, /**< Arrow Left - Move cursor left. */
	KEY_BACKSPACE = 132, /**< Backspace - Delete character to the left. */
	KEY_DELETE    = 133, /**< Delete - Remove character at current position. */
	KEY_HOME      = 134, /**< Home - Jump to start of line. */
	KEY_END       = 135  /**< End - Jump to end of line. */
} key_t;

// Log Levels
typedef enum {
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_DEBUG
} log_level_t;

// The core logging function
void pico_log(log_level_t level, const char *fmt, ...);

// Handy macros for shorter calls
#define LOG_I(...) pico_log(LOG_INFO,  __VA_ARGS__)
#define LOG_W(...) pico_log(LOG_WARN,  __VA_ARGS__)
#define LOG_E(...) pico_log(LOG_ERROR, __VA_ARGS__)

// The Magic: If disabled, LOG_D does absolutely nothing
#if DEBUG_ENABLED
#define LOG_D(...) pico_log(LOG_DEBUG, __VA_ARGS__)
#else
#define LOG_D(...) ((void)0) // Completely ignored by the compiler
#endif

key_t get_key();
bool is_i2c_initialized(i2c_inst_t *i2c);
#endif
