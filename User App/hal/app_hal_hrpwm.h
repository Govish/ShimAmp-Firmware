/*
 * app_hal_hrpwm.h
 *
 *  Created on: Sep 30, 2023
 *      Author: Ishaan
 */

#ifndef HAL_APP_HAL_HRPWM_H_
#define HAL_APP_HAL_HRPWM_H_

#include <stdbool.h>

extern "C" {
	#include "stm32g474xx.h" //for types
	#include "hrtim.h"
}

#include "app_hal_int_utils.h" //for callback definitions

class HRPWM {

	struct HRPWM_Hardware_Channel {
		HRTIM_HandleTypeDef* const hrtim;
		const callback_function_t init_func;
		HRPWM* instance; //points to the firmware instance corresponding to this hardware
	};

	static HRPWM_Hardware_Channel CHANNEL_0;
	static HRPWM_Hardware_Channel CHANNEL_1;
	static HRPWM_Hardware_Channel CHANNEL_2;
	static HRPWM_Hardware_Channel CHANNEL_3;


public:
	static void DISABLE_ALL();
	static void ENABLE_ALL();
	static void SET_PERIOD_ALL();
	static uint16_t GET_PERIOD(); //master timer period (raw register read basically, no unit conversion)
	static float GET_FSW(); //switching frequency in Hz

	/*TODO: ADC synchronization and period elapsed callback*/

	HRPWM(); //constructor
	void force_low();
	bool set_duty(float duty); //duty cycle 0-1; bounds checked version, returns true if set successfully
	void set_duty(uint16_t duty); //faster, non-bounds-checked version of set_duty(float)
	float get_duty();
	uint16_t get_raw_duty(); //faster version of get_duty; no float conversion

private:
	static bool MASTER_INITIALIZED;
	static uint16_t period; //period of the master timer that synchronizes all channels

	uint16_t duty; //duty cycle for the particular PWM channel
};



#endif /* HAL_APP_HAL_HRPWM_H_ */
