#include "DOutput.h"
#include "main.h"
DOutputState_t doutput_state;

void DOutput_Init(void)
{
    // Initialize the digital output state
    doutput_state.relay1 = 0;
    doutput_state.relay2 = 0;
}

void DOutput_Load(DOutputState_t* state){
    state->relay1 = g_holdingRegisters[0x0040];
    state->relay2 = g_holdingRegisters[0x0041];
}

void DOutput_Save(DOutputState_t* state){
    g_holdingRegisters[0x0040] = state->relay1;
    g_holdingRegisters[0x0041] = state->relay2;
}
void DOutput_SetRelay(uint8_t channel, bool state){
    if(channel == 1){
        doutput_state.relay1 = state;
        HAL_GPIO_WritePin(OUT1_GPIO_Port, OUT1_Pin, state);
    }
    else if(channel == 2){
        doutput_state.relay2 = state;
        HAL_GPIO_WritePin(OUT2_GPIO_Port, OUT2_Pin, state);
    }
}
void DOutput_GetState(DOutputState_t* state){
    state->relay1 = doutput_state.relay1;
    state->relay2 = doutput_state.relay2;
}

void DOutput_Process(DOutputState_t* state){
    // Relay 1 active when either motor is running (check actual running status)
    if(g_holdingRegisters[REG_M1_ACTUAL_SPEED] > 0 || g_holdingRegisters[REG_M2_ACTUAL_SPEED] > 0) {
        DOutput_SetRelay(1, true);
    } else {
        DOutput_SetRelay(1, false);
    }

    // Relay 2 active on critical system error
    if(g_holdingRegisters[0x0100] & 0x8000) { // Check critical error bit
        DOutput_SetRelay(2, true); 
    } else {
        DOutput_SetRelay(2, false);
    }
}