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

	//controller starts out as disabled
	//as such, disable the sampling callbacks for both channels
	disable_callback();
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

uint16_t Sampler::get_raw_fine() {
	return curr_fine.get_val();
}

uint16_t Sampler::get_raw_coarse() {
	return curr_coarse.get_val();
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

void Sampler::attach_sample_cb(Context_Callback_Function<> cb) {
	//attach the callback function to the interrupts
	curr_fine.attach_cb(cb);
	curr_coarse.attach_cb(cb);
}

void Sampler::enable_callback() {
	//disable just the COARSE ADC callback, and enable the fine callback;
	//since `fine` channel will be read first
	curr_coarse.disable_interrupt();
	curr_fine.enable_interrupt();

	//assert a flag
	callback_enable = true;
}

void Sampler::disable_callback() {
	//disable both ADC conversion interrupts
	curr_fine.disable_interrupt();
	curr_coarse.disable_interrupt();

	//deassert the flag
	callback_enable = false;
}

bool Sampler::get_callback_enabled() {
	return callback_enable;
}

bool Sampler::set_limits_fine(const uint32_t min_code, const uint32_t max_code) {
	//sanity check the inputs
	if(min_code > 0xFFFF) return false;
	if(max_code > 0xFFFF) return false;
	if(min_code > max_code) return false;

	//update internally maintained codes
	if_min = min_code;
	if_max = max_code;

	//and also update the configuration
	config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_LOW = min_code;
	config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_HIGH	= max_code;

	//everything is kosher
	return true;
}

//trim the coarse ADC, recompute constants, and adjust configuration
bool Sampler::trim_coarse(float gain_trim, float offset_trim) {
	//try to trim the ADC
	if(!curr_coarse.trim(gain_trim, offset_trim))
		return false;

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

	//everything worked out fine
	return true;
}

//trim the fine ADC, adjust config, and recompute constants
bool Sampler::trim_fine(float gain_trim, float offset_trim) {
	//try to trim the ADC
	if(!curr_fine.trim(gain_trim, offset_trim))
		return false;

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

	//everything went alright
	return true;
}

std::pair<uint32_t, uint32_t> Sampler::get_limits_fine() {
	return std::make_pair(	config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_LOW,
							config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_HIGH);
}

std::pair<float, float> Sampler::get_trim_fine() {
	return std::make_pair(	config.POWER_STAGE_CONFIGS[index].FINE_GAIN_TRIM,
							config.POWER_STAGE_CONFIGS[index].FINE_OFFSET_TRIM);
}

std::pair<float, float> Sampler::get_trim_coarse() {
	return std::make_pair(	config.POWER_STAGE_CONFIGS[index].COARSE_GAIN_TRIM,
							config.POWER_STAGE_CONFIGS[index].COARSE_OFFSET_TRIM);
}
