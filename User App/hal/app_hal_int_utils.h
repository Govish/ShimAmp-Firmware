/*
 * app_hal_int_priority_maps.h
 *
 *  Created on: Mar 11, 2023
 *      Author: Ishaan
 */

#ifndef BOARD_HAL_INC_APP_HAL_INT_UTILS_H_
#define BOARD_HAL_INC_APP_HAL_INT_UTILS_H_

//enum numeric mappings correspond to to NVIC values
typedef enum Priorities {
	REALTIME = 0,
	HIGH = 1,
	MED_HIGH = 2,
	MED = 3,
	MED_LOW = 4,
	LOW = 5
} int_priority_t;


//====================== DECLARING INTERRUPT HANDLERS HERE =========================
extern "C" {
	//ADC interrupts
	void ADC3_IRQHandler(void); //ADC channel 3 conversion complete interrupt
	void ADC4_IRQHandler(void); //ADC channel 4 conversion complete interrupt
	void ADC5_IRQHandler(void); //ADC channel 5 conversion complete interrupt
}

#endif /* BOARD_HAL_INC_APP_HAL_INT_UTILS_H_ */
