#include "Encoder.h"
#include "ModbusMap.h"
#include "main.h"
#include "MotorControl.h"

#include <math.h>

#define ENCODER_PPR 4       // 4 pulses per revolution (quadrature encoding)
#define WIRE_LENGTH_CM 300 // total wire length in cm
#define R_MAX 35.0f         // Maximum radius (full spool) in mm
#define R_MIN 10.0f         // Minimum radius (empty spool/core) in mm

// Static variables for wire length calculation
static float wire_unrolled = 0.0f;  // Current wire length unrolled in mm
static float current_radius = R_MAX; // Current effective radius in mm
static uint16_t last_encoder_count = 0; // Last encoder count for delta calculation

Encoder_t encoder1;
//Encoder_t encoder2;

void Encoder_Init(void){
    encoder1.Calib_Origin_Status = false;
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1 | TIM_CHANNEL_2);
    __HAL_TIM_SET_COUNTER(&htim2, 0);   // Reset counter về 0
    encoder1.Encoder_Count = 0; // Initialize encoder count
    encoder1.Encoder_Config = 100;
    encoder1.Encoder_Reset = 0;
    encoder1.Encoder_Calib_Sensor_Status = 0;
    encoder1.Encoder_Calib_Length_CM_Max = WIRE_LENGTH_CM;
    encoder1.Encoder_Calib_Start = 0;
    encoder1.Encoder_Calib_Status = 0;
    encoder1.Encoder_Calib_Current_Length_CM = 0;
    
    // Initialize wire length tracking
    wire_unrolled = 0.0f;
    current_radius = R_MAX;
    last_encoder_count = 0;
}

void Encoder_Read(Encoder_t* encoder){
    if(encoder->Encoder_Reset == true){
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        encoder->Encoder_Count = 0;
    }
    encoder->Encoder_Count = __HAL_TIM_GET_COUNTER(&htim2);
}

void Encoder_Write(Encoder_t* encoder, uint16_t value){
    encoder->Encoder_Calib_Current_Length_CM = value;
}

void Encoder_Reset(Encoder_t* encoder){
    encoder->Encoder_Count = __HAL_TIM_SET_COUNTER(&htim2, 0);
    encoder->Encoder_Calib_Current_Length_CM = 0;
}

void Encoder_Load(Encoder_t* encoder){
    encoder->Encoder_Count = g_holdingRegisters[REG_M1_ENCODER_COUNT];
    encoder->Encoder_Config = g_holdingRegisters[REG_M1_ENCODER_CONFIG];
    encoder->Encoder_Reset = g_holdingRegisters[REG_M1_ENCODER_RESET];
    encoder->Encoder_Calib_Sensor_Status = g_holdingRegisters[REG_M1_CALIB_SENSOR_STATUS];
    encoder->Encoder_Calib_Length_CM_Max = g_holdingRegisters[REG_M1_CALIB_DISTANCE_CM];
    encoder->Encoder_Calib_Start = g_holdingRegisters[REG_M1_ENCODER_RESET];
    encoder->Encoder_Calib_Status = 0;
    encoder->Encoder_Calib_Current_Length_CM = g_holdingRegisters[REG_M1_UNROLLED_WIRE_LENGTH_CM];
}

void Encoder_Save(Encoder_t* encoder){
    g_holdingRegisters[REG_M1_ENCODER_COUNT] = encoder->Encoder_Count;
    g_holdingRegisters[REG_M1_ENCODER_CONFIG] = encoder->Encoder_Config;
    g_holdingRegisters[REG_M1_ENCODER_RESET] = encoder->Encoder_Reset;
    g_holdingRegisters[REG_M1_CALIB_SENSOR_STATUS] = encoder->Encoder_Calib_Sensor_Status;
    g_holdingRegisters[REG_M1_CALIB_DISTANCE_CM] = encoder->Encoder_Calib_Length_CM_Max;
    g_holdingRegisters[REG_M1_ENCODER_RESET] = encoder->Encoder_Reset;
    g_holdingRegisters[REG_M1_CALIB_START] = encoder->Encoder_Calib_Start;
    g_holdingRegisters[REG_M1_CALIB_STATUS] = encoder->Encoder_Calib_Status;
    g_holdingRegisters[REG_M1_UNROLLED_WIRE_LENGTH_CM] = encoder->Encoder_Calib_Current_Length_CM;
    g_holdingRegisters[REG_M1_CALIB_ORIGIN_STATUS] = encoder->Calib_Origin_Status;
}

void Encoder_Process(Encoder_t* encoder){
    
    Encoder_Check_Calib_Origin(encoder);

    Encoder_Read(encoder);  // Collect encoder count
    
    Encoder_MeasureLength(encoder);

}

