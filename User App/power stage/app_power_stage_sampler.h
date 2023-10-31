/*
 * app_power_stage_sampler.h
 *
 *  Created on: Oct 16, 2023
 *      Author: Ishaan
 *
 *  This class aggregates 2 ADC instances:
 *   - Coarse current sensing
 *   - Fine current sensing
 *
 *   It's only job really is to own and initialize the 2 ADC channels and
 *   Call a callback function once all two channels have data
 *
 *   It will have functions that do the following:
 *    - attachment of a callback function
 *    - get appropriately scaled current (in real-world units, float converted)
 *    - set the fine range scaling factors, and max acceptable codes
 *    - get references to the ADC instances
 */

#ifndef POWER_STAGE_CONTROL_APP_POWER_STAGE_SAMPLER_H_
#define POWER_STAGE_CONTROL_APP_POWER_STAGE_SAMPLER_H_

#include <utility> //for pair

#include "app_config.h"

#include "app_hal_adc.h"
#include "app_hal_int_utils.h"
#include "app_hal_hrpwm.h" //to get whether the the HRPWM is running

//forward declaring class so we can make it a friend here
class Sampler_Wrapper;

class Sampler {
public:
	//initialize with 4 ADC hardware channels
	Sampler(	Triggered_ADC::Triggered_ADC_Hardware_Channel& h_curr_fine,
				Triggered_ADC::Triggered_ADC_Hardware_Channel& h_curr_coarse,
				Configuration::Configuration_Params& _config, //allows sampler to access configuration
				const size_t _index //which power stage index this sampler belongs to
			);
	//delete our copy constructor and assignment operator to avoid other issues
	Sampler(Sampler const&) = delete;
	void operator=(Sampler const&) = delete;

	//call this before anything; initialize our ADC hardware and any kinda configuration details
	void init();

	//attach a callback function called after channels have been sampled
	void attach_sample_cb(Context_Callback_Function<> cb);

	/*
	 * ADC limits and trimming
	 * Default values for these are loaded from the active configuration file on startup (calling `init()`)
	 * However, trimming can additionally be computed during runtime
	 * Changes made will be reflected in the configuration
	 *
	 * NOTE: ADC LIMITS ARE EXCLUSIVE! Trim functions are just passthroughs to the ADC trimming basically
	 */
	bool set_limits_fine(const uint32_t min_code, const uint32_t max_code, const uint32_t blend_length);
	bool trim_fine(float gain_trim, float offset_trim);
	bool trim_coarse(float gain_trim, float offset_trim);

	//getter functions for the above methods
	//these methods will pull from configuration--ensures that configuration is as desired
	std::pair<uint32_t, uint32_t> get_limits_fine();
	std::pair<float, float> get_trim_fine();
	std::pair<float, float> get_trim_coarse();

	/*
	 * return current as a floating point measurement (in Amps)
	 * will apply the offset, and measure from the fine range if within range
	 * since the processor has an FPU, keep it simple and do the control in floating point
	 * heavily optimize since we'll be calling from control loop
	 * returns current in real world units (might as well, since we'll need to apply correct ADC scaling here anyway)
	 * 	gains/scaling constants specified by the ADC instances
	 *
	 * 	Additionally, provide interfaces to read the raw ADC values from the sampler
	 */
	float __attribute__((optimize("O3"))) get_current_reading();
	uint16_t get_raw_fine();
	uint16_t get_raw_coarse();

	/*
	 * Enable or disable the interrupt callback from running
	 * (Along with appropriate getter function)
	 */
	void enable_callback();
	void disable_callback();
	bool get_callback_enabled();

	/*
	 * get the forward path gain of our sampler (from real world current to output units)
	 * since we've decided that sampler should just output in real world units
	 * this will simply be 1
	 * and therefore having this function for posterity/allowing for a consistent interface
	 */
	float get_gain();

	//just checks if the HRPWM is enabled essentially
	//indicates whether new values are being read by the sampler
	bool get_running();

