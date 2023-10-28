/*
 * app_control_compensator.h
 *
 *  Created on: Oct 18, 2023
 *      Author: Ishaan
 *
 *  The compensator class will have some computational optimizations that stem from just a single pole/zero pair
 *  It will also have a utility function that tweaks the effective instantaneous gain of the compensator due to variations
 *  DC gains in the forward/feedback paths
 */

#ifndef CONTROL_APP_CONTROL_COMPENSATOR_H_
#define CONTROL_APP_CONTROL_COMPENSATOR_H_

#include <span> //to pass gains for gain trim computation function

#include "app_control_biquad.h" //compensator is a special case biquad with just single pole and zero

class Compensator : public Biquad {
public:
	//============================= FUNCTIONS TO CREATE COMPENSATOR PARAMETERS ============================

	//build controller constants based off of some plant parameters and desired dynamics (with a pole and zero)
	static Biquad_Params make_gains(float desired_dc_gain, float f_crossover, float f_zero,
									std::span<float, std::dynamic_extent> other_loop_gains, float fs);

	//overload of previous method, but build a compensator with just a pole
	//useful when operating in regimes when load is mostly resistive
	static Biquad_Params make_gains(float desired_dc_gain, float f_crossover,
									std::span<float, std::dynamic_extent> other_loop_gains, float fs);

	//design biquad parameters for a feed-forward transfer function
	//calculates a 'nominal' actuator command based off setpoint dynamics; controller servos around this command
	//compensates for system forward path gains and load dynamics by placing a zero at the load's pole frequency
	//places a pole near nyquist in order to keep the feed-forward system causal
	static Biquad_Params make_gains(std::span<float, std::dynamic_extent> system_gains, float load_zero, float fs);

	//============================== INSTANCE FUNCTIONS ==========================

	//constructor, just forward to Biquad
	//gain trim already initialized in declaration
	explicit Compensator() : Biquad() {};

	//overriding the compute method
	//since a compensator will only have a single pole/zero
	//can reduce the number of mults/shifts for better computational efficiency
	//can also allow for gain post-scaling (to compensate for DC gain variations
	float __attribute__((optimize("O3"))) compute(float input) override;

	//use this function to correct for slight deviations between expected and actual DC gains of the system (e.g. input voltage changing, coil DC resistance)
	//the first parameter indicates the desired dc loop gain of our entire forward path
	//the second parameter is a list of all the individual DC gains of elements of the forward path (e.g. HRPWM, POWER
	//e.g. for our case, `other_loop_gains` =
	//		{ ADC_gain (1 since real units), power_stage_counts2duty (~1/3000), duty2vout (~12, Vin), vout2current (coil DC conductance, 10 for 100mR)}
	//will compute a gain_trim to the compensator such that the DC gain of the entire forward path (incl. compensator gain) matches desired dc loop gain
	void compute_gain_trim(	float desired_dc_loop_gain,
							std::span<float, std::dynamic_extent> other_loop_gains);

private:
	float gain_trim = 1;
};



#endif /* CONTROL_APP_CONTROL_COMPENSATOR_H_ */
