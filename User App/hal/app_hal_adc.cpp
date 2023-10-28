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
		.init_func = Callback_Function(MX_ADC3_Init),
		.in_mode = Input_Mode::SINGLE_ENDED,
		.interrupt_callback = Context_Callback_Function(),
		.interrupt_enabled = false,
};

Triggered_ADC::Triggered_ADC_Hardware_Channel Triggered_ADC::CHANNEL_4 = {
		.hadc = &hadc4,
		.init_func = Callback_Function(MX_ADC4_Init),
		.in_mode = Input_Mode::SINGLE_ENDED,
		.interrupt_callback = Context_Callback_Function(),
		.interrupt_enabled = false,
};

//Triggered_ADC::Triggered_ADC_Hardware_Channel Triggered_ADC::CHANNEL_5 = {
//		.hadc = &hadc5,
//		.init_func = MX_ADC5_Init,
//		.in_mode = Input_Mode::SINGLE_ENDED,
//		.interrupt_callback = empty_cb,
//		.interrupt_enabled = false,
//};

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

	//disable interrupts by default
	disable_interrupt();

	//start triggered ADC conversions
	hardware.hadc->Instance->CR |= ADC_CR_ADEN_Msk; //enable the ADC
	while(!(hardware.hadc->Instance->CR & ADC_CR_ADEN_Msk)); //wait for the ADC to be enabled
	hardware.hadc->Instance->CR |= ADC_CR_ADSTART_Msk; //start the ADC
}

//correct for ADC reading inaccuracies with this function
//won't actually scale the ADC value, basically just scaling the conversion constants returned by get_gain_offset()
bool Triggered_ADC::trim(float _gain_trim, float _offset_trim) {
	//don't allow gain_trim to be 0--unrecoverable
	if(gain_trim == 0) return false;

	//perform relative modification of the gain and offset values
	gain_trim *= _gain_trim;
	offset_trim += _offset_trim;

	//everything seems kosher
	return true;
}

//return the accumulated trim applied to the ADC
std::pair<float, float> Triggered_ADC::get_trim() {
	return std::make_pair(gain_trim, offset_trim);
}


//store the appropriate conversion complete callback passed in
void Triggered_ADC::attach_cb(Context_Callback_Function<> cb) {
	hardware.interrupt_callback = cb; //assign the callback function to the hardware mapping
}

//enable the conversion complete interrupt
void Triggered_ADC::enable_interrupt() {
	hardware.hadc->Instance->ISR = Triggered_ADC::CLEAR_ALL_INTERRUPTS; //clear any pending interrupts prior to enable
	hardware.hadc->Instance->IER = ADC_IER_EOCIE_Msk; //only want the end of regular conversion interrupt
	//NVIC gets initialized in the init func; don't have to worry about that here

	//set the interrupt enabled flag in the hardware struct
	hardware.interrupt_enabled = true;
}

void Triggered_ADC::disable_interrupt() {
	hardware.hadc->Instance->IER = 0; //disable all interrupts

	//clear the interrupt enabled flag in the hardware struct
	hardware.interrupt_enabled = false;
}

bool Triggered_ADC::interrupt_enabled() {
	//check to see if the interrupt is enabled in the corresponding hardware channel
	return hardware.interrupt_enabled;
}

//get the ADC value
uint16_t Triggered_ADC::get_val() {
	//direct register read
	return hardware.hadc->Instance->DR;
}

std::pair<float, float> Triggered_ADC::get_gain_offset() {
	//just return the internally maintained gain and offset values
	return std::make_pair(gain_v_to_counts * gain_trim, offset_counts + offset_trim);
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

	//run the callback function associated with this interrupt
	Triggered_ADC::CHANNEL_3.interrupt_callback();
}

void ADC4_IRQHandler(void) {
	//clear the interrupts (should just be the end of conversion interrupt)
	Triggered_ADC::CHANNEL_4.hadc->Instance->ISR = Triggered_ADC::CLEAR_ALL_INTERRUPTS;

	//run the callback function associated with this interrupt
	Triggered_ADC::CHANNEL_4.interrupt_callback();
}

//uncomment when necessary
//void ADC5_IRQHandler(void) {
//	//clear the interrupts (should just be the end of conversion interrupt)
//	Triggered_ADC::CHANNEL_5.hadc->Instance->ISR = Triggered_ADC::CLEAR_ALL_INTERRUPTS;
//
//	//run the callback function associated with this interrupt
//	Triggered_ADC::CHANNEL_5.interrupt_callback();
//}