/**
 * @brief Tính toán độ dài dây đã xả ra dựa trên encoder với bán kính biến thiên
 * 
 * Phương pháp:
 * 1. Đọc số pulse từ encoder để tính góc quay
 * 2. Tính độ dài dây xả = chu vi hiện tại × số vòng quay
 * 3. Cập nhật bán kính mới dựa trên mô hình tuyến tính
 * 
 * Mô hình bán kính: R(L) = R_max - (R_max - R_min) × (L / L_total)
 * - Khi dây đầy (L=0): R = R_max (35mm)
 * - Khi dây hết (L=L_total): R = R_min (10mm)
 * 
 * @param encoder: Con trỏ đến cấu trúc encoder
 * @return Độ dài dây đã xả ra tính bằng mm
 */
uint16_t Encoder_MeasureLength(Encoder_t* encoder){
    static uint16_t last_encoder_count = 0;
    static float wire_unrolled = 0.0f;     // Tổng chiều dài dây đã xả (mm)
    static float current_radius = R_MAX;   // Bán kính hiện tại (mm)

    // 1️⃣ Tính delta count (thay đổi xung)
    int16_t delta_count = (int16_t)(encoder->Encoder_Count - last_encoder_count);
    last_encoder_count = encoder->Encoder_Count;

    // 2️⃣ Nếu có thay đổi xung thì mới tính
    if (delta_count != 0)
    {
        // a. Số vòng quay (4 xung / vòng)
        float delta_revolutions = (float)delta_count / (float)ENCODER_PPR;

        // b. Chu vi hiện tại
        float current_circumference = 2.0f * (float)M_PI * current_radius;

        // c. Độ dài dây thay đổi
        float delta_length = current_circumference * delta_revolutions;

        // d. Cập nhật tổng chiều dài dây
        wire_unrolled += delta_length;

        // e. Giới hạn trong phạm vi hợp lệ
        if (wire_unrolled < 0.0f) wire_unrolled = 0.0f;
        if (wire_unrolled > WIRE_LENGTH_CM) wire_unrolled = WIRE_LENGTH_CM;

        // f. Cập nhật bán kính tuyến tính theo chiều dài đã xả
        current_radius = R_MAX - (R_MAX - R_MIN) * (wire_unrolled / WIRE_LENGTH_CM);

        if (current_radius < R_MIN) current_radius = R_MIN;
        if (current_radius > R_MAX) current_radius = R_MAX;
    }

    // 3️⃣ Trả về độ dài dây đã xả (mm)
    return (uint16_t)wire_unrolled;
}

/**
 * @brief Reset lại tính toán độ dài dây về trạng thái ban đầu (dây đầy)
 * 
 * @param encoder: Con trỏ đến cấu trúc encoder
 */
void Encoder_ResetWireLength(Encoder_t* encoder){
    wire_unrolled = 0.0f;
    current_radius = R_MAX;
    last_encoder_count = encoder->Encoder_Count;
}

/**
 * @brief Đặt độ dài dây hiện tại (dùng cho calibration)
 * 
 * @param encoder: Con trỏ đến cấu trúc encoder
 * @param length_cm: Độ dài dây đã xả (cm)
 */
void Encoder_SetWireLength(Encoder_t* encoder, float length_cm){
    // Giới hạn giá trị
    if(length_cm < 0.0f) length_cm = 0.0f;
    if(length_cm > encoder->Encoder_Calib_Length_CM_Max) length_cm = encoder->Encoder_Calib_Length_CM_Max;
    
    encoder->Encoder_Calib_Current_Length_CM = length_cm;
    
    // Cập nhật bán kính tương ứng
    current_radius = R_MAX - (R_MAX - R_MIN) * (wire_unrolled / encoder->Encoder_Calib_Length_CM_Max);
    
    // Cập nhật last_encoder_count để tránh nhảy số
    last_encoder_count = encoder->Encoder_Count;
}

/**
 * @brief Lấy bán kính hiện tại của vòng dây
 * 
 * @return Bán kính hiện tại (mm)
 */
float Encoder_GetCurrentRadius(void){
    return current_radius;
}

void Encoder_Check_Calib_Origin(Encoder_t* encoder){
    bool Calib_Origin_Status = false;
    Calib_Origin_Status = HAL_GPIO_ReadPin(GPIOA, IN1_Pin);
    if(Calib_Origin_Status == true){
        encoder->Calib_Origin_Status = true;
        Encoder_Reset(encoder);
    }
    else{
        encoder->Calib_Origin_Status = false;
    }
}

void Encoder_Process_Calib(Encoder_t* encoder){
    if(encoder->Encoder_Calib_Sensor_Status == true){
        encoder->Encoder_Calib_Status = true;
    }
    else{
        encoder->Encoder_Calib_Status = false;
    }
}