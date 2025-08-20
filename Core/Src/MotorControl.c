#include "MotorControl.h"
#include "main.h"
#include "ModbusMap.h"
#include "UartModbus.h"
#include "stm32f1xx_hal.h"

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
}

void SystemRegisters_Load(SystemRegisterMap_t* sys, uint16_t base_addr){
    sys->Device_ID = g_holdingRegisters[base_addr + 0];
    sys->Firmware_Version = g_holdingRegisters[base_addr + 1];
    sys->System_Status = g_holdingRegisters[base_addr + 2];
    sys->System_Error = g_holdingRegisters[base_addr + 3];
    sys->Reset_Error_Command = g_holdingRegisters[base_addr + 4];
    sys->Config_Baudrate = g_holdingRegisters[base_addr + 5];
    sys->Config_Parity = g_holdingRegisters[base_addr + 6];
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
}
void SystemRegisters_Save(SystemRegisterMap_t* sys, uint16_t base_addr){
    g_holdingRegisters[base_addr + 0] = sys->Device_ID;
    g_holdingRegisters[base_addr + 1] = sys->Firmware_Version;
    g_holdingRegisters[base_addr + 2] = sys->System_Status;
    g_holdingRegisters[base_addr + 3] = sys->System_Error;
    g_holdingRegisters[base_addr + 4] = sys->Reset_Error_Command;
    g_holdingRegisters[base_addr + 5] = sys->Config_Baudrate;
    g_holdingRegisters[base_addr + 6] = sys->Config_Parity;
}

// Xử lý logic điều khiển motor
void Motor_ProcessControl(MotorRegisterMap_t* motor){
    if(motor->Enable == 1){
        switch(motor->Control_Mode){
            case CONTROL_MODE_ONOFF:
                Motor_HandleOnOff(motor);
                break;
            case CONTROL_MODE_PID:
                Motor_HandlePID(motor);
                break;

            default:
                break;
        }   
    }
    else if(motor->Enable == 0){
        motor->Status_Word = 0x0000;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0000;
        motor->Direction = IDLE;
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
}



void Motor1_Set_Direction(uint8_t direction){
    motor1.Direction = direction;
    if(direction == FORWARD){
        HAL_GPIO_WritePin(GPIOA, DIR_1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, DIR_2_Pin, GPIO_PIN_RESET);
    }else if(direction == REVERSE){
        HAL_GPIO_WritePin(GPIOA, DIR_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, DIR_2_Pin, GPIO_PIN_SET);
    }else if(direction == IDLE){
        HAL_GPIO_WritePin(GPIOA, DIR_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, DIR_2_Pin, GPIO_PIN_RESET);
    }
}
void Motor2_Set_Direction(uint8_t direction){
    motor2.Direction = direction;
    if(direction == FORWARD){
        HAL_GPIO_WritePin(GPIOA, DIR_3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, DIR_4_Pin, GPIO_PIN_RESET);
    }else if(direction == REVERSE){
        HAL_GPIO_WritePin(GPIOA, DIR_3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, DIR_4_Pin, GPIO_PIN_SET);
    }else if(direction == IDLE){
        HAL_GPIO_WritePin(GPIOA, DIR_3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, DIR_4_Pin, GPIO_PIN_RESET);
    }
}

void Motor_Set_Speed(MotorRegisterMap_t* motor, uint8_t speed){
    motor->Command_Speed = speed;

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
    
    if(motor->Enable == 1) {
        motor->Status_Word = 0x0001;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0001;
        // Xuất PWM theo tốc độ đặt
    } else {
        motor->Status_Word = 0x0000;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0000;
        motor->Direction = IDLE;
        duty = 0;
    }
    
    return duty;
}

// Xử lý LINEAR mode (mode 2)
// Xử lý PID mode (mode 3)
uint8_t Motor_HandlePID(MotorRegisterMap_t* motor) {
    uint8_t motor_id = (motor == &motor1) ? 1 : 2;
    PIDState_t* pid_state = (motor_id == 1) ? &pid_state1 : &pid_state2;

    // Check enable & mode
    if (motor->Enable == 0 || motor->Control_Mode != CONTROL_MODE_PID) {
        // Reset PID state
        pid_state->integral = 0.0f;
        pid_state->last_error = 0.0f;
        pid_state->output = 0.0f;
        pid_state->error = 0.0f;
        
        // Disable motor output
        if (motor_id == 1) {
            Motor1_OutputPWM(motor, 0);
        } else {
            Motor2_OutputPWM(motor, 0);
        }
        return 0;
    }

    // Update acceleration limit from motor settings
    pid_state->acceleration_limit = (float)motor->Max_Acc;

    // Compute PID with current speed as feedback
    float output = PID_Compute(motor_id, (float)motor->Command_Speed, (float)motor->Actual_Speed);
    
    // Convert to PWM duty (0-100%)
    uint8_t duty = (uint8_t)output;
    motor->Actual_Speed = duty;
    
    // Update motor outputs
    if (motor_id == 1) {
        Motor1_OutputPWM(motor, duty);
    } else {
        Motor2_OutputPWM(motor, duty);
    }

    return duty;
}


// Gửi tín hiệu PWM dựa vào Actual_Speed
void Motor1_OutputPWM(MotorRegisterMap_t* motor, uint8_t duty_percent){
    // Chuyển % thành giá trị phù hợp với Timer (0 - TIM_ARR)
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t ccr = duty_percent * arr / 100;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
}  // motor_id = 1 hoặc 2  {}

void Motor2_OutputPWM(MotorRegisterMap_t* motor, uint8_t duty_percent){
    // Chuyển % thành giá trị phù hợp với Timer (0 - TIM_ARR)
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t ccr = duty_percent * arr / 100;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr);
}  // motor_id = 1 hoặc 2  {}

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
    
    // Calculate error
    pid_state->error = setpoint - feedback;
    
    // Proportional term
    float p_term = motor->PID_Kp * pid_state->error;
    
    // Integral term with anti-windup
    pid_state->integral += pid_state->error;
    if (pid_state->integral > pid_state->max_integral) {
        pid_state->integral = pid_state->max_integral;
    } else if (pid_state->integral < -pid_state->max_integral) {
        pid_state->integral = -pid_state->max_integral;
    }
    float i_term = motor->PID_Ki * pid_state->integral;
    
    // Derivative term
    float derivative = pid_state->error - pid_state->last_error;
    float d_term = motor->PID_Kd * derivative;
    pid_state->last_error = pid_state->error;
    
    // Calculate raw output
    float raw_output = p_term + i_term + d_term;
    
    // Apply acceleration limit
    float output_change = raw_output - pid_state->output;
    if (output_change > pid_state->acceleration_limit) {
        raw_output = pid_state->output + pid_state->acceleration_limit;
    } else if (output_change < -pid_state->acceleration_limit) {
        raw_output = pid_state->output - pid_state->acceleration_limit;
    }
    
    // Apply output limits
    if (raw_output > pid_state->max_output) {
        raw_output = pid_state->max_output;
    } else if (raw_output < 0.0f) {
        raw_output = 0.0f;
    }
    
    // Update and return output
    pid_state->output = raw_output;
    return raw_output;
}

// Reset các lỗi nếu có
void Motor_ResetError(MotorRegisterMap_t* motor){

}

// Kiểm tra và xử lý các điều kiện lỗi (overcurrent, timeout,...)
void Motor_CheckError(MotorRegisterMap_t* motor){

}

// Debug/log
void Motor_DebugPrint(const MotorRegisterMap_t* motor, const char* name){

}
void System_DebugPrint(const SystemRegisterMap_t* sys){

}
