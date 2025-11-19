#include "MotorControl.h"
#include "main.h"
#include "ModbusMap.h"
#include "UartModbus.h"
#include "stm32f1xx_hal.h"
#include "Encoder.h"

// Khởi tạo

MotorRegisterMap_t motor1;
MotorRegisterMap_t motor2;
SystemRegisterMap_t system;

PIDState_t pid_state1;
PIDState_t pid_state2;

uint16_t mapRegisterAddress(uint16_t modbusAddress) {
    // System registers (0x0000-0x0006)
    if (modbusAddress <= 0x0006) {
        return modbusAddress;
    }
    // Motor 1 registers (0x0010-0x001D)
    else if (modbusAddress >= 0x0010 && modbusAddress <= 0x001D) {
        return modbusAddress;
    }
    // Motor 2 registers (0x0020-0x002D)
    else if (modbusAddress >= 0x0020 && modbusAddress <= 0x002D) {
        return modbusAddress;
    }
    // Digital Input registers (0x0030-0x0034)
    else if (modbusAddress >= 0x0030 && modbusAddress <= 0x0034) {
        return modbusAddress;
    }
    // Digital Output registers (0x0040-0x0044)
    else if (modbusAddress >= 0x0040 && modbusAddress <= 0x0044) {
        return modbusAddress;
    }
    
    return modbusAddress; // Return as is if not in any range
}
// Load từ modbus registers
void MotorRegisters_Load(MotorRegisterMap_t* motor, uint16_t base_addr) {
    motor->Control_Mode = g_holdingRegisters[base_addr + 0x00];
    motor->Enable = g_holdingRegisters[base_addr + 0x01];
    motor->Command_Speed = g_holdingRegisters[base_addr + 0x02];
    motor->Actual_Speed = g_holdingRegisters[base_addr + 0x03];
    motor->Direction = g_holdingRegisters[base_addr + 0x04];
    motor->Max_Speed = g_holdingRegisters[base_addr + 0x05];
    motor->Min_Speed = g_holdingRegisters[base_addr + 0x06];
    motor->PID_Kp = g_holdingRegisters[base_addr + 0x07];
    motor->PID_Ki = g_holdingRegisters[base_addr + 0x08];
    motor->PID_Kd = g_holdingRegisters[base_addr + 0x09];
    motor->Max_Acc = g_holdingRegisters[base_addr + 0x0A];
    motor->Max_Dec = g_holdingRegisters[base_addr + 0x0B];
    motor->Status_Word = g_holdingRegisters[base_addr + 0x0C];
    motor->Error_Code = g_holdingRegisters[base_addr + 0x0D];
    motor->Position_Current = g_holdingRegisters[base_addr + 0x0E];
    motor->Position_Target = g_holdingRegisters[base_addr + 0x0F];
}

void SystemRegisters_Load(SystemRegisterMap_t* sys){
    sys->Device_ID = g_holdingRegisters[REG_DEVICE_ID];
    sys->Config_Baudrate = g_holdingRegisters[REG_CONFIG_BAUDRATE];
    sys->Config_Parity = g_holdingRegisters[REG_CONFIG_PARITY];
    sys->Config_Stop_Bit = g_holdingRegisters[REG_CONFIG_STOP_BIT];
    sys->Module_Type = g_holdingRegisters[REG_MODULE_TYPE];
    sys->Firmware_Version = g_holdingRegisters[REG_FIRMWARE_VERSION];
    sys->Hardware_Version = g_holdingRegisters[REG_HARDWARE_VERSION];
    sys->System_Status = g_holdingRegisters[REG_SYSTEM_STATUS];
    sys->System_Error = g_holdingRegisters[REG_SYSTEM_ERROR];
    sys->Reset_Error_Command = g_holdingRegisters[REG_RESET_ERROR_COMMAND];
}

