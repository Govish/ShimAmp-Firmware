/*
 * app_power_stage_sampler.cpp
 *
 *  Created on: Oct 16, 2023
 *      Author: Ishaan
 */

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
	//curr_fine.init();
	curr_coarse.init();

	//set our fine range ADC limits from configuration
	set_limits_fine(config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_LOW,
					config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_HIGH,
					config.POWER_STAGE_CONFIGS[index].RANGE_BLEND_LENGTH);

	//load our trim values into our ADCs from configuration
	//also generates our ADC code --> current lookup tables
	trim_fine(	config.POWER_STAGE_CONFIGS[index].FINE_GAIN_TRIM,
				config.POWER_STAGE_CONFIGS[index].FINE_OFFSET_TRIM);
	trim_coarse(config.POWER_STAGE_CONFIGS[index].COARSE_GAIN_TRIM,
				config.POWER_STAGE_CONFIGS[index].COARSE_OFFSET_TRIM);

	//controller starts out as disabled
	//as such, disable the sampling callbacks for both channels
	disable_callback();
}

//=================== MEASUREMENT FUNCTION ==================


float Sampler::get_current_reading() {
	//most of the hard work for ADC sensor fusion/range switching happens with in the ADC LUT
	//these apply the calculated gain and offset to the ADC readings to get a useful current value
	//and also apply a "blending" to add a bit of hand-wavey sensor fusion
	//and also smooth out any discontinuities in the transition between the two ranges
	return ADC_LUT_COARSE[curr_coarse.get_val()];// + ADC_LUT_FINE[curr_fine.get_val()];
}

uint16_t Sampler::get_raw_fine() {
	return 0; //return curr_fine.get_val();
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
	//curr_fine.attach_cb(cb);
	curr_coarse.attach_cb(cb);
}

void Sampler::enable_callback() {
	//disable just the COARSE ADC callback, and enable the fine callback;
	//since `fine` channel will be read first
	curr_coarse.enable_interrupt();
	//curr_fine.disable_interrupt();

	//assert a flag
	callback_enable = true;
}

void Sampler::disable_callback() {
	//disable both ADC conversion interrupts
	//curr_fine.disable_interrupt();
	curr_coarse.disable_interrupt();

	//deassert the flag
	callback_enable = false;
}

bool Sampler::get_callback_enabled() {
	return callback_enable;
}

bool Sampler::set_limits_fine(const uint32_t min_code, const uint32_t max_code, uint32_t blend_length) {
	//sanity check the inputs
	if(min_code > 0xFFFF) return false;
	if(max_code > 0xFFFF) return false;
	if(min_code > max_code) return false;
	if(2*blend_length > (max_code - min_code)) return false;

	//update internally maintained codes
	if_min = (uint16_t)min_code;
	if_max = (uint16_t)max_code;
	blend_counts = (uint16_t)blend_length;

	//and also update the configuration
	config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_LOW = min_code;
	config.POWER_STAGE_CONFIGS[index].FINE_RANGE_VALID_HIGH	= max_code;

	//recompute the current lookup tables
	COMPUTE_LUTs();

	//everything is kosher
	return true;
}

//trim the coarse ADC, recompute constants, and adjust configuration
bool Sampler::trim_coarse(float gain_trim, float offset_trim) {
	//don't allow this operation if the sampler is running --> updating ADC tables isn't atomic!
	if(get_running()) return false;

	//try to trim the ADC
	if(!curr_coarse.trim(gain_trim, offset_trim))
		return false;

	//compute the coarse channel overall gain and offset
	//such that we get +/- current after applying (`ADC_counts` - offset) / gain
	auto [adc_gain, adc_offset] = curr_coarse.get_gain_offset();
	coarse_offset_counts = adc_offset +
						curr_coarse.get_adc_max_code()/2; //offset by half of the ADC range
	coarse_total_gain = adc_gain * //V_ADC to counts
						config.POWER_STAGE_CONFIGS[index].COARSE_AMP_GAIN_VpV * //V_shunt to V_ADC
						config.POWER_STAGE_CONFIGS[index].SHUNT_RESISTANCE; //I_shunt to V_shunt

	//get the cumulative trim values that we've applied to the ADC and store into config
	auto [total_gain_trim, total_offset_trim] = curr_coarse.get_trim();
	config.POWER_STAGE_CONFIGS[index].COARSE_GAIN_TRIM = total_gain_trim;
	config.POWER_STAGE_CONFIGS[index].COARSE_OFFSET_TRIM = total_offset_trim;

	//update the current LUTs
	COMPUTE_LUTs();

	//everything worked out fine
	return true;
}

