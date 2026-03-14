#ifndef ANSI_H
#define ANSI_H

/**
 * @name Stringification Helpers
 * @brief Macros to convert numeric literals to strings for compile-time concatenation.
 * @{
 */
#define ANSI_STR_HELPER(x) #x                   /**< Converts macro argument to string literal. */
#define ANSI_STR(x) ANSI_STR_HELPER(x)          /**< Double-wrapper for macro expansion. */
#define ANSI_ESC            "\033["             /**< Standard ANSI Escape Sequence prefix. */
/** @} */

/**
 * @name Terminal Control
 * @{
 */
#define ANSI_RESET          "\033[0m"           /**< Reset all text attributes and colors. */
#define ANSI_CLR            "\033[2J"           /**< Clear entire screen buffer. */
#define ANSI_CLR_LINE       "\033[2K"           /**< Erase current line contents. */
#define ANSI_HOME           "\033[H"            /**< Move cursor to top-left (1,1). */
#define ANSI_OVERWRITE      "\r\033[2K"         /**< Move to start of line and clear it. */
/** @} */

/**
 * @name Text Decoration
 * @{
 */
#define ANSI_BOLD           "\033[1m"           /**< Bold or high intensity text. */
#define ANSI_DIM            "\033[2m"           /**< Decreased intensity text. */
#define ANSI_ITALIC         "\033[3m"           /**< Slanted text (limited terminal support). */
#define ANSI_UNDERLINE      "\033[4m"           /**< Underlined text. */
#define ANSI_BLINK          "\033[5m"           /**< Flashing text. */
#define ANSI_REVERSE        "\033[7m"           /**< Swap foreground and background colors. */
#define ANSI_HIDDEN         "\033[8m"           /**< Invisible text (still occupies space). */
#define ANSI_STRIKE         "\033[9m"           /**< Strike-through decoration. */
/** @} */

/**
 * @name Cursor Manipulation
 * @note Only works with literal numeric arguments due to compile-time concatenation.
 * @{
 */
#define ANSI_HIDE_CUR       "\033[?25l"         /**< Hide the terminal cursor. */
#define ANSI_SHOW_CUR       "\033[?25h"         /**< Show the terminal cursor. */
#define ANSI_GOTO(y, x)     ANSI_ESC ANSI_STR(y) ";" ANSI_STR(x) "H" /**< Move to row y, col x. */
#define ANSI_GOTO_COL(x)    ANSI_ESC ANSI_STR(x) "G"                 /**< Move to column x. */
#define ANSI_CUR_UP(n)      ANSI_ESC ANSI_STR(n) "A"                 /**< Move up n rows. */
#define ANSI_CUR_DOWN(n)    ANSI_ESC ANSI_STR(n) "B"                 /**< Move down n rows. */
#define ANSI_CUR_RIGHT(n)   ANSI_ESC ANSI_STR(n) "C"                 /**< Move right n columns. */
#define ANSI_CUR_LEFT(n)    ANSI_ESC ANSI_STR(n) "D"                 /**< Move left n columns. */
/** @} */

/**
 * @name Standard Foreground Colors (3x)
 * @{
 */
#define ANSI_BLACK          "\033[30m"          /**< Standard Black text. */
#define ANSI_RED            "\033[31m"          /**< Standard Red text. */
#define ANSI_GREEN          "\033[32m"          /**< Standard Green text. */
#define ANSI_YELLOW         "\033[33m"          /**< Standard Yellow text. */
#define ANSI_BLUE           "\033[34m"          /**< Standard Blue text. */
#define ANSI_MAGENTA        "\033[35m"          /**< Standard Magenta text. */
#define ANSI_CYAN           "\033[36m"          /**< Standard Cyan text. */
#define ANSI_WHITE          "\033[37m"          /**< Standard White text. */
/** @} */

/**
 * @name Bright Foreground Colors (9x)
 * @{
 */
