# 📘 Function Reference for Dual DC Motor Driver with Modbus RTU (STM32F103C8T6)

This document outlines the key functions used in the firmware for a dual DC motor driver system using Modbus RTU and running on STM32F103C8T6 with FreeRTOS. The driver supports three control modes: ON/OFF, LINEAR, and PID.

## 🔧 System Initialization

### `void System_Init(void)`

Initializes clocks, GPIOs, peripherals (PWM, UART, Timers), and FreeRTOS tasks.

### `void Modbus_Init(void)`

Initializes FreeModbus stack in RTU mode and configures callbacks.

## 🔁 Modbus Handling

### `eMBErrorCode eMBRegHoldingCB(...)`

Modbus callback for holding register read/write. Maps Modbus registers to variables in ModbusData_t.

### `void Update_Modbus_Register_Map(void)`

Updates Modbus RAM variables from actual system state.

## ⚙️ Motor Control Interface

### `void Motor_Init(uint8_t motor_id)`

Initializes motor-specific GPIOs, PWM, timers.

### `void Motor_Run(uint8_t motor_id)`

Runs motor logic according to selected mode and enable flags.

### `void Motor_Stop(uint8_t motor_id)`

Stops motor (sets PWM = 0, disables direction outputs).

### `void Motor_SetSpeed(uint8_t motor_id, int16_t speed)`

Sets PWM duty cycle based on speed command.

### `void Motor_SetDirection(uint8_t motor_id, uint8_t direction)`

Sets motor direction based on command (forward/reverse).

## 📈 PID Controller

### `void PID_Init(PID_t *pid, float kp, float ki, float kd)`

Initializes a PID control structure.

### `float PID_Compute(PID_t *pid, float setpoint, float feedback)`

Calculates PID output for given input and setpoint.

## 🎯 Control Mode Execution

### `void Execute_ONOFF_Mode(uint8_t motor_id)`

Handles logic for ON/OFF mode using enable bit and preset speed.

### `void Execute_LINEAR_Mode(uint8_t motor_id)`

Handles logic for LINEAR mode using input percentage (0–100%).

### `void Execute_PID_Mode(uint8_t motor_id)`

Handles logic for PID control using actual feedback and setpoint.

## 🔍 Feedback & Monitoring

### `int16_t Read_Motor_Feedback(uint8_t motor_id)`

Returns current speed or encoder feedback.

### `void Update_Motor_Status(uint8_t motor_id)`

Updates motor status word and error code.

## 🧠 System Tasks (FreeRTOS)

### `void Task_Modbus_Handler(void *params)`

Handles polling of Modbus RTU stack.

### `void Task_Motor_Control(void *params)`

Main task that evaluates mode and updates motor control logic.

### `void Task_Status_Monitor(void *params)`

Periodically updates status registers and handles system-level errors.

## 🔁 Utility & Config

### `void Save_Config_To_Flash(void) (optional)`

Saves Modbus config (slave ID, baudrate) to internal flash.

### `void Load_Config_From_Flash(void)`

Loads saved configuration on boot.