/*
 * app_power_stage_sampler.cpp
 *
 *  Created on: Oct 16, 2023
 *      Author: Ishaan
 */

#include <functional> //for std::ref

#include "app_power_stage_sampler.h"

//instantiate our ADC instances and store configuration and index
Sampler::Sampler(	Triggered_ADC::Triggered_ADC_Hardware_Channel& h_curr_fine,
					Triggered_ADC::Triggered_ADC_Hardware_Channel& h_curr_coarse,
					Configuration::Configuration_Params& _config,
					const size_t _index):
	curr_fine(h_curr_fine),
	curr_coarse(h_curr_coarse),
	config(_config),
	index(_index)
{}

//call this to initialize hardware and set stuff up for operation
void Sampler::init() {
	//initialize our ADCs
	curr_fine.init();
	curr_coarse.init();

	//set our fine range ADC limits from configuration
	set_limits_fine(config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_LOW,
					config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_HIGH);

	//load our trim values into our ADCs from configuration
	trim_fine(	config.POWER_STAGE_CONFIGS[index].FINE_GAIN_TRIM,
				config.POWER_STAGE_CONFIGS[index].FINE_OFFSET_TRIM);
	trim_coarse(config.POWER_STAGE_CONFIGS[index].COARSE_GAIN_TRIM,
				config.POWER_STAGE_CONFIGS[index].COARSE_OFFSET_TRIM);

	//hook up our interrupt callback function to the ADC interrupt
	//have to capture this particular instance when we pass the handlers--doing this via lambda function
	//https://stackoverflow.com/questions/44791928/pass-member-function-as-parameter-to-other-member-function-c-11-function
	curr_fine.attach_cb([this](){SAMPLE_ISR();});
	curr_coarse.attach_cb([this](){SAMPLE_ISR();});
}

//=================== MEASUREMENT FUNCTION ==================

//TODO: a bit more advanced sensor fusion--maybe some kinda LUT?
//I anticipate running into some limit cycling issues if we're operating at
//a margin between the fine and coarse ranges--want to keep the derivatives as smooth as possible

float Sampler::get_current_reading() {
	uint16_t if_read; //values we pull from the ADC read
	float iread; //return values

	//read the current range
	//read the fine range is within appropriate limits, use that as our measurement
	if_read = curr_fine.get_val();
	if(if_read < if_max && if_read > if_min) {
		iread = ((float)if_read - fine_offset_counts) / fine_total_gain; //scale current reading by appropriate scaling factor, and apply zero offset
	}

	else {
		iread = ((float)curr_coarse.get_val() - coarse_offset_counts) / coarse_total_gain; //apply gain and offset correction to the ADC reading
	}

	//return current value
	return iread;
}

bool Sampler::get_running() {
	return HRPWM::GET_ALL_ENABLED();
}

float Sampler::GET_SAMPLING_FREQUENCY() {
	return HRPWM::GET_ADC_TRIGGER_FREQUENCY();
}

//sampler outputs real-world-coherent units, i.e. doesn't have a gain associated with it
float Sampler::get_gain() {
	return 1.0;
}

//================================== BASIC ASSIGNMENT FUNCTIONS ============================

void Sampler::attach_sample_cb(callback_function_t cb) {
	if(cb == nullptr) return;
	sample_callback = cb;
}

void Sampler::enable_callback() {
	callback_enable = true;
}

void Sampler::disable_callback() {
	callback_enable = false;
}

bool Sampler::get_callback_enabled() {
	return callback_enable;
}

void Sampler::set_limits_fine(const uint16_t min_code, const uint16_t max_code) {
	//update internally maintained codes
	if_min = min_code;
	if_max = max_code;

	//and also update the configuration
	config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_LOW = min_code;
	config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_HIGH	= max_code;
}

//trim the coarse ADC, recompute constants, and adjust configuration
void Sampler::trim_coarse(float gain_trim, float offset_trim) {
	//trim the ADC
	curr_coarse.trim(gain_trim, offset_trim);

	//compute the coarse channel overall gain and offset
	//such that we get +/- current after applying (`ADC_counts` - offset) / gain
	auto [adc_gain, adc_offset] = curr_coarse.get_gain_offset();
	coarse_offset_counts = adc_offset; //offset is chill
	coarse_total_gain = adc_gain * //V_ADC to counts
						config.POWER_STAGE_CONFIGS[index].COARSE_AMP_GAIN_VpV * //V_shunt to V_ADC
						config.POWER_STAGE_CONFIGS[index].SHUNT_RESISTANCE; //I_shunt to V_shunt

	//get the cumulative trim values that we've applied to the ADC and store into config
	auto [total_gain_trim, total_offset_trim] = curr_coarse.get_trim();
	config.POWER_STAGE_CONFIGS[index].COARSE_GAIN_TRIM = total_gain_trim;
	config.POWER_STAGE_CONFIGS[index].COARSE_OFFSET_TRIM = total_offset_trim;
}

//trim the fine ADC, adjust config, and recompute constants
void Sampler::trim_fine(float gain_trim, float offset_trim) {
	//trim the ADC
	curr_fine.trim(gain_trim, offset_trim);

	//compute the coarse channel overall gain and offset
	//such that we get +/- current after applying (`ADC_counts` - offset) / gain
	auto [adc_gain, adc_offset] = curr_fine.get_gain_offset();
	fine_offset_counts = adc_offset; //offset is chill
	fine_total_gain = adc_gain * //V_ADC to counts
						config.POWER_STAGE_CONFIGS[index].FINE_AMP_GAIN_VpV * //V_shunt to V_ADC
						config.POWER_STAGE_CONFIGS[index].SHUNT_RESISTANCE; //I_shunt to V_shunt

	//get the cumulative trim values that we've applied to the ADC and store into config
	auto [total_gain_trim, total_offset_trim] = curr_fine.get_trim();
	config.POWER_STAGE_CONFIGS[index].FINE_GAIN_TRIM = total_gain_trim;
	config.POWER_STAGE_CONFIGS[index].FINE_OFFSET_TRIM = total_offset_trim;
}

//====================================== PRIVATE ISR ========================================
void Sampler::SAMPLE_ISR() {
	num_updates_received++; //one more sample from an ADC
	if(num_updates_received < NUM_REQUIRED_READINGS) return; //check if we have all samples

	//we have all our samples, run the callback if it's enabled
	if(callback_enable) sample_callback();

	//and reset the number of ADCs we've gotten updates from
	num_updates_received = 0;
}
