/*
 * app_hal_timing.h
 *
 *  Created on: Mar 10, 2023
 *      Author: Ishaan
 *
 *  TO ADD ANOTHER TIMER CHANNEL:
 *
 *
 */

#ifndef BOARD_HAL_INC_APP_HAL_TIMING_H_
#define BOARD_HAL_INC_APP_HAL_TIMING_H_

extern "C" {
#include "stm32g474xx.h" //for uint32_t
}

class Timer {
public:
	static void delay_ms(uint32_t ms);
	static uint32_t get_ms();

private:
	Timer(); //don't allow instantiation of a timer class just yet
};

#endif /* BOARD_HAL_INC_APP_HAL_TIMING_H_ */