// Save lại vào modbus registers
void MotorRegisters_Save(MotorRegisterMap_t* motor, uint16_t base_addr){
    g_holdingRegisters[base_addr + 0x00] = motor->Control_Mode;
    g_holdingRegisters[base_addr + 0x01] = motor->Enable;
    g_holdingRegisters[base_addr + 0x02] = motor->Command_Speed;
    g_holdingRegisters[base_addr + 0x03] = motor->Actual_Speed;
    g_holdingRegisters[base_addr + 0x04] = motor->Direction;
    g_holdingRegisters[base_addr + 0x05] = motor->Max_Speed;
    g_holdingRegisters[base_addr + 0x06] = motor->Min_Speed;
    g_holdingRegisters[base_addr + 0x07] = motor->PID_Kp;
    g_holdingRegisters[base_addr + 0x08] = motor->PID_Ki;
    g_holdingRegisters[base_addr + 0x09] = motor->PID_Kd;
    g_holdingRegisters[base_addr + 0x0A] = motor->Max_Acc;
    g_holdingRegisters[base_addr + 0x0B] = motor->Max_Dec;
    g_holdingRegisters[base_addr + 0x0C] = motor->Status_Word;
    g_holdingRegisters[base_addr + 0x0D] = motor->Error_Code;
    g_holdingRegisters[base_addr + 0x0E] = motor->Position_Current;
    g_holdingRegisters[base_addr + 0x0F] = motor->Position_Target;
}
void SystemRegisters_Save(SystemRegisterMap_t* sys){
    g_holdingRegisters[REG_DEVICE_ID] = sys->Device_ID;
    g_holdingRegisters[REG_FIRMWARE_VERSION] = sys->Firmware_Version;
    g_holdingRegisters[REG_SYSTEM_STATUS] = sys->System_Status;
    g_holdingRegisters[REG_SYSTEM_ERROR] = sys->System_Error;
    g_holdingRegisters[REG_RESET_ERROR_COMMAND] = sys->Reset_Error_Command;
    g_holdingRegisters[REG_CONFIG_BAUDRATE] = sys->Config_Baudrate;
    g_holdingRegisters[REG_CONFIG_PARITY] = sys->Config_Parity;
    g_holdingRegisters[REG_CONFIG_STOP_BIT] = sys->Config_Stop_Bit;
    g_holdingRegisters[REG_MODULE_TYPE] = sys->Module_Type;
    g_holdingRegisters[REG_HARDWARE_VERSION] = sys->Hardware_Version;
}

// Xử lý logic điều khiển motor
void Motor_ProcessControl(MotorRegisterMap_t* motor){
    Motor_UpdatePosition(motor);
    uint8_t motor_id = (motor == &motor1) ? 1 : 2;
    PIDState_t* pid_state = (motor_id == 1) ? &pid_state1 : &pid_state2;
    
    if(motor->Enable == 1){
        switch(motor->Control_Mode){
            case CONTROL_MODE_ONOFF:
                Motor_HandleOnOff(motor);
                break;
            case CONTROL_MODE_PID:
                Motor_HandlePID(motor);
                break;
            case CONTROL_MODE_POSITION:
                Motor_HandlePosition(motor);
                break;
            case CONTROL_MODE_CALIB:
                Motor_HandleCalib(motor);
                break;
            default:
                break;
        }   
    }
    else if(motor->Enable == 0){
        motor->Status_Word = 0x0000;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0000;
        pid_state->integral = 0.0f;
        pid_state->last_error = 0.0f;
        pid_state->output = 0.0f;
        pid_state->error = 0.0f;
        
        //motor->Direction = IDLE;
        motor->Actual_Speed = 0; // Reset actual speed when disabled       
        if(motor == &motor1) {
            Motor1_OutputPWM(motor, 0);           // Stop PWM with 0% duty
            //Motor1_Set_Direction(IDLE);           // Set direction to IDLE
        } else {
            Motor2_OutputPWM(motor, 0);           // Stop PWM with 0% duty  
            //Motor2_Set_Direction(IDLE);           // Set direction to IDLE
        } 
    }

    if(motor == &motor1) {   
        Motor1_Set_Direction(motor->Direction); 
    } else {
        Motor2_Set_Direction(motor->Direction);
    }
}



