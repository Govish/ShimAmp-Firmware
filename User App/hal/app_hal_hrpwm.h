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
	//#include "stm32g4xx_hal_hrtim.h" //for the hrtim handle, caused build errors for whatever reason
	#include "hrtim.h"
}

class HRPWM {
public:

	enum class Compare_Channel_Mapping {
		COMPARE_CHANNEL_1,
		COMPARE_CHANNEL_3,
	};

	//======================================= HARDWARE REFERENCES TO EACH PWM CHANNEL ======================================
	struct HRPWM_Hardware_Channel {
		const size_t TIMER_INDEX; //use this to index into HAL functions
		const Compare_Channel_Mapping COMPARE_CHANNEL;
		const uint32_t OUTPUT_CONTROL_BITMASK; //write this value to OENR to enable the particular channel output
	};

	static const HRPWM_Hardware_Channel CHANNEL_A1_PA8;
	static const HRPWM_Hardware_Channel CHANNEL_A2_PA9;
	static const HRPWM_Hardware_Channel CHANNEL_B1_PA10;
	static const HRPWM_Hardware_Channel CHANNEL_B2_PA11;

	//======================================= CLASS METHODS ======================================
	static bool GET_ALL_ENABLED();
	static bool SET_FSW(float fsw_hz); //set the desired switching frequency in Hz
	static float GET_FSW(); //switching frequency in Hz
	static uint16_t GET_PERIOD(); //HRTIM counts in one period

	//Triggered ADCs should be sampled atsome integer ratio of switching frequency
	//this aliases switching harmonics down to DC and at worst should just require some kinda DC correction factor
	//to ADC readings
	static bool SET_ADC_TRIGGER_FREQUENCY(float ftrig_hz);
	static float GET_ADC_TRIGGER_FREQUENCY();


	HRPWM(const HRPWM_Hardware_Channel& _channel_hw); //constructor

	//delete copy constructor, and assignment operator
	//such as to prevent weird hardware conflicts
	//TODO: can't delete destructor since I need it when accessing HRPWM_Hardware_Channels?
	HRPWM(HRPWM const&) = delete;
	void operator=(HRPWM const&) = delete;

	//###### initialization and enable ######
	void init(); //need this function in order to force the execution order to be after HAL functions
	void enable(); //enable the particular channel
	void disable(); //disable the particular channel
	bool get_enabled(); //return whether the stage is enabled or not

	//###### duty cycle control ######
	void force_low();
	void force_high();
	bool set_duty(float duty); //duty cycle 0-1; bounds checked version, returns true if set successfully
	void set_duty_raw(uint16_t duty); //faster, non-bounds-checked version of set_duty(float)
	float get_duty();
	uint16_t get_duty_raw(); //faster version of get_duty; no float conversion

private:
	//============== quick utility functions ===============
	static void ENABLE_ALL();
	static void DISABLE_ALL();

	//============================== OPERATIONAL CONSTANTS ============================
	static constexpr float HRTIM_EFFECTIVE_CLOCK = 170.0e6 * 32.0; //effective clock rate of the high resolution timer
	static const uint16_t PWM_MIN_MAX_DUTY = 0x60; //duty cycle can be min <this> or max <period> - <this>
	static const uint16_t PWM_MIN_PERIOD = 0x100; //might not be strictly this, but constrain to something reasonable
	static const uint16_t PWM_MAX_PERIOD = 0xFFDF; //maximum value we can load into any counters
	static constexpr float FSW_MIN = HRTIM_EFFECTIVE_CLOCK / (float)PWM_MAX_PERIOD;
	static constexpr float FSW_MAX = HRTIM_EFFECTIVE_CLOCK / (float)PWM_MIN_PERIOD;

	//============================== BITMASK AND REGISTER CONSTANTS ============================
	static const uint32_t TIMER_ENABLE_MASK = 0x7F0000; //enable/disable all timers with this mask
	static const uint32_t RESET_MODE = ~(0x18); //reset the channel mode bits
	static const uint32_t SINGLE_SHOT_RETRIGGERABLE_MODE = 0x10; //bitwise or with channel to put into single shot retriggerable
	static const uint8_t ADC_POSTSCALER_MASK = 0x1F; //maximum value we can put into the post-scaler

	//================================ STATIC MEMBERS =================================
	static const HRTIM_HandleTypeDef* hrtim_handle; //pointer to the hardware
	static bool MASTER_INITIALIZED; //flag that says whether the HRTIM peripheral has been initialized
	static int num_timer_users; //counting semaphore to determine when we can enable/disable our timer

	//============================== INSTANCE MEMBERS ============================
	const HRPWM_Hardware_Channel& channel_hw; //connect the instance to a particular piece of hardware
	bool channel_enabled = false;
};


#endif /* HAL_APP_HAL_HRPWM_H_ */
