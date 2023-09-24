/*
 * app_hal_int_priority_maps.h
 *
 *  Created on: Mar 11, 2023
 *      Author: Ishaan
 */

#ifndef BOARD_HAL_INC_APP_HAL_INT_UTILS_H_
#define BOARD_HAL_INC_APP_HAL_INT_UTILS_H_

inline void empty_cb() {} //defining some kinda empty function with which to initialize callback functions (inlining to avoid compilation errors)

//define a type for callback functions that we can call when interrupts are handled
//doing this for easier readability
//https://stackoverflow.com/questions/6339970/c-using-function-as-parameter
typedef void (*callback_function_t)(void);

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
	//example provided below:
	//void TIM1_BRK_TIM9_IRQHandler(void); //general purpose timer channel 0
}

#endif /* BOARD_HAL_INC_APP_HAL_INT_UTILS_H_ */