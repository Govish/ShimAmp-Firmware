/*
 * app_hal_timing.cpp
 *
 *  Created on: Mar 10, 2023
 *      Author: Ishaan
 *
 *	TIMER MAPPINGS:
 *
 */

#include "app_hal_timing.h"
extern "C" {
	#include "stm32g4xx_hal.h" //for HAL_Delay
}

//utility delay function
//should really never be called in the program if we write stuff well
//but useful for debugging
void Timer::delay_ms(uint32_t ms) {
	HAL_Delay(ms);
}

//system tick wrapper
//good for millisecond polling-based scheduling
uint32_t Timer::get_ms(){
	return HAL_GetTick();
}