void Motor_Set_Mode(MotorRegisterMap_t* motor, uint8_t mode){
    motor->Control_Mode = mode;
}
void Motor_Set_Enable(MotorRegisterMap_t* motor){
    motor->Enable = 1;
}
void Motor_Set_Disable(MotorRegisterMap_t* motor){
    motor->Enable = 0;
    motor->Actual_Speed = 0; // Reset actual speed when disabled
}



void Motor1_Set_Direction(uint8_t direction){
    if(direction == IDLE){
        motor1.Direction = IDLE;
        HAL_GPIO_WritePin(GPIOA, DIR_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, DIR_2_Pin, GPIO_PIN_RESET);
        motor1.Actual_Speed = 0; // Reset actual speed when idle
        
        // ✅ CRITICAL FIX: STOP ALL PWM CHANNELS WHEN IDLE
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
    }else if(direction == FORWARD){
        motor1.Direction = FORWARD;
        HAL_GPIO_WritePin(GPIOA, DIR_1_Pin, GPIO_PIN_SET);
        // HAL_GPIO_WritePin(GPIOB, DIR_2_Pin, GPIO_PIN_RESET);
        // motor1.Actual_Speed = motor1.Command_Speed; // Set actual speed to command speed
    }else if(direction == REVERSE){
        motor1.Direction = REVERSE;
        HAL_GPIO_WritePin(GPIOA, DIR_1_Pin, GPIO_PIN_RESET);
        // HAL_GPIO_WritePin(GPIOB, DIR_2_Pin, GPIO_PIN_SET);
        // motor1.Actual_Speed = motor1.Command_Speed; // Set actual speed to command speed
    }
}
void Motor2_Set_Direction(uint8_t direction){
    if(direction == IDLE){
        motor2.Direction = IDLE;
        HAL_GPIO_WritePin(GPIOA, DIR_3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, DIR_4_Pin, GPIO_PIN_RESET);
        motor2.Actual_Speed = 0; // Reset actual speed when idle
        
        // ✅ CRITICAL FIX: STOP ALL PWM CHANNELS WHEN IDLE
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
    }else if(direction == FORWARD){
        motor2.Direction = FORWARD;
        HAL_GPIO_WritePin(GPIOA, DIR_3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, DIR_4_Pin, GPIO_PIN_RESET);
        // motor2.Actual_Speed = motor2.Command_Speed; // Set actual speed to command speed
    }else if(direction == REVERSE){
        motor2.Direction = REVERSE;
        HAL_GPIO_WritePin(GPIOA, DIR_3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, DIR_4_Pin, GPIO_PIN_SET);
        // motor2.Actual_Speed = motor2.Command_Speed; // Set actual speed to command speed
    }
}

void Motor_Set_Speed(MotorRegisterMap_t* motor, uint8_t speed){
    motor->Command_Speed = speed;
    if(motor->Enable && motor->Direction != IDLE) {
        motor->Actual_Speed = speed; // Update actual speed when enabled and not idle
    }
}


void Motor_Set_PID_Kp(MotorRegisterMap_t* motor, uint8_t kp){
    motor->PID_Kp = kp;
}
void Motor_Set_PID_Ki(MotorRegisterMap_t* motor, uint8_t ki){
    motor->PID_Ki = ki;
}
void Motor_Set_PID_Kd(MotorRegisterMap_t* motor, uint8_t kd){
    motor->PID_Kd = kd;
}



