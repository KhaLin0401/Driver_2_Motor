#include "MotorControl.h"
#include "main.h"
#include "ModbusMap.h"

// Khởi tạo
void SystemRegisters_Init(SystemRegisterMap_t* sys){
    sys->Device_ID = 0x0003;
    sys->Firmware_Version = 0x0101;
    sys->System_Status = 0x0000;
    sys->System_Error = 0x0000;
    sys->Reset_Error_Command = 0;
    sys->Config_Baudrate = 1;
    sys->Config_Parity = 0;
}
void MotorRegisters_Init(MotorRegisterMap_t* motor){
    motor->Control_Mode = 1;
    motor->Enable = 0;
    motor->Command_Speed = 0;
    motor->Linear_Input = 0;
    motor->Linear_Unit = 5;
    motor->Linear_State = 0;
    motor->Actual_Speed = 0;
    motor->Direction = 0;
    motor->PID_Kp = 100;
    motor->PID_Ki = 10;
    motor->PID_Kd = 5;
    motor->Status_Word = 0x0000;
    motor->Error_Code = 0;
}

MotorRegisterMap_t motor1;
MotorRegisterMap_t motor2;
SystemRegisterMap_t system;

// Load từ modbus registers
void MotorRegisters_Load(MotorRegisterMap_t* motor, uint16_t base_addr){
    motor->Control_Mode = g_holdingRegisters[base_addr + 0];
    motor->Enable = g_holdingRegisters[base_addr + 1];
    motor->Command_Speed = g_holdingRegisters[base_addr + 2];
    motor->Linear_Input = g_holdingRegisters[base_addr + 3];
    motor->Linear_Unit = g_holdingRegisters[base_addr + 4];
    motor->Linear_State = g_holdingRegisters[base_addr + 5];
    motor->Actual_Speed = g_holdingRegisters[base_addr + 6];
    motor->Direction = g_holdingRegisters[base_addr + 7];
    motor->PID_Kp = g_holdingRegisters[base_addr + 8];
    motor->PID_Ki = g_holdingRegisters[base_addr + 9];
    motor->PID_Kd = g_holdingRegisters[base_addr + 10];
    motor->Status_Word = g_holdingRegisters[base_addr + 11];
    motor->Error_Code = g_holdingRegisters[base_addr + 12];
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
    g_holdingRegisters[base_addr + 0] = motor->Control_Mode;
    g_holdingRegisters[base_addr + 1] = motor->Enable;
    g_holdingRegisters[base_addr + 2] = motor->Command_Speed;
    g_holdingRegisters[base_addr + 3] = motor->Linear_Input;
    g_holdingRegisters[base_addr + 4] = motor->Linear_Unit;
    g_holdingRegisters[base_addr + 5] = motor->Linear_State;
    g_holdingRegisters[base_addr + 6] = motor->Actual_Speed;
    g_holdingRegisters[base_addr + 7] = motor->Direction;
    g_holdingRegisters[base_addr + 8] = motor->PID_Kp;
    g_holdingRegisters[base_addr + 9] = motor->PID_Ki;
    g_holdingRegisters[base_addr + 10] = motor->PID_Kd;
    g_holdingRegisters[base_addr + 11] = motor->Status_Word;
    g_holdingRegisters[base_addr + 12] = motor->Error_Code;
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

    switch(motor->Control_Mode){
        case CONTROL_MODE_ONOFF:
            Motor_HandleOnOff(motor);
            break;
        case CONTROL_MODE_LINEAR:
            Motor_HandleLinear(motor);
        case CONTROL_MODE_PID:
            Motor_HandlePID(motor);
            break;
        default:
            break;
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

void Motor_Set_Linear_Input(MotorRegisterMap_t* motor, uint8_t input){
    motor->Linear_Input = input;
    
}

void Motor_Set_Linear_Unit(MotorRegisterMap_t* motor, uint8_t unit){
    motor->Linear_Unit = unit;
}

void Motor_Set_Linear_State(MotorRegisterMap_t* motor, uint8_t state){
    motor->Linear_State = state;
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
void Motor_HandleOnOff(MotorRegisterMap_t* motor){
    if(motor->Enable == 1){
        motor->Status_Word = 0x0001;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0001;
        // Hàm xuất PWM cho động cơ chạy
        
    }else{
        motor->Status_Word = 0x0000;
        g_holdingRegisters[REG_M1_STATUS_WORD] = 0x0001;
        motor->Direction = IDLE;
    }

}

// Xử lý LINEAR mode (mode 2)
void Motor_HandleLinear(MotorRegisterMap_t* motor){
    if (motor->Actual_Speed < motor->Linear_Input){
        if (motor->Linear_State == 1){
    // Giới hạn tốc độ đặt trong khoảng hợp lệ (giả sử 0 - 1000)
        if (motor->Actual_Speed + motor->Linear_Unit > motor->Linear_Input){
            motor->Actual_Speed = motor->Linear_Input;
        }else{
            motor->Actual_Speed += motor->Linear_Unit;
        }
    } else if (motor->Actual_Speed > motor->Linear_Input){
        if (motor->Linear_State == 0){
        if (motor->Actual_Speed - motor->Linear_Unit < motor->Linear_Input){
            motor->Actual_Speed = motor->Linear_Input;
        }else{
            motor->Actual_Speed -= motor->Linear_Unit;
        }
    }
    // Tính toán duty cycle PWM dựa vào tốc độ đặt
    uint16_t duty = motor->Actual_Speed * 100 / 1000;  // kết quả: 0 - 100%

    // Xuất PWM
    Motor1_OutputPWM(motor, duty);

}

// Xử lý PID mode (mode 3)
void Motor_HandlePID(MotorRegisterMap_t* motor){

}

// Gửi tín hiệu PWM dựa vào Actual_Speed
void Motor1_OutputPWM(uint8_t motor_id, uint8_t duty_percent){
    // Chuyển % thành giá trị phù hợp với Timer (0 - TIM_ARR)
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t ccr = duty_percent * arr / 100;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
}  // motor_id = 1 hoặc 2  {}

void Motor2_OutputPWM(uint8_t motor_id, uint8_t speed){

}

// Điều khiển chiều quay motor
void Motor_SetDirection(uint8_t motor_id, uint8_t direction){

    
}  // 0=Idle, 1=Forward, 2=Reverse

// Khởi tạo giá trị PID cho từng motor
void PID_Init(uint8_t motor_id, float kp, float ki, float kd){

}

// Tính toán PID mỗi chu kỳ
float PID_Compute(uint8_t motor_id, float setpoint, float feedback){

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