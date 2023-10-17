/*
 * app_power_stage_sampler.h
 *
 *  Created on: Oct 16, 2023
 *      Author: Ishaan
 *
 *  This class aggregates 4 ADC instances:
 *   - Coarse voltage sensing
 *   - Fine voltage sensing
 *   - Coarse current sensing
 *   - Fine current sensing
 *
 *   It's only job really is to own and initialize the 4 ADC channels and
 *   Call a callback function once all four channels have data
 *
 *   It will have functions that do the following:
 *    - attachment of a callback function
 *    - get appropriately scaled voltage and current (in units of coarse-range ADC codes, float converted)
 *    - set the fine range scaling factors, and max acceptable codes
 *    - get references to the ADC instances
 */

#ifndef POWER_STAGE_CONTROL_APP_POWER_STAGE_SAMPLER_H_
#define POWER_STAGE_CONTROL_APP_POWER_STAGE_SAMPLER_H_

#include <utility> //for pair

#include "app_hal_adc.h"
#include "app_hal_int_utils.h"

//forward declaring class so we can make it a friend here
class Sampler_Wrapper;

class Sampler {
	friend class Sampler_Wrapper; //let the wrapper access the ADC instances
public:
	//initialize with 4 ADC hardware channels
	Sampler(	Triggered_ADC::Triggered_ADC_Hardware_Channel& h_volts_fine,
				Triggered_ADC::Triggered_ADC_Hardware_Channel& h_volts_coarse,
				Triggered_ADC::Triggered_ADC_Hardware_Channel& h_curr_fine,
				Triggered_ADC::Triggered_ADC_Hardware_Channel& h_curr_coarse
			);
	//delete our copy constructor and assignment operator to avoid other issues
	Sampler(Sampler const&) = delete;
	void operator=(Sampler const&) = delete;

	//call this before anything; initialize our ADC hardware
	void init();

	//attach a callback function after all four channels have been sampled
	void attach_sample_cb(callback_function_t cb);

	/*
	 * set the fine range resolution and ADC code limits
	 * `gain` parameter describes the resolution scaling factor between coarse and fine range
	 * i.e. if fine range is 10x as fine as coarse range, pass `10` to gain
	 *
	 * We'll maintain separate limits for min and max codes since zero-offsets can make this a little funky
	 * LIMITS ARE EXCLUSIVE, I.E. READING WILL BE CONSIDERED INVALID AT THE GIVEN CODES
	 */
	constexpr void set_limits_volts_fine(const uint16_t min_code, const uint16_t max_code);
	constexpr void set_limits_curr_fine(const uint16_t min_code, const uint16_t max_code);

	/*
	 * Set scaling factors for the current and voltage channels
	 * NOTE: THIS IS AFTER OFFSETS ARE APPLIED, e.g.
	 * coarse range current sensor measures 2V at 10A, but has a 1V offset at 0V
	 * `adc_v_to_i_coarse` should be 10, not 5!
	 *
	 * ADDITIONAL NOTE: THIS FUNCTION WILL ACCUMULATE ROUNDOFF ERROR, SO CALL AS FEW TIMES AS POSSIBLE!
	 */
	void set_current_gains(const float adcv_to_i_fine, const float adcv_to_i_coarse);
	void set_voltage_gains(const float adcv_to_vterm_fine, const float adcv_to_vterm_coarse);

	/*
	 * return voltage and current ADC codes as a floating-point pair
	 * will apply the offset, and measure from the fine range if within range
	 * since the processor has an FPU, keep it simple and do the control in floating point
	 * heavily optimize since we'll be calling from control loop
	 * returns [voltage, current] in real world units (might as well, since we'll need to apply correct ADC scaling here anyway)
	 * 	gains/scaling constants specified by the ADC instances
	 */
	std::pair<float, float> __attribute__((optimize("O3"))) get_vi_readings(); //[volts, amps]

private:

	void __attribute__((optimize("O3"))) SAMPLE_ISR(); //run this as ADC callback function, heavily optimize

	//=========================== FINE RANGE MEASUREMENT LIMITS ========================

	uint16_t vf_min = 0;
	uint16_t vf_max = 0xFFFF;

	uint16_t if_min = 0;
	uint16_t if_max = 0xFFFF;

	//=========================== ADC INSTANCES ===========================

	Triggered_ADC volts_fine;
	Triggered_ADC volts_coarse;
	Triggered_ADC curr_fine;
	Triggered_ADC curr_coarse;

	//=========================== ADDITIONAL MEMBER VARIABLES ===========================

	size_t num_updates_received = 0; //if we receive readings from all channels, run the callback function
	const size_t NUM_REQUIRED_READINGS = 4; //want to make sure we get readings from all 4 ADCs before calling the callback
	callback_function_t sample_callback = empty_cb; //call this function when we've received all of our voltage/current samples

};


//=================================================== WRAPPER INTERFACE TO LIMIT ACCESS =========================================================
//access controlled version of the sampler; only allow limited adjustments to be made

class Sampler_Wrapper {
private:
	//hold onto a sampler instance
	Sampler& sampler;

public:
	enum class Measurement_Channel {
		VOLTAGE_FINE,
		VOLTAGE_COARSE,
		CURRENT_FINE,
		CURRENT_COARSE
	};

	inline constexpr void set_limits_volts_fine(const uint16_t min_code, const uint16_t max_code) {
		sampler.set_limits_volts_fine(min_code, max_code);
	}

	inline constexpr void set_limits_curr_fine(const uint16_t min_code, const uint16_t max_code) {
		sampler.set_limits_curr_fine(min_code, max_code);
	}

	inline Sampler_Wrapper(Sampler& _sampler):
			sampler(_sampler)
	{}

	inline std::pair<float, float> get_vi_readings() {
		return sampler.get_vi_readings();
	}

	inline void trim_channel(Measurement_Channel channel, float gain_trim, float offset_trim) {
		switch (channel) {
			case Measurement_Channel::VOLTAGE_FINE:
				sampler.volts_fine.trim(gain_trim, offset_trim);
				return;
			case Measurement_Channel::VOLTAGE_COARSE:
				sampler.volts_coarse.trim(gain_trim, offset_trim);
				return;
			case Measurement_Channel::CURRENT_FINE:
				sampler.curr_fine.trim(gain_trim, offset_trim);
				return;
			case Measurement_Channel::CURRENT_COARSE:
				sampler.curr_coarse.trim(gain_trim, offset_trim);
				return;
			default:
				return;
		}
	}

	inline float read_channel(Measurement_Channel channel) {
		//grab the channel we want to measure
		Triggered_ADC* adc_channel;
		switch (channel) {
			case Measurement_Channel::VOLTAGE_FINE:
				adc_channel = &sampler.volts_fine;
				break;
			case Measurement_Channel::VOLTAGE_COARSE:
				adc_channel = &sampler.volts_coarse;
				break;
			case Measurement_Channel::CURRENT_FINE:
				adc_channel = &sampler.curr_fine;
				break;
			case Measurement_Channel::CURRENT_COARSE:
				adc_channel = &sampler.curr_coarse;
				break;
			default:
				return 0;
		}

		//then grab a reading, and scale it to a meaningful value
		auto [gain, offset] = adc_channel->get_gain_offset(); //pull gain and offset for the ADC channel
		return ((float)adc_channel->get_val()) / gain - offset; //apply gain and offset correction to the ADC reading
	}
};

#endif /* POWER_STAGE_CONTROL_APP_POWER_STAGE_SAMPLER_H_ */