// Xử lý ON/OFF mode (mode 1)
uint8_t Motor_HandleOnOff(MotorRegisterMap_t* motor) {
    uint8_t duty = 0;
    uint8_t motor_id = (motor == &motor1) ? 1 : 2;
    
    if(motor->Enable == 1 && motor->Direction != IDLE) {
        motor->Status_Word = 0x0001;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0001;
        // Xuất PWM theo tốc độ đặt
        duty = motor->Command_Speed;
        motor->Actual_Speed = duty; // Update actual speed in ON/OFF mode
        
        // ✅ CRITICAL FIX: OUTPUT PWM WHEN ENABLED
        if(motor_id == 1) {
            Motor1_OutputPWM(motor, duty);
        } else {
            Motor2_OutputPWM(motor, duty);
        }
    } else {
        motor->Status_Word = 0x0000;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0000;
        motor->Direction = IDLE;
        motor->Actual_Speed = 0;
        duty = 0;
        
        // ✅ CRITICAL FIX: STOP PWM WHEN DISABLED OR IDLE
        if(motor_id == 1) {
            Motor1_OutputPWM(motor, 0);
            Motor1_Set_Direction(IDLE);
        } else {
            Motor2_OutputPWM(motor, 0);
            Motor2_Set_Direction(IDLE);
        }
    }
    
    return duty;
}


// Function to simulate actual speed measurement (replace with real encoder reading)
uint8_t getActualSpeed(uint8_t motor_id) {
    // For now, simulate speed based on PWM duty with some delay/filtering
    // In real implementation, this should read from encoder or current sensor
    static uint8_t simulated_speed1 = 0;
    static uint8_t simulated_speed2 = 0;
    
    if (motor_id == 1) {
        // Simple first-order filter to simulate motor response
        uint8_t target_speed = motor1.Command_Speed;
        if (simulated_speed1 < target_speed) {
            simulated_speed1 += (target_speed > simulated_speed1 + 2) ? 2 : (target_speed - simulated_speed1);
        } else if (simulated_speed1 > target_speed) {
            simulated_speed1 -= (simulated_speed1 > target_speed + 2) ? 2 : (simulated_speed1 - target_speed);
        }
        return simulated_speed1;
    } else {
        uint8_t target_speed = motor2.Command_Speed;
        if (simulated_speed2 < target_speed) {
            simulated_speed2 += (target_speed > simulated_speed2 + 2) ? 2 : (target_speed - simulated_speed2);
        } else if (simulated_speed2 > target_speed) {
            simulated_speed2 -= (simulated_speed2 > target_speed + 2) ? 2 : (simulated_speed2 - target_speed);
        }
        return simulated_speed2;
    }
}

// Xử lý PID mode (mode 3)
uint8_t Motor_HandlePID(MotorRegisterMap_t* motor) {
    uint8_t motor_id = (motor == &motor1) ? 1 : 2;
    PIDState_t* pid_state = (motor_id == 1) ? &pid_state1 : &pid_state2;

    // Check enable & mode
    if (motor->Enable == 0 || motor->Control_Mode != CONTROL_MODE_PID || motor->Direction == IDLE) {
        // Reset PID state
        pid_state->integral = 0.0f;
        pid_state->last_error = 0.0f;
        pid_state->output = 0.0f;
        pid_state->error = 0.0f;
        pid_state->simulated_output = 0.0f;
        
        // Reset actual speed when disabled
        motor->Actual_Speed = 0;
        
        // Disable motor output
        if (motor_id == 1) {
            pid_state1.simulated_output = 0;
            Motor1_OutputPWM(motor, 0);
            Motor1_Set_Direction(DIRECTION_IDLE);
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        } else {
            pid_state2.simulated_output = 0;
            Motor2_OutputPWM(motor, 0);
            Motor2_Set_Direction(DIRECTION_IDLE);
        }
        return 0;
    }


    // Update acceleration limit from motor settings
    pid_state->acceleration_limit = (float)motor->Max_Acc;

    // Compute PID with REAL feedback
    float output = PID_Compute(motor_id, (float)motor->Command_Speed, (float)motor->Actual_Speed);
    
    motor->Actual_Speed = output;

    // Convert to PWM duty (0-100%)
    uint8_t duty = (uint8_t)output;
    
    // Clamp duty to max/min speed limits
    if (duty > motor->Max_Speed) duty = motor->Max_Speed;
    if (duty < motor->Min_Speed && duty > 0) duty = motor->Min_Speed;
    duty = duty * 0.98;
    // Update motor outputs
    if (motor_id == 1) {
        Motor1_OutputPWM(motor, duty);
    } else {
        Motor2_OutputPWM(motor, duty);
    }

    return duty;
}