//trim the fine ADC, adjust config, and recompute constants
bool Sampler::trim_fine(float gain_trim, float offset_trim) {
	//don't allow this operation if the sampler is running --> updating ADC tables isn't atomic!
//	if(get_running()) return false;
//
//	//try to trim the ADC
//	if(!curr_fine.trim(gain_trim, offset_trim))
//		return false;
//
//	//compute the coarse channel overall gain and offset
//	//such that we get +/- current after applying (`ADC_counts` - offset) / gain
//	auto [adc_gain, adc_offset] = curr_fine.get_gain_offset();
//	fine_offset_counts = adc_offset +
//						curr_fine.get_adc_max_code() / 2.0f; //offset by half of the ADC range
//	fine_total_gain = adc_gain * //V_ADC to counts
//						config.POWER_STAGE_CONFIGS[index].FINE_AMP_GAIN_VpV * //V_shunt to V_ADC
//						config.POWER_STAGE_CONFIGS[index].SHUNT_RESISTANCE; //I_shunt to V_shunt
//
//	//get the cumulative trim values that we've applied to the ADC and store into config
//	auto [total_gain_trim, total_offset_trim] = curr_fine.get_trim();
//	config.POWER_STAGE_CONFIGS[index].FINE_GAIN_TRIM = total_gain_trim;
//	config.POWER_STAGE_CONFIGS[index].FINE_OFFSET_TRIM = total_offset_trim;
//
//	//update the current LUTs
//	COMPUTE_LUTs();

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

//======================== PRIVATE FUNCTION DEFS =======================
void Sampler::COMPUTE_LUTs() {
	//####################### ADC VALUE TO CURRENT COMPUTATION - RAW ####################

	//compute the table value for every possible ADC code
	//compute the coarse and fine ranges simultaneously
	for(size_t adc_code = 0; adc_code < TABLE_SIZE; adc_code++) {
		//compute the LUT value for the particular FINE ADC code
		ADC_LUT_FINE[adc_code] = ((float)adc_code - fine_offset_counts) / fine_total_gain;

		//compute the LUT value for the particular COARSE ADC code
		ADC_LUT_COARSE[adc_code] = ((float)adc_code - coarse_offset_counts) / coarse_total_gain;
	}

	//################ CURRENT VALUES CORRESPONDING TO START AND END OF BLENDING #################
	/*
	 * This step picks off the current values affected by our blending
	 * Need to do this because we can't easily use indices to blend current for our coarse range
	 * Have to do this before we apply our blending, bc post-blend, the current values are gonna get distorted
	 */

	float blend_start_low_current = ADC_LUT_FINE[if_min + blend_counts]; //first negative-valued current we're gonna start applying our blending
	float blend_start_high_current = ADC_LUT_FINE[if_max - blend_counts]; //first positive-valued current we're gonna start applying our blending
	float blend_end_low_current = ADC_LUT_FINE[if_min]; //largest (in magnitude) negative current we CAN'T sense on the fine range
	float blend_end_high_current = ADC_LUT_FINE[if_max]; //largest (in magnitude) positive current we CAN'T sense on the fine range

	//################# APPLYING LIMITS AND BLEND TO FINE RANGE ##################

	//null out any samples including and below the valid min code
	for(size_t adc_code = 0; adc_code <= if_min; adc_code++)
		ADC_LUT_FINE[adc_code] = 0;

	//then null out any samples including and above the valid max code
	for(size_t adc_code = if_max; adc_code < TABLE_SIZE; adc_code++)
		ADC_LUT_FINE[adc_code] = 0;

	//apply a linear blending across the blend length
	//can do this on the positive and negative sides simultaneously
	for(size_t blend_index = 0; blend_index < (size_t)blend_counts; blend_index++) {
		//scale each ADC conversion as we traverse higher from the minimum value
		ADC_LUT_FINE[if_min + blend_index] *= (float)blend_index/((float)blend_counts);

		//scale each ADC conversion as we traverse lower from the maximum value
		ADC_LUT_FINE[if_max - blend_index] *= (float)blend_index/((float)blend_counts);
	}

	//################# APPLY BLEND TO COARSE RANGE ##################
	/*
	 * BLENDING THE COARSE RANGE works like this:
	 * With respect to blending, current values in the coarse range will be scaled based off which "region" they fall into
	 * These regions are as follows:
	 *  1) the lookup coarse-range current value is MORE NEGATIVE than `blend_end_low_current` --> NO SCALING HAPPENS
	 *  2) the lookup coarse-range current value is MORE POSITIVE than `blend_end_high_current` --> NO SCALING HAPPENS
	 *  3) the lookup coarse-range current value is between `blend_end_low_current` and `blend_start_low_current`
	 *  	\--> apply a linear scaling from (1, 0] based on how close it is to blend_start_low_current
	 *  4) the lookup coarse-range current value is between `blend_start_high_current` and `blend_end_high_current`
	 *  	\--> apply a linear scaling from (1, 0] based on how close it is to blend_start_high_current
	 *  5) the lookup coarse-range current value is between `blend_start_low_current` and `blend_start_high_current` (excl.) --> NULL THESE VALUES OUT
	 */

	//TODO: REMOVE AFTER DEBUGGING!!!!
	return;

	//just go through all the samples and apply blending conditionally
	for(size_t adc_code = 0; adc_code < TABLE_SIZE; adc_code++) {
		//reference the current at the particular index (REFERENCE SO WE CAN MODIFY AND REFLECT THE ORIGINAL ARRRAY)
		float& ival = ADC_LUT_COARSE[adc_code];

		//scale based off which "region" we're in --> see above
		if(ival <= blend_end_low_current); //no scaling here!
		else if(ival >= blend_end_high_current); //no scaling here too!

		else if((ival >= blend_end_low_current) && (ival < blend_start_low_current)) //scale linearly in blending area
			ival *= (ival - blend_start_low_current)/(blend_end_low_current - blend_start_low_current);

		else if((ival >= blend_start_high_current) && (ival < blend_end_high_current)) //scale linearly in blending area
			ival *= (ival - blend_start_high_current)/(blend_end_high_current - blend_start_high_current);

		else ival = 0; //we're between the `blend_start_x_current`s --> NULL OUT THESE VALUES
	}

}
