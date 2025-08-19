#ifndef MODBUS_REGISTERS_H
#define MODBUS_REGISTERS_H

#include <stdint.h>

// System Registers (Global) - Addresses 0x00F0-0x00F6
#define REG_DEVICE_ID                   0x00F0
#define REG_FIRMWARE_VERSION            0x00F1
#define REG_SYSTEM_STATUS               0x00F2
#define REG_SYSTEM_ERROR                0x00F3
#define REG_RESET_ERROR_COMMAND         0x00F4
#define REG_CONFIG_BAUDRATE             0x00F5
#define REG_CONFIG_PARITY               0x00F6

// Motor 1 Registers - Addresses 0x0000-0x000C
#define REG_M1_CONTROL_MODE             0x0000
#define REG_M1_ENABLE                   0x0001
#define REG_M1_COMMAND_SPEED            0x0002
#define REG_M1_LINEAR_INPUT             0x0003
#define REG_M1_LINEAR_UNIT              0x0004
#define REG_M1_LINEAR_STATE             0x0005
#define REG_M1_ACTUAL_SPEED             0x0006
#define REG_M1_DIRECTION                0x0007
#define REG_M1_PID_KP                   0x0008
#define REG_M1_PID_KI                   0x0009
#define REG_M1_PID_KD                   0x000A
#define REG_M1_STATUS_WORD              0x000B
#define REG_M1_ERROR_CODE               0x000C
#define REG_M1_ONOFF_SPEED              0x000D

// Motor 2 Registers - Addresses 0x0010-0x001C
#define REG_M2_CONTROL_MODE             0x0010
#define REG_M2_ENABLE                   0x0011
#define REG_M2_COMMAND_SPEED            0x0012
#define REG_M2_LINEAR_INPUT             0x0013
#define REG_M2_LINEAR_UNIT              0x0014
#define REG_M2_LINEAR_STATE             0x0015
#define REG_M2_ACTUAL_SPEED             0x0016
#define REG_M2_DIRECTION                0x0017
#define REG_M2_PID_KP                   0x0018
#define REG_M2_PID_KI                   0x0019
#define REG_M2_PID_KD                   0x001A
#define REG_M2_STATUS_WORD              0x001B
#define REG_M2_ERROR_CODE               0x001C
#define REG_M2_ONOFF_SPEED              0x001D

// Input Registers - Addresses 0x0020-0x0024
#define REG_DI_STATUS_WORD              0x0020
#define REG_DI1_ASSIGNMENT              0x0021
#define REG_DI2_ASSIGNMENT              0x0022
#define REG_DI3_ASSIGNMENT              0x0023
#define REG_DI4_ASSIGNMENT              0x0024

// Output Registers - Addresses 0x0030-0x0034
#define REG_DO_STATUS_WORD              0x0030
#define REG_DO1_CONTROL                 0x0031
#define REG_DO1_ASSIGNMENT              0x0032
#define REG_DO2_CONTROL                 0x0033
#define REG_DO2_ASSIGNMENT              0x0034

// Register Counts
#define SYSTEM_REG_COUNT                7
#define MOTOR1_REG_COUNT                13
#define MOTOR2_REG_COUNT                13
#define INPUT_REG_COUNT                 5
#define OUTPUT_REG_COUNT                5
#define TOTAL_HOLDING_REG_COUNT         53

// Default Values
#define DEFAULT_DEVICE_ID               3
#define DEFAULT_FIRMWARE_VERSION        0x0101
#define DEFAULT_SYSTEM_STATUS           0x0000
#define DEFAULT_SYSTEM_ERROR            0
#define DEFAULT_CONFIG_BAUDRATE         1
#define DEFAULT_CONFIG_PARITY           0

