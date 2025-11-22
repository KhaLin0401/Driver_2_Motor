#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "stdint.h" 
#include "main.h"
#include "stdbool.h"
typedef struct {
    uint16_t Status_Word;                           // REG_ENCODER_STATUS_WORD (0x0040)
    uint16_t volatile Encoder_Count;                // REG_ENCODER_COUNT (0x0041) - Quantity of pulses
    uint16_t Revolutions;                           // REG_ENCODER_REVOLUTIONS (0x0042)
    uint16_t Rmax;                                  // REG_ENCODER_RMAX (0x0043)
    uint16_t Rmin;                                  // REG_ENCODER_RMIN (0x0044)
    uint16_t Wire_Length_CM;                        // REG_ENCODER_WIRE_LENGTH_CM (0x0045)
    uint16_t Encoder_Reset;                         // REG_ENCODER_RESET (0x0046) - Reset encoder flag
    uint16_t Encoder_Calib_Length_CM_Max;           // REG_ENCODER_CALIB_WIRE_LENGTH_CM (0x0048) - Max distance of calibration
    uint16_t Encoder_Calib_Status;                  // REG_ENCODER_CALIB_STATUS (0x004A) - Status of calibration
    uint16_t Encoder_Calib_Current_Length_CM;       // REG_ENCODER_CALIB_CURRENT_LENGTH_CM (0x004B) - Current length of calibration
    bool Calib_Origin_Status;                       // REG_ENCODER_CALIB_ORIGIN_STATUS (0x004C) - Status of calibration origin
    uint16_t Unrolled_Wire_Length_CM;               // REG_ENCODER_UNROLLED_WIRE_LENGTH_CM (0x004D)
    
    // Internal fields (not mapped to Modbus)
    uint16_t Encoder_Config;                        // Configuration of encoder
    uint16_t Encoder_Calib_Sensor_Status;           // Calibration sensor status
    uint16_t Encoder_Calib_Start;                   // Calibration start flag
} Encoder_t;

extern Encoder_t encoder1;
//extern Encoder_t encoder2;

void Encoder_Init(void);
void Encoder_Read(Encoder_t* encoder);
void Encoder_Write(Encoder_t* encoder, uint16_t value);
void Encoder_Reset(Encoder_t* encoder);
void Encoder_Load(Encoder_t* encoder);
void Encoder_Save(Encoder_t* encoder);
void Encoder_Process(Encoder_t* encoder);
void Encoder_Process_Calib(Encoder_t* encoder);
void Encoder_Check_Calib_Origin(Encoder_t* encoder);

// Wire length measurement functions
uint16_t Encoder_MeasureLength(Encoder_t* encoder);
void Encoder_ResetWireLength(Encoder_t* encoder);
void Encoder_SetWireLength(Encoder_t* encoder, float length_mm);
float Encoder_GetCurrentRadius(void);

// Diagnostic functions
int32_t Encoder_GetTotalTicks(void);
uint32_t Encoder_GetNoiseRejectCount(void);
uint32_t Encoder_GetOverflowCount(void);
uint32_t Encoder_GetDMAHalfComplete(void);
uint32_t Encoder_GetDMAFullComplete(void);
uint32_t Encoder_GetPulseCount(void);
void Encoder_ResetDiagnostics(void);

#endif
