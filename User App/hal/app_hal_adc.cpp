/*
 * app_hal_adc.cpp
 *
 *  Created on: Oct 15, 2023
 *      Author: Ishaan
 */


#include "app_hal_adc.h"

///========================= initialization of static fields ========================
//ADC instance will be initialized in the constructor, so don't need to worry too much about the NULL here
Triggered_ADC::Triggered_ADC_Hardware_Channel Triggered_ADC::CHANNEL_3 = {
		.hadc = &hadc3,
		.init_func = MX_ADC3_Init,
		.in_mode = Input_Mode::SINGLE_ENDED,
		.adc_val = 0,
		.recently_updated = false,
		.interrupt_callback = empty_cb
};

//===================================================================================

//constructor just stores the corresponding hardware instance
Triggered_ADC::Triggered_ADC(Triggered_ADC_Hardware_Channel& _hardware):
		hardware(_hardware)
{
	//for single-ended operation, treat 0 as 0V, and map the max value to VREF on the input
	if(hardware.in_mode == Input_Mode::SINGLE_ENDED) {
		gain_v_to_counts = ADC_REFERENCE_VOLTAGE / ADC_MAX_CODE;
		offset_counts = 0;
	}

	//for differential-mode operation, treat somewhere around the middle as zero, and each LSB represents double the voltage
	else {
		gain_v_to_counts = 2*ADC_REFERENCE_VOLTAGE / ADC_MAX_CODE;
		offset_counts = (ADC_MAX_CODE / 2) + 8; //zero is slightly above the middle value
	}
}


/*
 * Initialization routine does the following:
 *  - initializes hardware through HAL functions
 *  - configures the channel in single-ended or differential mode accordingly
 *  - calibrates the particular ADC channel
 *  - configures the particular ADC channel in BULB sampling mode
 *  - Enables the ADC operation in triggered + interrupt mode
 *
 *  NOTE: Normal operation of triggered ADCs is sampling in bulb mode with conversion triggered by HRTIM period
 *  As a byproduct, two things:
 *  	- ADC conversions will only be ongoing if the HRTIM is enabled;
 *  		- I can't think of a use case where we'd want output voltage and current to be read when HRTIM is disabled
 *  	- We'd want to only run a conversion every couple of HRTIM cycles;
 *  		- THIS IS ADJUSTED ON THE HRTIM SIDE, can't really do anything about that here
 */
void Triggered_ADC::init() {
	//call the STM32 initialization
	//will configure all channels in their desired modes
	hardware.init_func();

	//calibrate the ADC channel in the correct input mode
	//write to ADCAL (and set ADCALDIF bit accordingly)
	//poll ADCAL until it's 0
	if(hardware.in_mode == Input_Mode::SINGLE_ENDED)
		HAL_ADCEx_Calibration_Start(hardware.hadc, ADC_SINGLE_ENDED);
	else
		HAL_ADCEx_Calibration_Start(hardware.hadc, ADC_DIFFERENTIAL_ENDED);

	//configure the channel in bulb sampling mode
	//sampling period starts right after the previous conversion finishes
	//ensures the longest possible time to charge sampling capacitor
	//and conversion right on the trigger event
	hardware.hadc->Instance->CFGR2 |= ADC_CFGR2_BULB_Msk;

	//enable NVIC interrupts and ADC interrupts
	hardware.hadc->Instance->IER = CLEAR_ALL_INTERRUPTS; //clear any set interrupt flags by writing 1
	hardware.hadc->Instance->IER = ADC_IER_EOCIE_Msk; //only want the end of regular conversion interrupt
	//NVIC gets initialized in the init func; don't have to worry about that here

	//start triggered ADC conversions
	hardware.hadc->Instance->CR |= ADC_CR_ADEN_Msk; //enable the ADC
	while(!(hardware.hadc->Instance->CR & ADC_CR_ADEN_Msk)); //wait for the ADC to be enabled
	hardware.hadc->Instance->CR |= ADC_CR_ADSTART_Msk; //start the ADC
}

//correct for ADC reading inaccuracies with this function
//adds gain and offset at the hardware level, so slightly higher performance than software implementation
void Triggered_ADC::trim(float gain_trim, float offset_trim) {
	//don't allow gain_trim to be 0--unrecoverable
	if(gain_trim == 0) return;

	//perform relative modification of the gain and offset values
	gain_v_to_counts *= gain_trim;
	offset_counts += offset_trim;
}

//store the appropriate conversion complete callback passed in
void Triggered_ADC::attach_cb(callback_function_t cb) {
	if(cb == nullptr) return; //don't take null functions
	hardware.interrupt_callback = cb; //assign the callback function to the hardware mapping
}

//get whether the ADC has been updated since last read of the ADC value
bool Triggered_ADC::get_updated() {
	return hardware.recently_updated;
}

//get the ADC value
uint16_t Triggered_ADC::get_val(const bool clear_flag) {
	if(clear_flag) hardware.recently_updated = false; //clear the read flag as necessary
	return hardware.adc_val;
}

std::pair<float, float> Triggered_ADC::get_gain_offset() {
	//just return the internally maintained gain and offset values
	return std::make_pair(gain_v_to_counts, offset_counts);
}


//======================================= PROCESSOR ISRs ====================================

/*
 * In ADC ISR, do the following:
 *  - read the ISR (to clear it maybe)
 *  - read the ADC_DR to get the regular data converted
 */
void ADC3_IRQHandler(void) {
	//clear the interrupts (should just be the end of conversion interrupt)
	Triggered_ADC::CHANNEL_3.hadc->Instance->ISR = Triggered_ADC::CLEAR_ALL_INTERRUPTS;

	//store the converted value into the data register and update the `recently_updated` flag
	Triggered_ADC::CHANNEL_3.adc_val = Triggered_ADC::CHANNEL_3.hadc->Instance->DR;
	Triggered_ADC::CHANNEL_3.recently_updated = true;

	//run the callback function associated with this interrupt
	Triggered_ADC::CHANNEL_3.interrupt_callback();
}