uint8_t Motor_HandlePosition(MotorRegisterMap_t* motor){
    uint8_t motor_id = (motor == &motor1) ? 1 : 2;
    PIDState_t* pid_state = (motor_id == 1) ? &pid_state1 : &pid_state2;

    // Check enable & mode
    if (motor->Enable == 0 || motor->Control_Mode != CONTROL_MODE_POSITION || motor->Direction == IDLE) {
        // Reset PID state
        pid_state->integral = 0.0f;
        pid_state->last_error = 0.0f;
        pid_state->output = 0.0f;
        pid_state->error = 0.0f;
        
        // Reset actual speed when disabled
        motor->Actual_Speed = 0;
        
        // Disable motor output
        if (motor_id == 1) {
            Motor1_OutputPWM(motor, 0);
            Motor1_Set_Direction(DIRECTION_IDLE);
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        } else {
            Motor2_OutputPWM(motor, 0);
            Motor2_Set_Direction(DIRECTION_IDLE);
        }
        return 0;
    }

    // Get current position from encoder (in cm)
    uint16_t current_position = Encoder_MeasureLength(&encoder1);
    
    // Get target position from motor registers (in cm)
    uint16_t target_position = motor->Position_Target;
    
    // Calculate position error (in cm)
    int16_t position_error = (int16_t)target_position - (int16_t)current_position;
    
    // Calculate absolute position error
    uint16_t abs_position_error = (position_error > 0) ? position_error : -position_error;
    
    // Check if at target position (within tolerance)
    if (abs_position_error <= 1) {
        // At target position - stop motor
        motor->Direction = DIRECTION_IDLE;
        motor->Actual_Speed = 0;
        if (motor_id == 1) {
            Motor1_OutputPWM(motor, 0);
            Motor1_Set_Direction(DIRECTION_IDLE);
        } else {
            Motor2_OutputPWM(motor, 0);
            Motor2_Set_Direction(DIRECTION_IDLE);
        }
        return 0;
    }
    
    // Determine direction based on position error
    if (position_error > 0) {
        // Need to move forward (unroll wire)
        motor->Direction = DIRECTION_FORWARD;
        if (motor_id == 1) {
            Motor1_Set_Direction(DIRECTION_FORWARD);
        } else {
            Motor2_Set_Direction(DIRECTION_FORWARD);
        }
    } else {
        // Need to move reverse (roll wire)
        motor->Direction = DIRECTION_REVERSE;
        if (motor_id == 1) {
            Motor1_Set_Direction(DIRECTION_REVERSE);
        } else {
            Motor2_Set_Direction(DIRECTION_REVERSE);
        }
    }
    
    // Target speed is Command_Speed (configured speed %)
    float target_speed = (float)motor->Command_Speed;
    
    // Update acceleration limit from motor settings
    pid_state->acceleration_limit = (float)motor->Max_Acc;

    // Compute PID with position error as feedback
    // When far from target (large position error), output approaches target_speed
    // When close to target (small position error), output decreases
    float output = PID_Compute(motor_id, target_speed, (float)abs_position_error);
    
    motor->Actual_Speed = (uint8_t)output;

    // Convert to PWM duty (0-100%)
    uint8_t duty = (uint8_t)output;
    
    // Clamp duty to max/min speed limits
    if (duty > motor->Max_Speed) duty = motor->Max_Speed;
    if (duty < motor->Min_Speed && duty > 0) duty = motor->Min_Speed;
    duty = duty * 0.98;
    
    // Update motor outputs
    if (motor_id == 1) {
        Motor1_OutputPWM(motor, duty);
    } else {
        Motor2_OutputPWM(motor, duty);
    }

    return duty;
}



