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
	friend class Sampler_Wrapper; //let the wrapper access the ADC instances
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

	//attach a callback function after all four channels have been sampled
	void attach_sample_cb(callback_function_t cb);

	/*
	 * ADC limits and trimming
	 * Default values for these are loaded from the active configuration file on startup (calling `init()`)
	 * However, trimming can additionally be computed during runtime
	 * Changes made will be reflected in the configuration
	 *
	 * NOTE: ADC LIMITS ARE EXCLUSIVE! Trim functions are just passthroughs to the ADC trimming basically
	 */
	void set_limits_fine(const uint16_t min_code, const uint16_t max_code);
	void trim_fine(float gain_trim, float offset_trim);
	void trim_coarse(float gain_trim, float offset_trim);

	/*
	 * return current as a floating point measurement (in Amps)
	 * will apply the offset, and measure from the fine range if within range
	 * since the processor has an FPU, keep it simple and do the control in floating point
	 * heavily optimize since we'll be calling from control loop
	 * returns current in real world units (might as well, since we'll need to apply correct ADC scaling here anyway)
	 * 	gains/scaling constants specified by the ADC instances
	 */
	float __attribute__((optimize("O3"))) get_current_reading();

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

	void __attribute__((optimize("O3"))) SAMPLE_ISR(); //run this as ADC callback function, heavily optimize

	//=========================== ADC RELATED VARIABLES ========================

	//##### INSTANCE #####
	Triggered_ADC curr_fine;
	Triggered_ADC curr_coarse;

	//##### FINE RANGE PARAMETERS #####
	uint16_t if_min = 0;
	uint16_t if_max = 0xFFFF;

	//##### GAIN AND OFFSET CONSTANTS #####
	//these are sums/products of application related gains/offsets as well as ADC trimming constants
	float fine_offset_counts;
	float coarse_offset_counts;
	float fine_total_gain;
	float coarse_total_gain;

	//=========================== ADDITIONAL MEMBER VARIABLES ===========================

	size_t num_updates_received = 0; //if we receive readings from all channels, run the callback function
	const size_t NUM_REQUIRED_READINGS = 2; //want to make sure we get readings from both current channels before running the callback
	callback_function_t sample_callback = empty_cb; //call this function when we've received all of our samples
	bool callback_enable = false; //whether we should actually run the user callback function

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
	inline uint16_t read_fine_raw() {return sampler.curr_fine.get_val(false);}
	inline uint16_t read_coarse_raw() {return sampler.curr_coarse.get_val(false);}

	//==================== INSTANCE SETTERS =================
	inline void set_limits_fine(const uint16_t min_code, const uint16_t max_code) {sampler.set_limits_fine(min_code, max_code);} //forward to sampler
	inline void trim_fine(float gain_trim, float offset_trim) {sampler.trim_fine(gain_trim, offset_trim);} //forward to sampler
	inline void trim_coarse(float gain_trim, float offset_trim) {sampler.trim_coarse(gain_trim, offset_trim);} //forward to sampler
};

#endif /* POWER_STAGE_CONTROL_APP_POWER_STAGE_SAMPLER_H_ */
