LOG_* Style Guide

================================================

Purpose

- Consistent, immediately understandable logs
- Easy to read in terminal / ringbuffer / TUI
- Easy to grep
- No variation for identical problems


------------------------------------------------
0. Function name via __func__
------------------------------------------------

Never write function names as strings.
Always use __func__.

Recommended helper:

    #define LOG_FN ANSI_COLOR_FN __func__ "()" ANSI_RESET

Example:

    LOG_E(LOG_FN ": g_hmc = NULL");
    LOG_W(LOG_FN ": overflow detected raw=(%d,%d,%d)", x, y, z);
    LOG_I(LOG_FN ": initialized addr=0x%02X", addr);
    LOG_D(LOG_FN ": raw read x=%d y=%d z=%d", x, y, z);

Why:

- no copy-paste
- no typos
- automatically correct after refactoring
- consistent everywhere


------------------------------------------------
1. General format
------------------------------------------------

Every log message follows:

    <function>(): <message>

Example:

    LOG_E(LOG_FN ": g_hmc = NULL");

Rules:

- function name first
- colon immediately after
- message follows


------------------------------------------------
2. Writing style
------------------------------------------------

After ":":

- start lowercase
- technical, short, direct

Correct:

    ": g_hmc = NULL"
    ": invalid len = %u"
    ": register read failed"
    ": initialized"

Incorrect:

    ": G_HMC Is NULL"
    ": Invalid Length"
    ": Something went wrong"


------------------------------------------------
3. Null pointers
------------------------------------------------

Always use:

    <name> = NULL

Examples:

    LOG_E(LOG_FN ": g_hmc = NULL");
    LOG_E(LOG_FN ": data = NULL");
    LOG_E(LOG_FN ": out = NULL");

Do NOT use:

    ": null pointer"
    ": invalid pointer"
    ": data pointer is null"


------------------------------------------------
4. Invalid parameters
------------------------------------------------

Always:

    invalid <name> = <value>

Examples:

    LOG_E(LOG_FN ": invalid len = %u", len);
    LOG_E(LOG_FN ": invalid scale = %.3f", scale);
    LOG_E(LOG_FN ": invalid mode = %u", mode);

Incorrect:

    ": bad len"
    ": wrong mode"
    ": invalid scale (0)"


------------------------------------------------
5. Failed operations
------------------------------------------------

Project standard:

    <action> failed

Why:

- error is already clear from LOG_E
- action is the most important information

Examples:

    LOG_E(LOG_FN ": register select failed reg=0x%02X", reg);
    LOG_E(LOG_FN ": data block read failed reg=0x%02X", reg);

    LOG_E(LOG_FN ": I2C write failed addr=0x%02X reg=0x%02X len=%u",
          addr, reg, len);

    LOG_E(LOG_FN ": I2C read failed addr=0x%02X reg=0x%02X len=%u",
          addr, reg, len);

Incorrect:

    ": failed to read register"
    ": failed to select reg"


------------------------------------------------
6. Context values
------------------------------------------------

Always use:

    key=value

Examples:

    LOG_E(LOG_FN ": I2C write failed addr=0x%02X reg=0x%02X len=%u nostop=%u",
          addr, reg, len, nostop);

    LOG_W(LOG_FN ": overflow detected raw=(%d,%d,%d)", x, y, z);

    LOG_I(LOG_FN ": initialized addr=0x%02X scale=%.3f", addr, scale);

Rule:

- problem first
- then values


------------------------------------------------
7. No multiline logs
------------------------------------------------

Forbidden:

    LOG_E("line1\nline2");

Reason:

- breaks TUI / layout
- hard to read


------------------------------------------------
8. Log level meaning
------------------------------------------------

LOG_E (Error)

Use for real errors:

- NULL pointers
- invalid parameters
- hardware failures
- inconsistent states

    LOG_E(LOG_FN ": out = NULL");
    LOG_E(LOG_FN ": invalid len = %u", len);
    LOG_E(LOG_FN ": I2C read failed addr=0x%02X reg=0x%02X len=%u", ...);


LOG_W (Warning)

For unusual but handled conditions:

    LOG_W(LOG_FN ": overflow detected raw=(%d,%d,%d)", x, y, z);
    LOG_W(LOG_FN ": config missing, using defaults");


LOG_I (Info)

For important normal events:

- initialization
- state changes
- configuration

    LOG_I(LOG_FN ": initialized addr=0x%02X scale=%.3f", addr, scale);
    LOG_I(LOG_FN ": mode set mode=%u", mode);
    LOG_I(LOG_FN ": offset updated x=%d y=%d z=%d", x, y, z);

Do NOT use for:

    ": entered function"
    ": loop iteration"
    ": read byte"


LOG_D (Debug)

For developer/debug information:

    LOG_D(LOG_FN ": raw read x=%d y=%d z=%d", x, y, z);
    LOG_D(LOG_FN ": reg select reg=0x%02X len=%u", reg, len);

Rules:

- can be more verbose
- still keep it short

Note:

    Disable debug logs before release.


------------------------------------------------
9. Vocabulary
------------------------------------------------

LOG_E:

    invalid ...
    <action> failed
    <name> = NULL

LOG_W:

    overflow detected
    config missing, using defaults
    value clipped

LOG_I:

    initialized
    enabled
    mode set
    offset updated

LOG_D:

    raw read
    state=
    cfg=
    computed


------------------------------------------------
10. Most important rule
------------------------------------------------

Same problem → exact same wording

Correct:

    ": g_hmc = NULL"
    ": invalid len = %u"
    ": I2C read failed ..."

Incorrect:

    ": g_hmc was null"
    ": null device pointer"
    ": wrong len"
    ": failed to read"


------------------------------------------------
11. Reference examples
------------------------------------------------

#define LOG_FN ANSI_COLOR_FN __func__ "()" ANSI_RESET

// ERROR
LOG_E(LOG_FN ": g_hmc = NULL");
LOG_E(LOG_FN ": data = NULL");
LOG_E(LOG_FN ": invalid len = %u", len);
LOG_E(LOG_FN ": I2C write failed addr=0x%02X reg=0x%02X len=%u", ...);

// WARNING
LOG_W(LOG_FN ": overflow detected raw=(%d,%d,%d)", x, y, z);

// INFO
LOG_I(LOG_FN ": initialized addr=0x%02X scale=%.3f", addr, scale);

// DEBUG
LOG_D(LOG_FN ": raw read x=%d y=%d z=%d", x, y, z);


------------------------------------------------
12. Quick rules
------------------------------------------------

1. function(): message
2. use __func__ for function name
3. lowercase after ":"
4. name = NULL
5. invalid name = value
6. <action> failed
7. key=value
8. use LOG_I sparingly
9. LOG_D only for debugging
10. one log = one line
11. same wording for same problems

================================================
