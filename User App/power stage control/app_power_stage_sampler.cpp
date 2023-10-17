/*
 * app_power_stage_sampler.cpp
 *
 *  Created on: Oct 16, 2023
 *      Author: Ishaan
 */

#include <functional> //for std::ref

#include "app_power_stage_sampler.h"

//instantiate our ADC instances
Sampler::Sampler(	Triggered_ADC::Triggered_ADC_Hardware_Channel& h_volts_fine,
					Triggered_ADC::Triggered_ADC_Hardware_Channel& h_volts_coarse,
					Triggered_ADC::Triggered_ADC_Hardware_Channel& h_curr_fine,
					Triggered_ADC::Triggered_ADC_Hardware_Channel& h_curr_coarse):
	volts_fine(h_volts_fine),
	volts_coarse(h_volts_coarse),
	curr_fine(h_curr_fine),
	curr_coarse(h_curr_coarse)
{}

//call this to initialize hardware and set stuff up for operation
void Sampler::init() {
	//initialize our ADCs
	volts_fine.init();
	volts_coarse.init();
	curr_fine.init();
	curr_coarse.init();

	//hook up our interrupt callback function to the ADC interrupt
	//have to capture this particular instance when we pass the handlers--doing this via lambda function
	//https://stackoverflow.com/questions/44791928/pass-member-function-as-parameter-to-other-member-function-c-11-function
	volts_fine.attach_cb([this](){SAMPLE_ISR();});
	volts_coarse.attach_cb([this](){SAMPLE_ISR();});
	curr_fine.attach_cb([this](){SAMPLE_ISR();});
	curr_coarse.attach_cb([this](){SAMPLE_ISR();});

	//since current is pseudo-differential, we'll initialize the offset to half the ADC range
	curr_fine.trim(1, 0x8000); //don't trim gain, just indicate the zero current is at 0x8000
	curr_coarse.trim(1, 0x8000);
}

//=================== MEASUREMENT FUNCTION ==================

//TODO: a bit more advanced sensor fusion--maybe some kinda LUT?
//I anticipate running into some limit cycling issues if we're operating at
//a margin between the fine and coarse ranges--want to keep the derivatives as smooth as possible

std::pair<float, float> Sampler::get_vi_readings() {
	uint16_t vf_read, if_read; //values we pull from the ADC read
	float vread, iread; //return values

	//read the current range
	//read the fine range is within appropriate limits, use that as our measurement
	if_read = curr_fine.get_val();
	if(if_read < if_max && if_read > if_min) {
		auto [gain, offset] = curr_fine.get_gain_offset(); //pull the zero offset from the fine ADC channel
		iread = ((float)if_read) / gain - offset; //scale current reading by appropriate scaling factor, and apply zero offset
	}
	else {
		auto [gain, offset] = curr_coarse.get_gain_offset(); //pull zero offset from the coarse ADC channel
		iread = ((float)curr_coarse.get_val()) / gain - offset; //apply gain and offset correction to the ADC reading
	}

	//read the voltage range
	//read the fine range is within appropriate limits, use that as our measurement
	vf_read = volts_fine.get_val();
	if(vf_read < vf_max && vf_read > vf_min) {
		auto [gain, offset] = volts_fine.get_gain_offset(); //pull the zero offset from the fine ADC channel
		vread = ((float)vf_read) / gain - offset; //scale voltage reading by appropriate scaling factor, and apply zero offset
	}
	else {
		auto [gain, offset] = volts_coarse.get_gain_offset(); //pull adjustments from the ADC channel
		vread = ((float)volts_coarse.get_val()) / gain - offset; //apply gain and offset correction to the ADC reading
	}

	//return the pair of values
	return std::make_pair(vread, iread);
}

//================================== BASIC ASSIGNMENT FUNCTIONS ============================

void Sampler::attach_sample_cb(callback_function_t cb) {
	if(cb == nullptr) return;
	sample_callback = cb;
}

constexpr void Sampler::set_limits_curr_fine(const uint16_t min_code, const uint16_t max_code) {
	if_min = min_code;
	if_max = max_code;
}

constexpr void Sampler::set_limits_volts_fine(const uint16_t min_code, const uint16_t max_code) {
	vf_min = min_code;
	vf_max = max_code;
}

void Sampler::set_current_gains(const float adcv_to_i_fine, const float adcv_to_i_coarse) {
	//have to maintain these static variables, since adc `trim()` function expects relative adjustments
	static float prev_i_fine = 1;
	static float prev_i_coarse = 1;

	//`gain_trim` is a unitless quantity, so need to refer to what we scaled it to previously
	//ADC gain is in units of counts per amp (or counts per adc_volt)
	//we passed in amps per adc_volt, so need to invert it (reason it's in the denominator not numerator)
	float fine_trim = prev_i_fine / adcv_to_i_fine;
	float coarse_trim = prev_i_coarse / adcv_to_i_coarse;

	//apply our calculated trim factors to the ADC channels
	curr_fine.trim(fine_trim, 0);
	curr_coarse.trim(coarse_trim, 0);

	//remember our settings for the future
	//since trim adjustments are all relative
	prev_i_fine = adcv_to_i_fine;
	prev_i_coarse = adcv_to_i_coarse;

}

void Sampler::set_voltage_gains(const float adcv_to_vterm_fine, const float adcv_to_vterm_coarse) {
	//have to maintain these static variables, since adc `trim()` function expects relative adjustments
	static float prev_vterm_fine = 1;
	static float prev_vterm_coarse = 1;

	//`gain_trim` is a unitless quantity, so need to refer to what we scaled it to previously
	//ADC gain is in units of counts per volt (or counts per adc_volt)
	//we passed in volts per adc_volt, so need to invert it (reason it's in the denominator not numerator)
	float fine_trim = prev_vterm_fine / adcv_to_vterm_fine;
	float coarse_trim = prev_vterm_coarse / adcv_to_vterm_coarse;

	//apply our calculated trim factors to the ADC channels
	volts_fine.trim(fine_trim, 0);
	volts_coarse.trim(coarse_trim, 0);

	//remember our settings for the future
	//since trim adjustments are all relative
	prev_vterm_fine = adcv_to_vterm_fine;
	prev_vterm_coarse = adcv_to_vterm_coarse;

}

//====================================== PRIVATE ISR ========================================
void Sampler::SAMPLE_ISR() {
	num_updates_received++; //one more sample from an ADC
	if(num_updates_received < NUM_REQUIRED_READINGS) return; //check if we have all samples

	//we have all our samples, run the callback
	sample_callback();

	//and reset the number of ADCs we've gotten updates from
	num_updates_received = 0;
}