uint8_t Motor_HandleCalib(MotorRegisterMap_t* motor){
    uint8_t duty = 0;
    uint8_t motor_id = (motor == &motor1) ? 1 : 2;
    
    if(motor->Enable == 1 && motor->Direction != IDLE) {
        motor->Status_Word = 0x0001;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0001;
        // Xuất PWM theo tốc độ đặt
        duty = motor->Command_Speed;
        // Chu trình calibration
        static uint8_t calib_state = 0;
        static uint32_t calib_previousTick = 0;
        
        switch(calib_state) {
            case 0: // Bắt đầu - Di chuyển về vị trí gốc (REVERSE)
                motor->Direction = REVERSE;
                duty = motor->Command_Speed * 0.98;
                
                // Kiểm tra sensor gốc
                if(encoder1.Calib_Origin_Status == true) {
                    Encoder_Reset(&encoder1);
                    calib_state = 1;
                    calib_previousTick = osKernelGetTickCount();
                }
                break;
                
            case 1: // Dừng tại vị trí gốc
                motor->Direction = IDLE;
                duty = 0;
                
                // Chờ 500ms để ổn định
                if(osKernelGetTickCount() >= calib_previousTick + 500) {
                    calib_state = 2;
                    calib_previousTick = osKernelGetTickCount();
                }
                break;
                
            case 2: // Xả dây ra (FORWARD) theo khoảng cách calib
                motor->Direction = FORWARD;
                duty = motor->Command_Speed * 0.98;
                
                // Kiểm tra đã đạt khoảng cách calib chưa
                uint16_t current_length = Encoder_MeasureLength(&encoder1);
                if(current_length >= encoder1.Encoder_Calib_Length_CM_Max) {
                    calib_state = 3;
                    calib_previousTick = osKernelGetTickCount();
                }
                break;
                
            case 3: // Dừng sau khi đạt khoảng cách calib
                motor->Direction = IDLE;
                duty = 0;
                
                // Chờ 500ms để ổn định
                if(osKernelGetTickCount() >= calib_previousTick + 500) {
                    encoder1.Encoder_Calib_Status = 1; // Đánh dấu hoàn thành
                    uint16_t current_length = Encoder_MeasureLength(&encoder1);
                    encoder1.Encoder_Calib_Length_CM_Max = current_length;
                    calib_state = 4;
                    calib_previousTick = osKernelGetTickCount();
                }
                break;
                
            case 4: // Quay về vị trí gốc (REVERSE)
                motor->Direction = REVERSE;
                duty = motor->Command_Speed * 0.98;
                
                // Kiểm tra sensor gốc
                if(encoder1.Calib_Origin_Status == true) {
                    Encoder_Reset(&encoder1);
                    calib_state = 5;
                    calib_previousTick = osKernelGetTickCount();
                }
                break;
                
            case 5: // Hoàn thành calibration
                motor->Direction = IDLE;
                duty = 0;
                
                // Reset về trạng thái ban đầu sau 1s
                if(osKernelGetTickCount() >= calib_previousTick + 1000) {
                    calib_state = 0;
                    motor->Enable = 0; // Disable motor
                    motor->Control_Mode = 1; // ONOFF mode
                }
                break;
                
            default:
                calib_state = 0;
                break;
        }
        
        motor->Actual_Speed = duty;
        
        if(motor_id == 1) {
            Motor1_OutputPWM(motor, duty);
            Motor1_Set_Direction(motor->Direction);
        } else {
            Motor2_OutputPWM(motor, duty);
            Motor2_Set_Direction(motor->Direction);
        }
    } else {
        motor->Status_Word = 0x0000;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0000;
        motor->Direction = IDLE;
        motor->Actual_Speed = 0;
        duty = 0;
        if(motor_id == 1) {
            Motor1_OutputPWM(motor, 0);
            Motor1_Set_Direction(IDLE);
        } else {
            Motor2_OutputPWM(motor, 0);
            Motor2_Set_Direction(IDLE);
        }
    }
    
    return duty;
}
// Gửi tín hiệu PWM dựa vào Actual_Speed
void Motor1_OutputPWM(MotorRegisterMap_t* motor, uint8_t duty_percent){
    // Chuyển % thành giá trị phù hợp với Timer (0 - TIM_ARR)
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim3);
    uint32_t ccr = duty_percent * arr / 100;
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr);
    // if(motor->Direction == FORWARD){
    //     HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
    //     HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr);
    // }else if(motor->Direction == REVERSE){
    //     HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    //     HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    //     __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, ccr);
    // }
}