#define DEFAULT_M1_CONTROL_MODE         1
#define DEFAULT_M1_ENABLE               0
#define DEFAULT_M1_COMMAND_SPEED        0
#define DEFAULT_M1_LINEAR_INPUT         0
#define DEFAULT_M1_LINEAR_UNIT          5
#define DEFAULT_M1_LINEAR_STATE         0
#define DEFAULT_M1_ACTUAL_SPEED         0
#define DEFAULT_M1_DIRECTION            0
#define DEFAULT_M1_PID_KP               100
#define DEFAULT_M1_PID_KI               10
#define DEFAULT_M1_PID_KD               5
#define DEFAULT_M1_STATUS_WORD          0x0000
#define DEFAULT_M1_ERROR_CODE           0

#define DEFAULT_M2_CONTROL_MODE         1
#define DEFAULT_M2_ENABLE               0
#define DEFAULT_M2_COMMAND_SPEED        0
#define DEFAULT_M2_LINEAR_INPUT         0
#define DEFAULT_M2_LINEAR_UNIT          5
#define DEFAULT_M2_LINEAR_STATE         0
#define DEFAULT_M2_ACTUAL_SPEED         0
#define DEFAULT_M2_DIRECTION            0
#define DEFAULT_M2_PID_KP               100
#define DEFAULT_M2_PID_KI               10
#define DEFAULT_M2_PID_KD               5
#define DEFAULT_M2_STATUS_WORD          0x0000
#define DEFAULT_M2_ERROR_CODE           0

#define DEFAULT_DI_STATUS_WORD          0x0000
#define DEFAULT_DI1_ASSIGNMENT          0
#define DEFAULT_DI2_ASSIGNMENT          0
#define DEFAULT_DI3_ASSIGNMENT          0
#define DEFAULT_DI4_ASSIGNMENT          0

#define DEFAULT_DO_STATUS_WORD          0x0000
#define DEFAULT_DO1_CONTROL             0
#define DEFAULT_DO1_ASSIGNMENT          0
#define DEFAULT_DO2_CONTROL             0
#define DEFAULT_DO2_ASSIGNMENT          0

// Control Mode Values
#define CONTROL_MODE_ONOFF              1
#define CONTROL_MODE_LINEAR             2
#define CONTROL_MODE_PID                3

// Enable/Disable Values
#define MOTOR_DISABLE                   0
#define MOTOR_ENABLE                    1

// Direction Values
#define DIRECTION_IDLE                  0
#define DIRECTION_FORWARD               1
#define DIRECTION_REVERSE               2

// Linear State Values
#define LINEAR_STATE_DECELERATION       0
#define LINEAR_STATE_ACCELERATION       1

// Parity Values
#define PARITY_NONE                     0
#define PARITY_EVEN                     1
#define PARITY_ODD                      2

// Baudrate Values
#define BAUD_9600                       1
#define BAUD_19200                      2
#define BAUD_38400                      3
#define BAUD_57600                      4
#define BAUD_115200                     5

// Digital Input Assignment Values
#define DI_ASSIGN_NONE                  0
#define DI_ASSIGN_START_M1              1
#define DI_ASSIGN_STOP_M1               2
#define DI_ASSIGN_REVERSE_M1            3
#define DI_ASSIGN_FAULT_RESET           4
#define DI_ASSIGN_MODE_SWITCH           5
#define DI_ASSIGN_START_M2              6
#define DI_ASSIGN_STOP_M2               7
#define DI_ASSIGN_REVERSE_M2            8
#define DI_ASSIGN_EMERGENCY_STOP        9
#define DI_ASSIGN_JOG_MODE              10

// Digital Output Assignment Values
#define DO_ASSIGN_NONE                  0
#define DO_ASSIGN_RUNNING_M1            1
#define DO_ASSIGN_FAULT_M1              2
#define DO_ASSIGN_SPEED_REACHED_M1      3
#define DO_ASSIGN_READY                 4
#define DO_ASSIGN_RUNNING_M2            5
#define DO_ASSIGN_FAULT_M2              6
#define DO_ASSIGN_SPEED_REACHED_M2      7
#define DO_ASSIGN_SYSTEM_FAULT          8
#define DO_ASSIGN_WARNING               9
#define DO_ASSIGN_MAINTENANCE           10

#endif // MODBUS_REGISTERS_H
