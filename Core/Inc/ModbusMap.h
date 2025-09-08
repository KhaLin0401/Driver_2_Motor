#ifndef MODBUSMAP_H
#define MODBUSMAP_H

#include <stdint.h>

// System Registers (Base Address: 0x0000)
#define REG_DEVICE_ID              0x0100
#define REG_CONFIG_BAUDRATE        0x0101
#define REG_CONFIG_PARITY          0x0102
#define REG_CONFIG_STOP_BIT        0x0103
#define REG_MODULE_TYPE            0x0104
#define REG_FIRMWARE_VERSION       0x0105
#define REG_HARDWARE_VERSION       0x0106
#define REG_SYSTEM_STATUS          0x0107
#define REG_SYSTEM_ERROR           0x0108
#define REG_RESET_ERROR_COMMAND    0x0109


// Motor 1 Registers (Base Address: 0x0010)
#define REG_M1_CONTROL_MODE        0x0000
#define REG_M1_ENABLE              0x0001
#define REG_M1_COMMAND_SPEED       0x0002
#define REG_M1_ACTUAL_SPEED        0x0003
#define REG_M1_DIRECTION           0x0004
#define REG_M1_MAX_SPEED           0x0005
#define REG_M1_MIN_SPEED           0x0006
#define REG_M1_PID_KP              0x0007
#define REG_M1_PID_KI              0x0008
#define REG_M1_PID_KD              0x0009
#define REG_M1_MAX_ACCELERATION    0x000A
#define REG_M1_MAX_DECELERATION    0x000B
#define REG_M1_STATUS_WORD         0x000C
#define REG_M1_ERROR_CODE          0x000D

// Motor 2 Registers (Base Address: 0x0020)
#define REG_M2_CONTROL_MODE        0x0010
#define REG_M2_ENABLE              0x0011
#define REG_M2_COMMAND_SPEED       0x0012
#define REG_M2_ACTUAL_SPEED        0x0013
#define REG_M2_DIRECTION           0x0014
#define REG_M2_MAX_SPEED           0x0015
#define REG_M2_MIN_SPEED           0x0016
#define REG_M2_PID_KP              0x0017
#define REG_M2_PID_KI              0x0018
#define REG_M2_PID_KD              0x0019
#define REG_M2_MAX_ACCELERATION    0x001A
#define REG_M2_MAX_DECELERATION    0x001B
#define REG_M2_STATUS_WORD         0x001C
#define REG_M2_ERROR_CODE          0x001D

// Digital Input Registers (Base Address: 0x0030)
#define REG_DI_STATUS_WORD         0x0020
#define REG_DI1_ASSIGNMENT         0x0021
#define REG_DI2_ASSIGNMENT         0x0022
#define REG_DI3_ASSIGNMENT         0x0023
#define REG_DI4_ASSIGNMENT         0x0024
#define REG_CURRENT                0x0025

// Digital Output Registers (Base Address: 0x0040)
#define REG_DO_STATUS_WORD         0x0030
#define REG_DO1_CONTROL            0x0031
#define REG_DO1_ASSIGNMENT         0x0032
#define REG_DO2_CONTROL            0x0033
#define REG_DO2_ASSIGNMENT         0x0034

// Total register count
#define TOTAL_HOLDING_REG_COUNT    0x0045  // Total number of registers

// Default Values for System Registers
#define DEFAULT_DEVICE_ID          3
#define DEFAULT_CONFIG_BAUDRATE    5
#define DEFAULT_CONFIG_PARITY      0
#define DEFAULT_CONFIG_STOP_BIT    1
#define DEFAULT_MODULE_TYPE        4
#define DEFAULT_FIRMWARE_VERSION   0x0001
#define DEFAULT_HARDWARE_VERSION   0x0001
#define DEFAULT_SYSTEM_STATUS      0x0000
#define DEFAULT_SYSTEM_ERROR       0
#define DEFAULT_RESET_ERROR_COMMAND 0


// Default Values for Motor Registers
#define DEFAULT_CONTROL_MODE       1       // ONOFF mode
#define DEFAULT_ENABLE             0       // Disabled
#define DEFAULT_COMMAND_SPEED      0
#define DEFAULT_ACTUAL_SPEED       0
#define DEFAULT_DIRECTION          0       // Idle
#define DEFAULT_MAX_SPEED          100
#define DEFAULT_MIN_SPEED          0
#define DEFAULT_PID_KP             100      // ×100
#define DEFAULT_PID_KI             10       // ×100
#define DEFAULT_PID_KD             5        // ×100
#define DEFAULT_MAX_ACCELERATION   5
#define DEFAULT_MAX_DECELERATION   4
#define DEFAULT_STATUS_WORD        0x0000
#define DEFAULT_ERROR_CODE         0
#define DEFAULT_CURRENT            0

// Control Mode Values
#define CONTROL_MODE_ONOFF        1
#define CONTROL_MODE_PID          2

// Direction Values
#define DIRECTION_IDLE            0
#define DIRECTION_FORWARD         1
#define DIRECTION_REVERSE         2

// Digital I/O Assignment Values
#define DIO_ASSIGN_NONE           0
#define DIO_ASSIGN_START_M1       1
#define DIO_ASSIGN_STOP_M1        2
#define DIO_ASSIGN_REVERSE_M1     3
#define DIO_ASSIGN_FAULT_RESET    4
#define DIO_ASSIGN_MODE_SWITCH    5
#define DIO_ASSIGN_START_M2       6
#define DIO_ASSIGN_STOP_M2        7
#define DIO_ASSIGN_REVERSE_M2     8
#define DIO_ASSIGN_EMERGENCY_STOP 9
#define DIO_ASSIGN_JOG_MODE       10

#endif // 