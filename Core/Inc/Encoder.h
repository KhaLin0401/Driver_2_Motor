#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "stdint.h" 
#include "main.h"
#include "stdbool.h"
typedef struct {
    uint16_t Encoder_Count;                //Quantity of pulses
    uint16_t Encoder_Config;               //Configuration of encoder
    uint16_t Encoder_Reset;                //Reset encoder flag
    uint16_t Encoder_Calib_Sensor_Status;  //
    uint16_t Encoder_Calib_Length_CM_Max;    //Max distance of calibration 
    uint16_t Encoder_Calib_Start;          //
    uint16_t Encoder_Calib_Status;         //Status of calibration
    uint16_t Encoder_Calib_Current_Length_CM;                //Current length of calibration
    bool Calib_Origin_Status;              //Status of calibration origin
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


#endif