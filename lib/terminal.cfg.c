/**
 * @file terminal_cfg.c
 * @brief Project specific configuration for the terminal.
 */

#include "terminal.h"
#include "ansi-esc.h"
#include "mpu60x0.h"

// Prototyp der automatischen Hilfe (aus terminal.c)
extern void cmd_help_auto(const char* args);

/* 1. Register Sidebar Items (Right Column) */
sidebar_item_t sidebar_config[] = {
    {"Accel X", NULL, TYPE_FLOAT,   "g"},
    {"Accel Y", NULL, TYPE_FLOAT,   "g"},
    {"Accel Z", NULL, TYPE_FLOAT,   "g"},
    {"Temp   ", NULL, TYPE_FLOAT, "C"}, // Assuming scaled in update
    {"Gyro X", NULL, TYPE_FLOAT,   "°/s"},
    {"Gyro Y", NULL, TYPE_FLOAT,   "°/s"},
    {"Gyro Z", NULL, TYPE_FLOAT,   "°/s"},
    {"Status ", "ACTIVE", TYPE_STRING, ""}
};
const int sidebar_count = sizeof(sidebar_config) / sizeof(sidebar_item_t);

/**
 * @brief Verknüpft die Hardware-Variablen mit der UI-Konfiguration.
 */
void term_cfg_init() {
    sidebar_config[0].value_ptr = (void*)&(g_mpu->v.accel.g.x);
    sidebar_config[1].value_ptr = (void*)&(g_mpu->v.accel.g.y);
    sidebar_config[2].value_ptr = (void*)&(g_mpu->v.accel.g.z);
    sidebar_config[3].value_ptr = (void*)&(g_mpu->v.temp.celsius); // oder dein berechneter Wert
    sidebar_config[4].value_ptr = (void*)&(g_mpu->v.gyro.dps.x);
    sidebar_config[5].value_ptr = (void*)&(g_mpu->v.gyro.dps.y);
    sidebar_config[6].value_ptr = (void*)&(g_mpu->v.gyro.dps.z);
}

/* 2. Register Commands (Bottom Line) */
// Deine Hardware-Funktionen
void cmd_mpu_status(const char* a) { LOG_I("MPU6050 is %sONLINE%s", ANSI_GREEN, ANSI_RESET); }
void cmd_mpu_calibrate(const char* a) { mpu_calibrate((MPU_ACCEL_Z | MPU_GYRO), 50); LOG_I("Accel, Gyro calibrate sample=50");}
void cmd_reset(const char* a) { mpu_reset(MPU_RESET_ALL); LOG_W("System Reset!"); }
void cmd_sleep_off(const char* a) { mpu_sleep(MPU_SLEEP_ALL_OFF); LOG_I("Sleep turned off!");}
void cmd_sleep_on(const char* a) { mpu_sleep(MPU_SLEEP_DEVICE_ON); LOG_I("Sleep turned on!");}

command_t command_config[] = {
    {"help",   cmd_help_auto,   "Listet alle verfügbaren Befehle auf"},
    {"status", cmd_mpu_status,  "Zeigt den aktuellen Hardware-Status"},
    {"calib",  cmd_mpu_calibrate,   "Startet die Sensor-Kalibrierung"},
    {"sleep off",  cmd_sleep_off,   "Disable sleep mode"},
    {"sleep on",  cmd_sleep_on,   "Enable sleep mode"},
    {"reset",  cmd_reset,       "Führt einen System-Reboot durch"}
};
const int command_count = sizeof(command_config) / sizeof(command_t);