	//grab the sampling frequency from a sampler
	//DON'T ALLOW A SAMPLER TO SET SAMPLING FREQUENCY
	//	\--> A LOTTA DEPENDENCIES, SO DO THIS AT THE TOP LEVEL
	static float GET_SAMPLING_FREQUENCY();

private:
	/*
	 * CALL THIS FUNCTION TO COMPUTE OUR ADC LOOKUP TABLES
	 * Uses the following (internally maintained) parameters:
	 * 	- fine/coarse offset counts
	 * 	- fine/coarse total gain
	 * 	- TODO: BLEND START, BLEND END
	 */
	void COMPUTE_LUTs();

	//=========================== ADC RELATED VARIABLES ========================

	//##### INSTANCE #####
	Triggered_ADC curr_fine;
	Triggered_ADC curr_coarse;

	//##### FINE RANGE PARAMETERS #####
	uint16_t if_min; //code value is EXCLUSIVE i.e. this corresponds to the first INVALID code on the low side
	uint16_t if_max; //code value is EXCLUSIVE i.e. this corresponds to the first INVALID code on the high side
	uint16_t blend_counts; //this many counts from if_min, and if_max

	//##### GAIN AND OFFSET CONSTANTS #####
	//these are sums/products of application related gains/offsets as well as ADC trimming constants
	float fine_offset_counts;
	float coarse_offset_counts;
	float fine_total_gain;
	float coarse_total_gain;

	/*
	 * ADC COUNT TO CURRENT LOOKUP TABLES
	 * A speculation, but I implementing ADC to current conversion as a LUT
	 * should be faster than doing even the relatively simple floating-point arithmetic
	 * Also has a byproduct of letting us implement a "cross-fader" that gradually switches between the fine and coarse ranges
	 */
	static const size_t TABLE_SIZE = (size_t)Triggered_ADC::get_adc_max_code() + 1; //should be 12 bits
	std::array<float, TABLE_SIZE> ADC_LUT_FINE = {0};
	std::array<float , TABLE_SIZE> ADC_LUT_COARSE = {0};


	//=========================== ADDITIONAL MEMBER VARIABLES ===========================

	bool callback_enable = false; //doesn't actively control behavior, just a flag essentially

	Configuration::Configuration_Params& config; //configuration structure to read/write
	const size_t index;

};


//=================================================== WRAPPER INTERFACE TO LIMIT ACCESS =========================================================
//access controlled version of the sampler; only allow limited adjustments to be made

class Sampler_Wrapper {
private:
	//hold onto a sampler instance
	Sampler& sampler;

public:

	//================= CONSTRUCTORS AND OPERATORS =================
	inline Sampler_Wrapper(Sampler& _sampler):
				sampler(_sampler)
		{}

	//delete copy constructor and assignment operator to avoid any weird issues
	Sampler_Wrapper(Sampler_Wrapper const&) = delete;
	void operator=(Sampler_Wrapper const&) = delete;

	//=================== INSTANCE GETTERS =================

	inline bool get_running() {return sampler.get_running();}
	inline float get_current_reading() {return sampler.get_current_reading();}

	//##### RAW ADC READS #####
	//don't clear the recently_updated flag since these are just diagnostic
	inline uint16_t read_fine_raw() {return sampler.get_raw_fine();}
	inline uint16_t read_coarse_raw() {return sampler.get_raw_coarse();}

	//==================== INSTANCE SETTERS =================

	inline bool set_limits_fine(const uint32_t min_code, const uint32_t max_code, const uint32_t blend_length) {
		return sampler.set_limits_fine(min_code, max_code, blend_length);
	} //forward to sampler

	inline bool trim_fine(float gain_trim, float offset_trim) { return sampler.trim_fine(gain_trim, offset_trim);} //forward to sampler
	inline bool trim_coarse(float gain_trim, float offset_trim) { return sampler.trim_coarse(gain_trim, offset_trim);} //forward to sampler
	inline std::pair<uint32_t, uint32_t> get_limits_fine() { return sampler.get_limits_fine(); }
	inline std::pair<float, float> get_trim_fine() { return sampler.get_trim_fine(); }
	inline std::pair<float, float> get_trim_coarse() { return sampler.get_trim_coarse(); }

};

#endif /* POWER_STAGE_CONTROL_APP_POWER_STAGE_SAMPLER_H_ */
