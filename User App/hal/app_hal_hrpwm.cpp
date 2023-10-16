/*
 * app_hal_hrpwm.cpp
 *
 *  Created on: Sep 30, 2023
 *      Author: Ishaan
 */

#include "app_hal_hrpwm.h"

#include <algorithm> //for std::clamp
#include <cmath> //for round

//===================================== STATIC VARIABLE INITIALIZATION ===================================
const HRTIM_HandleTypeDef* HRPWM::hrtim_handle = &hhrtim1; //point to the hardware instance
bool HRPWM::MASTER_INITIALIZED = false;

//###### INITIALIZE HARDWARE CHANNEL DEFINITIONS #######
const HRPWM::HRPWM_Hardware_Channel HRPWM::CHANNEL_A1_PA8 = {
		.TIMER_INDEX = HRTIM_TIMERINDEX_TIMER_A,
		.COMPARE_CHANNEL = Compare_Channel_Mapping::COMPARE_CHANNEL_1,
		.OUTPUT_CONTROL_BITMASK = HRTIM_OUTPUT_TA1,
};

const HRPWM::HRPWM_Hardware_Channel HRPWM::CHANNEL_A2_PA9 = {
		.TIMER_INDEX = HRTIM_TIMERINDEX_TIMER_A,
		.COMPARE_CHANNEL = Compare_Channel_Mapping::COMPARE_CHANNEL_3,
		.OUTPUT_CONTROL_BITMASK = HRTIM_OUTPUT_TA2,
};

const HRPWM::HRPWM_Hardware_Channel HRPWM::CHANNEL_B1_PA10 = {
		.TIMER_INDEX = HRTIM_TIMERINDEX_TIMER_B,
		.COMPARE_CHANNEL = Compare_Channel_Mapping::COMPARE_CHANNEL_1,
		.OUTPUT_CONTROL_BITMASK = HRTIM_OUTPUT_TB1,
};

const HRPWM::HRPWM_Hardware_Channel HRPWM::CHANNEL_B2_PA11 = {
		.TIMER_INDEX = HRTIM_TIMERINDEX_TIMER_B,
		.COMPARE_CHANNEL = Compare_Channel_Mapping::COMPARE_CHANNEL_3,
		.OUTPUT_CONTROL_BITMASK = HRTIM_OUTPUT_TB2,
};

int HRPWM::num_timer_users = 0;

//=========================================== CLASS MEMBER FUNCTIONS =========================================

bool HRPWM::GET_ALL_ENABLED() {
	//read the master control register and see if timers are enabled
	return (hrtim_handle->Instance->sMasterRegs.MCR & TIMER_ENABLE_MASK) > 0;
}

/*
 * TODO (maybe) write a version of this function that:
 * 	- will disable and re-enable the PWM timers as necessary
 * 	- recomputes and updates all the duty cycle registers
 */
bool HRPWM::SET_FSW(float fsw_hz) {
	if(GET_ALL_ENABLED()) return false; //don't adjust the period if the timers are enabled

	//bounds check the desired switching frequency
	if(fsw_hz < FSW_MIN || fsw_hz > FSW_MAX) return false; //switching frequency is outside of hardware limits
	uint16_t period = (uint16_t)round(HRTIM_EFFECTIVE_CLOCK / fsw_hz); //compute the period value that should go into the period control register
	if(period < PWM_MIN_PERIOD || period > PWM_MAX_PERIOD) return false; //second round of sanity checking, in case uint16_t cast had errors

	//set the period by adjusting the master timer
	hrtim_handle->Instance->sMasterRegs.MPER = period;

	return true;
}

//switching frequency in Hz
float HRPWM::GET_FSW() {
	//compute the switching frequency by dividing effective clock rate (5.44GHz) by period value
	return HRTIM_EFFECTIVE_CLOCK / (float)GET_PERIOD();
}

uint16_t HRPWM::GET_PERIOD() {
	//read the period register
	return (uint16_t)(0xFFFF & hrtim_handle->Instance->sMasterRegs.MPER);
}

//triggered ADCs are configured to run off of ADC_TRIGGER_1
//expect a decent amount of rounding error here
//application can get the actual ADC trigger frequency from the getter function (useful for controller)
//these values can be updated even when timer is ticking
bool HRPWM::SET_ADC_TRIGGER_FREQUENCY(float ftrig_hz) {
	//bounds check the desired trigger frequency
	if(ftrig_hz > GET_FSW() || (ftrig_hz * (ADC_POSTSCALER_MASK + 1)) < GET_FSW()) return false;

	//compute the dividing factor between the switching frequency and the ADC trigger frequency
	uint8_t adc_multiple = std::round(GET_FSW() / ftrig_hz);

	//write the dividing factor to the ADC trigger postscaler bits (subtract 1 to get register value)
	hrtim_handle->Instance->sCommonRegs.ADCPS1 = (adc_multiple - 1) &  ADC_POSTSCALER_MASK;
	return true;
}

//based on the post-scaler value, figure out what frequency the ADC is getting triggered at
float HRPWM::GET_ADC_TRIGGER_FREQUENCY() {
	uint32_t dividing_ratio = (hrtim_handle->Instance->sCommonRegs.ADCPS1 & ADC_POSTSCALER_MASK) + 1;
	return GET_FSW() / (float)dividing_ratio;
}

//=========================================== INSTANCE METHODS =========================================

