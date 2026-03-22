#include <stdio.h>
#include <stdarg.h>
#include "ansi-esc.h"
#include "pico/stdio.h"
#include "rp_pico.h"

/**
 * @brief Reads a single character from UART/USB and translates ANSI escape sequences.
 *
 * This function handles multi-byte sequences for navigation and editing keys
 * like Arrows, Home, End, and Delete. It uses a short timeout to differentiate
 * between a standalone ESC key and the start of a sequence.
 *
 * @return The detected key as @ref key_t.
 */
key_t get_key(void) {
    int c = getchar_timeout_us(1);
    if (c == PICO_ERROR_TIMEOUT) return KEY_NONE;

    /* Handle standard Backspace (ASCII 127 = DEL, ASCII 8 = BS) */
    if (c == 127 || c == 8) return KEY_BACKSPACE;

    /* Handle ANSI Escape Sequences (starting with ESC [ ...) */
    if (c == 27) {
        /* Short wait to see if more bytes follow the ESC */
        c = getchar_timeout_us(1);
        if (c == '[') {
            c = getchar_timeout_us(1);
            switch (c) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                case '3':
                    /* Special handling for DELETE (ESC [ 3 ~) */
                    if (getchar_timeout_us(1) == '~') return KEY_DELETE;
                    break;
                default:
                    /* Unknown sequence: consume and return NONE or ESC */
                    return KEY_ESC;
            }
        }
        /* If only ESC was pressed (no '[' followed) */
        return KEY_ESC;
    }

    /* Return standard ASCII character */
    return (key_t)c;
}

/**
 * @brief Simple timestamp container.
 *
 * Values are derived from milliseconds since boot.
 */
typedef struct {
	uint32_t h;   /**< Hours since boot */
	uint32_t m;   /**< Minutes [0..59] */
	uint32_t s;   /**< Seconds [0..59] */
	uint32_t ms;  /**< Milliseconds [0..999] */
} pico_time_t;

/**
 * @brief Return current uptime as split timestamp fields.
 *
 * @return Timestamp struct containing hours, minutes, seconds and milliseconds.
 */
static inline pico_time_t pico_get_timestamp(void)
{
	uint32_t total_ms = to_ms_since_boot(get_absolute_time());
	uint32_t total_s  = total_ms / 1000;

	return (pico_time_t){
		.h  = total_s / 3600,
		.m  = (total_s / 60) % 60,
		.s  = total_s % 60,
		.ms = total_ms % 1000
	};
}

/**
 * @brief Print a formatted uptime timestamp.
 *
 * @details
 * Supported format tokens:
 * - h = hours (2 digits)
 * - m = minutes (2 digits)
 * - s = seconds (2 digits)
 * - S = milliseconds (3 digits)
 *
 * Any other character is printed unchanged.
 *
 * Example:
 * @code
 * pico_tsprintf("h:m:s");
 * pico_tsprintf("[h:m:s:S] ");
 * @endcode
 *
 * @param fmt Format string using h/m/s/S placeholders.
 */
static inline void pico_tsprintf(const char *fmt)
{
	if (!fmt) return;

	pico_time_t t = pico_get_timestamp();

	while (*fmt) {
		switch (*fmt) {
			case 'h': printf("%02u", t.h);  break;
			case 'm': printf("%02u", t.m);  break;
			case 's': printf("%02u", t.s);  break;
			case 'S': printf("%03u", t.ms); break;
			default:  putchar(*fmt);        break;
		}
		fmt++;
	}
}

void pico_log(log_level_t level, const char *fmt, ...){
	printf(ANSI_DIM);
	pico_tsprintf("[h:m:s:S] ");
	printf(ANSI_RESET);

	// 4. Set color and prefix based on level
	switch (level) {
		case LOG_INFO:  printf(ANSI_GREEN  "[INFO] " ANSI_RESET); break;
		case LOG_WARN:  printf(ANSI_YELLOW "[WARN] " ANSI_RESET); break;
		case LOG_ERROR: printf(ANSI_RED    "[ERROR] " ANSI_RESET); break;
		case LOG_DEBUG: printf(ANSI_ITALIC ANSI_CYAN "[DEBUG] " ANSI_RESET); break;
		default: printf(ANSI_BG_MAGENTA ANSI_BLUE "[UNKNOWN] " ANSI_RESET); break;
	}

	// 5. Process the actual message (like printf)
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
#if LOG_NEW_LINE
	// 6. Always end with a newline and reset
	printf(ANSI_RESET"\n");
#endif
}
