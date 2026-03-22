LOG_* Style Guide

================================================

Ziel

- Einheitliche, sofort verständliche Logs
- Gut lesbar im Terminal / Ringbuffer / TUI
- Leicht grepbar
- Keine wechselnden Formulierungen für denselben Fall


------------------------------------------------
0. Funktionsname über __func__
------------------------------------------------

Funktionsnamen niemals als String schreiben, sondern immer __func__ verwenden.

Helper (empfohlen):

    #define LOG_FN ANSI_COLOR_FN __func__ "()" ANSI_RESET

Beispiel:

    LOG_E(LOG_FN ": g_hmc = NULL");
    LOG_W(LOG_FN ": overflow detected raw=(%d,%d,%d)", x, y, z);
    LOG_I(LOG_FN ": initialized addr=0x%02X", addr);
    LOG_D(LOG_FN ": raw read x=%d y=%d z=%d", x, y, z);

Warum:

- kein Copy-Paste
- keine Tippfehler
- automatisch korrekt bei Refactoring
- überall konsistent


------------------------------------------------
1. Grundformat
------------------------------------------------

Jede Logmeldung hat die Form:

    <function>(): <message>

Beispiel:

    LOG_E(LOG_FN ": g_hmc = NULL");

Regeln:

- Funktionsname immer zuerst
- Doppelpunkt direkt danach
- danach die Nachricht


------------------------------------------------
2. Schreibstil
------------------------------------------------

Nach ":":

- klein anfangen
- technisch, kurz, direkt

Richtig:

    ": g_hmc = NULL"
    ": invalid len = %u"
    ": register read failed"
    ": initialized"

Falsch:

    ": G_HMC Is NULL"
    ": Invalid Length"
    ": Something went wrong"


------------------------------------------------
3. Nullpointer
------------------------------------------------

Immer exakt dieses Muster:

    <name> = NULL

Beispiele:

    LOG_E(LOG_FN ": g_hmc = NULL");
    LOG_E(LOG_FN ": data = NULL");
    LOG_E(LOG_FN ": out = NULL");

Nicht verwenden:

    ": null pointer"
    ": invalid pointer"
    ": data pointer is null"


------------------------------------------------
4. Ungültige Parameter
------------------------------------------------

Immer:

    invalid <name> = <value>

Beispiele:

    LOG_E(LOG_FN ": invalid len = %u", len);
    LOG_E(LOG_FN ": invalid scale = %.3f", scale);
    LOG_E(LOG_FN ": invalid mode = %u", mode);

Falsch:

    ": bad len"
    ": wrong mode"
    ": invalid scale (0)"


------------------------------------------------
5. Fehlgeschlagene Operationen
------------------------------------------------

Projektstandard:

    <action> failed

Warum:

- Fehler ist durch LOG_E schon klar
- Aktion ist die wichtigste Info

Beispiele:

    LOG_E(LOG_FN ": register select failed reg=0x%02X", reg);
    LOG_E(LOG_FN ": data block read failed reg=0x%02X", reg);

    LOG_E(LOG_FN ": I2C write failed addr=0x%02X reg=0x%02X len=%u",
          addr, reg, len);

    LOG_E(LOG_FN ": I2C read failed addr=0x%02X reg=0x%02X len=%u",
          addr, reg, len);

Falsch:

    ": failed to read register"
    ": failed to select reg"


------------------------------------------------
6. Kontextwerte
------------------------------------------------

Immer als:

    key=value

Beispiele:

    LOG_E(LOG_FN ": I2C write failed addr=0x%02X reg=0x%02X len=%u nostop=%u",
          addr, reg, len, nostop);

    LOG_W(LOG_FN ": overflow detected raw=(%d,%d,%d)", x, y, z);

    LOG_I(LOG_FN ": initialized addr=0x%02X scale=%.3f", addr, scale);

Regel:

- erst Problem
- dann Werte


------------------------------------------------
7. Keine Multiline-Logs
------------------------------------------------

Verboten:

    LOG_E("line1\nline2");

Grund:

- zerstört TUI / Layout
- schlecht lesbar


------------------------------------------------
8. Log-Level Bedeutung
------------------------------------------------

LOG_E (Error)

Für echte Fehler:

- NULL Pointer
- ungültige Parameter
- Hardwarefehler
- inkonsistente Zustände

    LOG_E(LOG_FN ": out = NULL");
    LOG_E(LOG_FN ": invalid len = %u", len);
    LOG_E(LOG_FN ": I2C read failed addr=0x%02X reg=0x%02X len=%u", ...);


LOG_W (Warning)

Für ungewöhnliche, aber abgefangene Zustände:

    LOG_W(LOG_FN ": overflow detected raw=(%d,%d,%d)", x, y, z);
    LOG_W(LOG_FN ": config missing, using defaults");


LOG_I (Info)

Für wichtige normale Ereignisse:

- Initialisierung
- Statusänderungen
- Konfiguration

    LOG_I(LOG_FN ": initialized addr=0x%02X scale=%.3f", addr, scale);
    LOG_I(LOG_FN ": mode set mode=%u", mode);
    LOG_I(LOG_FN ": offset updated x=%d y=%d z=%d", x, y, z);

Nicht verwenden:

    ": entered function"
    ": loop iteration"
    ": read byte"


LOG_D (Debug)

Für Entwickler-Infos:

    LOG_D(LOG_FN ": raw read x=%d y=%d z=%d", x, y, z);
    LOG_D(LOG_FN ": reg select reg=0x%02X len=%u", reg, len);

Regeln:

- darf ausführlicher sein
- trotzdem kurz bleiben

Hinweis:

    Bei Veröffentlichung Debug-Logs deaktivieren nicht vergessen.


------------------------------------------------
9. Wortwahl
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
10. Wichtigste Regel
------------------------------------------------

Gleiches Problem → exakt gleiche Formulierung

Richtig:

    ": g_hmc = NULL"
    ": invalid len = %u"
    ": I2C read failed ..."

Falsch:

    ": g_hmc was null"
    ": null device pointer"
    ": wrong len"
    ": failed to read"


------------------------------------------------
11. Referenzbeispiele
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
12. Kurzregeln
------------------------------------------------

1. function(): message
2. Funktionsname über __func__
3. nach ":" klein schreiben
4. name = NULL
5. invalid name = value
6. <action> failed
7. key=value
8. LOG_I sparsam
9. LOG_D nur Debug
10. eine Zeile pro Log
11. gleiche Formulierung für gleiche Fehler

================================================
