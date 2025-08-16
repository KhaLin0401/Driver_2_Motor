#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

typedef struct
{
    uint8_t Control_Mode;      // 0x0000: 1=ONOFF, 2=LINEAR, 3=PID
    uint8_t Enable;            // 0x0001: 0=DISABLE, 1=ENABLE
    uint8_t Command_Speed;     // 0x0002: Speed setpoint
    uint8_t Linear_Input;      // 0x0003: Linear control input (0–100%)
    uint8_t Linear_Unit;       // 0x0004: Linear unit (0-20%)
    uint8_t Linear_State;      // 0x0005: 0=DECELERATION, 1=ACCELERATION
    uint8_t Actual_Speed;      // 0x0006: Measured speed
    uint8_t Direction;         // 0x0007: 0=Idle, 1=Forward, 2=Reverse
    uint8_t PID_Kp;            // 0x0008: PID Kp gain (×100)
    uint8_t PID_Ki;            // 0x0009: PID Ki gain (×100)
    uint8_t PID_Kd;            // 0x000A: PID Kd gain (×100)
    uint8_t Status_Word;       // 0x000B: Motor status flags
    uint8_t Error_Code;        // 0x000C: Error code if any
} MotorControl_t;



#endif