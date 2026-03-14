/**
 * @file terminal.h
 * @brief Portable TUI Engine for Microcontrollers.
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stdbool.h>
#include "rp_pico.h"

#define TERM_WIDTH       130
#define TERM_SIDEBAR_COL 90
#define TERM_LOG_START   1
#define TERM_LOG_END     35
#define TERM_CMD_ROW     36

extern volatile bool term_busy;

// 1. Alte Definitionen aufheben
#undef LOG_I
#undef LOG_W
#undef LOG_E
#undef DEBUG_ENABLED

#define DEBUG_ENABLED 0

// 2. Neue, UI-gesteuerte Definitionen setzen
#define LOG_I(...) do {term_busy=true; term_scroll_log(); pico_log(LOG_INFO,  __VA_ARGS__); term_busy=false;} while(0)
#define LOG_W(...) do {term_busy=true; term_scroll_log(); pico_log(LOG_WARN,  __VA_ARGS__); term_busy=false;} while(0)
#define LOG_E(...) do {term_busy=true; term_scroll_log(); pico_log(LOG_ERROR, __VA_ARGS__); term_busy=false;} while(0)

/* Data types for sidebar items */
typedef enum { TYPE_INT, TYPE_FLOAT, TYPE_STRING } data_type_t;

typedef struct {
    const char* label;
    void* value_ptr;
    data_type_t type;
    const char* unit;
} sidebar_item_t;

typedef struct {
    const char* name;
    void (*func)(const char* args);
    const char* help;
} command_t;

/* Public API */
void term_cfg_init(void);
void term_init(void);
void term_update_sidebar(void);
void term_scroll_log(void);
void term_handle_input(key_t key); // Non-blocking input handler
void term_log_wrapper(log_level_t level, const char* fmt, ...);

#endif