#define ANSI_BRIGHT_BLACK   "\033[90m"          /**< Bright Black (Dark Gray). */
#define ANSI_BRIGHT_RED     "\033[91m"          /**< Bright Red text. */
#define ANSI_BRIGHT_GREEN   "\033[92m"          /**< Bright Green text. */
#define ANSI_BRIGHT_YELLOW  "\033[93m"          /**< Bright Yellow text. */
#define ANSI_BRIGHT_BLUE    "\033[94m"          /**< Bright Blue text. */
#define ANSI_BRIGHT_MAGENTA "\033[95m"          /**< Bright Magenta text. */
#define ANSI_BRIGHT_CYAN    "\033[96m"          /**< Bright Cyan text. */
#define ANSI_BRIGHT_WHITE   "\033[97m"          /**< Bright White text. */
/** @} */

/**
 * @name Background Colors (4x)
 * @{
 */
#define ANSI_BG_BLACK       "\033[40m"          /**< Black background. */
#define ANSI_BG_RED         "\033[41m"          /**< Red background. */
#define ANSI_BG_GREEN       "\033[42m"          /**< Green background. */
#define ANSI_BG_YELLOW      "\033[43m"          /**< Yellow background. */
#define ANSI_BG_BLUE        "\033[44m"          /**< Blue background. */
#define ANSI_BG_MAGENTA     "\033[45m"          /**< Magenta background. */
#define ANSI_BG_CYAN        "\033[46m"          /**< Cyan background. */
#define ANSI_BG_WHITE       "\033[47m"          /**< White background. */
/** @} */

 /**
 * @name Extended Color Palettes
 * @param n Color index (0-255).
 * @{
 */
#define ANSI_FG_256(n)      ANSI_ESC "38;5;" ANSI_STR(n) "m" /**< Set foreground to 256-color index n. */
#define ANSI_BG_256(n)      ANSI_ESC "48;5;" ANSI_STR(n) "m" /**< Set background to 256-color index n. */
/** @} */

/**
 * @name Screen Buffer Management (App Mode)
 * @brief Switch between the main terminal history and a clean app screen.
 * @{
 */
#define ANSI_BUF_ALT        "\033[?1049h"       /**< Switch to alternative screen buffer (clean slate). */
#define ANSI_BUF_MAIN       "\033[?1049l"       /**< Switch back to main buffer (restores terminal history). */
/** @} */

/**
 * @name Scrolling Regions
 * @brief Fix parts of the screen (e.g., a header) while letting others scroll.
 * @{
 */
/** 
 * @brief Sets a scrolling region between two rows. 
 * @param start Top line of the region (1-indexed).
 * @param end Bottom line of the region.
 */
#define ANSI_SET_SCROLL(start, end) "\033[" ANSI_STR(start) ";" ANSI_STR(end) "r"
#define ANSI_RESET_SCROLL   "\033[r"            /**< Reset scrolling to the full screen. */
/** @} */

/**
 * @name TrueColor Support (24-bit RGB)
 * @brief Direct RGB control for modern terminals (16.7 million colors).
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 * @{
 */
#define ANSI_FG_RGB(r, g, b) "\033[38;2;" ANSI_STR(r) ";" ANSI_STR(g) ";" ANSI_STR(b) "m"
#define ANSI_BG_RGB(r, g, b) "\033[48;2;" ANSI_STR(r) ";" ANSI_STR(g) ";" ANSI_STR(b) "m"
/** @} */

/**
 * @name Advanced Cursor & Viewport
 * @{
 */
#define ANSI_SAVE_CUR       "\033[s"            /**< Save current cursor position (SCO). */
#define ANSI_RESTORE_CUR    "\033[u"            /**< Restore previously saved cursor position. */
#define ANSI_REQ_CUR_POS    "\033[6n"           /**< Request cursor position (Reports back as ESC[n;mR via UART). */
#define ANSI_WRAP_ON        "\033[7h"           /**< Enable automatic line wrapping. */
#define ANSI_WRAP_OFF       "\033[7l"           /**< Disable automatic line wrapping. */
/** @} */

/**
 * @name Box Drawing / Special Graphics (ACS)
 * @brief Enables the "Alternate Character Set" for drawing frames and boxes.
 * @{
 */
#define ANSI_ACS_START      "\033(0"            /**< Start Graphics Mode (j=corner, q=horizontal, x=vertical, etc.). */
#define ANSI_ACS_END        "\033(B"            /**< Return to standard ASCII Mode. */
/** @} */

#endif
