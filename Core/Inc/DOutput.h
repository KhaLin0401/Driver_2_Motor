#ifndef __DOUTPUT_H__
#define __DOUTPUT_H__
#include "ModbusMap.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif
// Digital Output Channel Structure
typedef struct {
    bool relay1;    // Relay channel 1 state (0=OFF, 1=ON)
    bool relay2;    // Relay channel 2 state

} DOutputState_t;

extern DOutputState_t doutput_state;

// Function prototypes
void DOutput_Init(void);
void DOutput_Load(DOutputState_t* state);
void DOutput_Save(DOutputState_t* state);
void DOutput_SetRelay(uint8_t channel, bool state);
void DOutput_GetState(DOutputState_t* state);
void DOutput_Process(DOutputState_t* state);

#ifdef __cplusplus
}
#endif
#endif