HRPWM::HRPWM(const HRPWM_Hardware_Channel& _channel_hw):
		channel_hw(_channel_hw) //initialize the channel hardware struct with the one passed in
{}
void HRPWM::init() {
	//call the initialization function at least once
	if(!MASTER_INITIALIZED) {
		MX_HRTIM1_Init();
		MASTER_INITIALIZED = true;
	}

	//ensure the channel is turned off
	//NOTE: depending on order of initialization, this might not do anythin
	this->force_low();

	//ensure that the particular channel is configured in one-shot retriggerable mode
	//i know these throw warnings, but this is the most convenient way to access this register
	//and will almost certainly be fine
	hrtim_handle->Instance->sTimerxRegs[channel_hw.TIMER_INDEX].TIMxCR &= RESET_MODE;
	hrtim_handle->Instance->sTimerxRegs[channel_hw.TIMER_INDEX].TIMxCR |= SINGLE_SHOT_RETRIGGERABLE_MODE;

	//ensure the output is disabled
	hrtim_handle->Instance->sCommonRegs.ODISR = channel_hw.OUTPUT_CONTROL_BITMASK;
}

void HRPWM::enable() {
	if(channel_enabled) return; //don't do anything if the channel is already enabled

	//enable the corresponding output
	hrtim_handle->Instance->sCommonRegs.OENR = channel_hw.OUTPUT_CONTROL_BITMASK;

	//flag this channel as enabled
	channel_enabled = true;

	//increment the number of users of the HRTIM instance
	num_timer_users++;

	//if this is the first user of the HRTIM, enable the peripheral
	if(num_timer_users <= 1) {
		ENABLE_ALL();
		num_timer_users = 1; //lowest it can be here is 1, correct any weird errors
	}
}

void HRPWM::disable() {
	if(!channel_enabled) return; //don't do anything if the channel is already disabled

	//disable the corresponding output
	hrtim_handle->Instance->sCommonRegs.ODISR = channel_hw.OUTPUT_CONTROL_BITMASK;

	//now flag this channel as disabled
	channel_enabled = false;

	//decrement the number of timer users
	num_timer_users--;

	//if there aren't any more users of the HRTIM, disable the peripheral
	if(num_timer_users <= 0) {
		DISABLE_ALL();
		num_timer_users = 0; //lowest it can be here is 0, correct any weird errors
	}
}

bool HRPWM::get_enabled() {
	return channel_enabled;
}


void HRPWM::force_low() {
	//HRTIM peripheral can handle a special case where we want to skip a PWM pulse
	//just set the appropriate compare channel to 0
	set_duty_raw(0);
}

void HRPWM::force_high() {
	//since we're operating the HRTIM TIMERx in "retriggerable one-shot mode"
	//set the compare values up such that the channel will just get retriggered before it can get cleared
	set_duty_raw(PWM_MAX_PERIOD);
}

//duty cycle 0-1; bounds checked version, returns true if set successfully
bool HRPWM::set_duty(float duty) {
	if(duty < 0 || duty > 1) return false; //if our duty cycle value is outta bounds

	//if we are setting our PWM value to fully-on or fully off, then handle as a special case
	if(duty == 0) this->force_low();
	else if(duty == 1) this->force_high();

	//otherwise, apply the bounds constraint and set the duty cycle using the other method
	//constrain between minimum and maximum acceptable duty cycle values
	else {
		uint16_t period = GET_PERIOD(); //boils down to register read and bitwise operations
		set_duty_raw((uint16_t)std::clamp(	(uint16_t)(duty * period), //value to constrain
											(uint16_t)PWM_MIN_MAX_DUTY, //min
											(uint16_t)(period - PWM_MIN_MAX_DUTY)) //max
										);
	}

	//we've successfully updated our duty cycle
	return true;
}

//faster, non-bounds-checked version of set_duty(float)
void HRPWM::set_duty_raw(uint16_t duty) {
	//directly write the duty parameter into the duty cycle register (the appropriate timer compare register)
	if(channel_hw.COMPARE_CHANNEL == Compare_Channel_Mapping::COMPARE_CHANNEL_1)
		hrtim_handle->Instance->sTimerxRegs[channel_hw.TIMER_INDEX].CMP1xR = duty;
	else
		hrtim_handle->Instance->sTimerxRegs[channel_hw.TIMER_INDEX].CMP3xR = duty;
}

float HRPWM::get_duty() {
	//duty cycle will be the value in the compare register divided by the period value
	return std::clamp(((float)(get_duty_raw())) / ((float)(GET_PERIOD())), 0.0f, 1.0f); //ensure duty cycle is capped to 1 (in case PWM is forced high)
}

//faster version of get_duty; no float conversion
//BE WARY--IF CHANNEL IS BEING FORCED HIGH, THIS FUNCTION WILL RETURN AN EFFECTIVE DUTY CYCLE > 1
uint16_t HRPWM::get_duty_raw() {
	if(channel_hw.COMPARE_CHANNEL == Compare_Channel_Mapping::COMPARE_CHANNEL_1)
		return hrtim_handle->Instance->sTimerxRegs[channel_hw.TIMER_INDEX].CMP1xR;
	else
		return hrtim_handle->Instance->sTimerxRegs[channel_hw.TIMER_INDEX].CMP3xR;
}

//========================= PRIVATE METHODS =======================

/*
 * TODO: halt DMA and interrupts as necessary
 */
void HRPWM::DISABLE_ALL() {
	//disable master timer and all individual timers in one write
	//i know this compound assignment throws warnings, but should almost certainly be fine
	hrtim_handle->Instance->sMasterRegs.MCR &= ~(TIMER_ENABLE_MASK);
}

/*
 * TODO: re-enable interrupts and DMA as necessary
 * maybe force a master timer reset event to resynchronize all the other timers?
 * 		- can guarantee resynch after one clock cycle so might not be necessary
 */
void HRPWM::ENABLE_ALL() {
	//enable master timer and all individual timers in one write
	//i know this compound assignment throws warnings, but should almost certainly be fine
	hrtim_handle->Instance->sMasterRegs.MCR |= TIMER_ENABLE_MASK;
}
