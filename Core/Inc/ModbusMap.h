#ifndef _MODBUSMAP_H
#define _MODBUSMAP_H

#define TOTAL_REG_COUNT 39

// Motor 1 registers (0x0000 - 0x000D)
#define REG_M1_CONTROL_MODE     0
#define REG_M1_ONOFF_ENABLE     1
#define REG_M1_LINEAR_ENABLE    2
#define REG_M1_PID_ENABLE       3
#define REG_M1_COMMAND_SPEED    4
#define REG_M1_LINEAR_INPUT     5
#define REG_M1_ACTUAL_SPEED     6
#define REG_M1_DIRECTION        7
#define REG_M1_PID_KP           8
#define REG_M1_PID_KI           9
#define REG_M1_PID_KD           10
#define REG_M1_STATUS_WORD      11
#define REG_M1_ERROR_CODE       12
#define REG_M1_RESERVED         13

// Motor 2 registers (0x0010 - 0x001D)
#define REG_M2_CONTROL_MODE     16
#define REG_M2_ONOFF_ENABLE     17
#define REG_M2_LINEAR_ENABLE    18
#define REG_M2_PID_ENABLE       19
#define REG_M2_COMMAND_SPEED    20
#define REG_M2_LINEAR_INPUT     21
#define REG_M2_ACTUAL_SPEED     22
#define REG_M2_DIRECTION        23
#define REG_M2_PID_KP           24
#define REG_M2_PID_KI           25
#define REG_M2_PID_KD           26
#define REG_M2_STATUS_WORD      27
#define REG_M2_ERROR_CODE       28
#define REG_M2_RESERVED         29

// System registers (0x0020 - 0x0026)
#define REG_DEVICE_ID           32
#define REG_FIRMWARE_VERSION    33
#define REG_SYSTEM_STATUS       34
#define REG_SYSTEM_ERROR        35
#define REG_RESET_ERROR_COMMAND 36
#define REG_CONFIG_BAUDRATE     37
#define REG_CONFIG_PARITY       38

#endif