void Motor2_OutputPWM(MotorRegisterMap_t* motor, uint8_t duty_percent){
    // Chuyển % thành giá trị phù hợp với Timer (0 - TIM_ARR)
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t ccr = duty_percent * arr / 100;
    
    if(motor->Direction == FORWARD){
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
    }else if(motor->Direction == REVERSE){
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr);
    }

    
}

// Điều khiển chiều quay motor


// Khởi tạo giá trị PID cho từng motor
// void PID_Init(MotorRegisterMap_t* motor, float kp, float ki, float kd){

// Khởi tạo giá trị PID cho từng motor
void PID_Init(uint8_t motor_id, float kp, float ki, float kd) {
    PIDState_t* pid_state = (motor_id == 1) ? &pid_state1 : &pid_state2;
    
    // Reset all state variables
    pid_state->integral = 0.0f;
    pid_state->last_error = 0.0f;
    pid_state->output = 0.0f;
    pid_state->error = 0.0f;
    
    // Set limits
    pid_state->max_integral = 1000.0f;  // Anti-windup limit
    pid_state->acceleration_limit = 10.0f;  // Limit rate of change
    pid_state->max_output = 100.0f;  // Maximum PWM duty cycle
    
    // Set PID gains
    MotorRegisterMap_t* motor = (motor_id == 1) ? &motor1 : &motor2;
    motor->PID_Kp = kp;
    motor->PID_Ki = ki;
    motor->PID_Kd = kd;
}

// Tính toán PID mỗi chu kỳ - trả về duty % (0-100)
float PID_Compute(uint8_t motor_id, float setpoint, float feedback) {
    // Get correct motor and PID state
    MotorRegisterMap_t* motor = (motor_id == 1) ? &motor1 : &motor2;
    PIDState_t* pid_state = (motor_id == 1) ? &pid_state1 : &pid_state2;
    
    // Get sample time in seconds (motor task runs every 10ms)
    const float SAMPLE_TIME = 0.01f; // 10ms = 0.01s
    
    // Calculate error
    pid_state->error = setpoint - feedback;
    
    // ✅ CRITICAL FIX: Scale PID gains properly (×100 according to modbus_map.md)
    float kp = (float)motor->PID_Kp / 100.0f;  // Scale down from ×100
    float ki = (float)motor->PID_Ki / 100.0f;  // Scale down from ×100  
    float kd = (float)motor->PID_Kd / 100.0f;  // Scale down from ×100
    
    // Proportional term
    float p_term = kp * pid_state->error;
    
    // Integral term with proper time scaling and anti-windup
    pid_state->integral += pid_state->error * SAMPLE_TIME;
    
    // Anti-windup: limit integral based on max output
    float max_integral = (ki != 0) ? (pid_state->max_output / ki) : pid_state->max_integral;
    if (pid_state->integral > max_integral) {
        pid_state->integral = max_integral;
    } else if (pid_state->integral < -max_integral) {
        pid_state->integral = -max_integral;
    }
    float i_term = ki * pid_state->integral;
    
    // Derivative term with proper time scaling
    float derivative = (pid_state->error - pid_state->last_error) / SAMPLE_TIME;
    
    // Simple derivative filter to reduce noise
    static float filtered_derivative1 = 0;
    static float filtered_derivative2 = 0;
    float* filtered_d = (motor_id == 1) ? &filtered_derivative1 : &filtered_derivative2;
    
    const float FILTER_ALPHA = 0.1f; // Low-pass filter coefficient
    *filtered_d = (FILTER_ALPHA * derivative) + ((1.0f - FILTER_ALPHA) * (*filtered_d));
    
    float d_term = kd * (*filtered_d);
    pid_state->last_error = pid_state->error;
    
    // Calculate raw output
    float raw_output = p_term + i_term + d_term;
    
    // Apply rate limiting (acceleration limit per second)
    float max_rate_change = pid_state->acceleration_limit * SAMPLE_TIME;
    float output_change = raw_output - pid_state->output;
    if (output_change > max_rate_change) {
        raw_output = pid_state->output + max_rate_change;
    } else if (output_change < -max_rate_change) {
        raw_output = pid_state->output - max_rate_change;
    }
    
    // Apply output limits (0-100%)
    if (raw_output > pid_state->max_output) {
        raw_output = pid_state->max_output;
    } else if (raw_output < 0.0f) {
        raw_output = 0.0f;
    }
    
    // Update and return output
    pid_state->output = raw_output;
    return raw_output;
}

void Motor_UpdatePosition(MotorRegisterMap_t* motor){
    motor->Position_Current = encoder1.Encoder_Calib_Current_Length_CM;
}

// Reset các lỗi nếu có
void Motor_ResetError(MotorRegisterMap_t* motor){

}

// Kiểm tra và xử lý các điều kiện lỗi (overcurrent, timeout,...)
void Motor_CheckError(MotorRegisterMap_t* motor){

}

void Motor_DebugPrint(const MotorRegisterMap_t* motor, const char* name){

}
void System_DebugPrint(const SystemRegisterMap_t* sys){
    // This can be implemented to output system status via UART if needed
}

// Debug function to monitor PID values (call this periodically if needed)
void PID_DebugPrint(uint8_t motor_id) {
    MotorRegisterMap_t* motor = (motor_id == 1) ? &motor1 : &motor2;
    PIDState_t* pid_state = (motor_id == 1) ? &pid_state1 : &pid_state2;
    
    // Store debug values in holding registers for Modbus monitoring
    // You can read these via Modbus to monitor PID performance
    if (motor_id == 1) {
        // Use some unused registers for debug (example addresses)
        g_holdingRegisters[0x00E0] = (uint16_t)(pid_state->error * 10);       // Error x10
        g_holdingRegisters[0x00E1] = (uint16_t)(pid_state->integral * 10);    // Integral x10  
        g_holdingRegisters[0x00E2] = (uint16_t)(pid_state->output);           // PID Output
        g_holdingRegisters[0x00E3] = motor->Command_Speed;                    // Setpoint
        g_holdingRegisters[0x00E4] = motor->Actual_Speed;                     // Feedback
    } else {
        g_holdingRegisters[0x00E5] = (uint16_t)(pid_state->error * 10);       // Error x10
        g_holdingRegisters[0x00E6] = (uint16_t)(pid_state->integral * 10);    // Integral x10
        g_holdingRegisters[0x00E7] = (uint16_t)(pid_state->output);           // PID Output  
        g_holdingRegisters[0x00E8] = motor->Command_Speed;                    // Setpoint
        g_holdingRegisters[0x00E9] = motor->Actual_Speed;                     // Feedback
    }
}
void System_ResetSystem(void){
    initializeModbusRegisters();

    MotorRegisters_Load(&motor1, 0x0000);
    MotorRegisters_Load(&motor2, 0x0010);
    SystemRegisters_Load(&system);
    PID_Init(1, DEFAULT_PID_KP, DEFAULT_PID_KI, DEFAULT_PID_KD); // Motor 1
    PID_Init(2, DEFAULT_PID_KP, DEFAULT_PID_KI, DEFAULT_PID_KD); // Motor 2
}