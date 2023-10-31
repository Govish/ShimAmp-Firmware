/*
 * app_control_compensator.h
 *
 *  Created on: Oct 18, 2023
 *      Author: Ishaan
 *
 *  The compensator class will have some computational optimizations that stem from just a single pole/zero pair
 *  NOTE: I got rid of the DC gain trim since across 9-16V operating range, the step response dynamics don't change much (just a bit of overshoot/settling time
 *  	\--> I'm thinking I'll still read in the DC supply voltage when recomputing controller gains
 *  	\--> May do this on every single DISABLE --> ENABLE transition actually, we'll see
 */

#ifndef CONTROL_APP_CONTROL_COMPENSATOR_H_
#define CONTROL_APP_CONTROL_COMPENSATOR_H_

#include <span> //to pass gains for gain trim computation function
#include <limits> //extreme float values for initialization

#include "app_control_biquad.h" //compensator is a special case biquad with just single pole and zero

class Compensator : public Biquad {
public:
	//============================= FUNCTIONS TO CREATE COMPENSATOR PARAMETERS ============================

	//build controller constants based off of some plant parameters and desired dynamics (with a pole and zero)
	//but in a dumb way (i.e. just place a single gain in the forward path)
	static Biquad_Params make_gains_dumb(	float desired_dc_gain, float f_crossover, float f_zero,
											std::span<float, std::dynamic_extent> other_loop_gains, float fs);

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
	//TODO: IMPLEMENT
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

	//additionally provide a method to set the output boundaries of the compensator
	//prevents "wind up" of the output value, theoretically making control loops more predictable
	void set_output_limits(float low_lim, float high_lim);

private:
	//have some variables that set the boundaries of the output
	//default these to be basically nonexistent
	float output_max = std::numeric_limits<float>::max();
	float output_min = -std::numeric_limits<float>::max();
};



#endif /* CONTROL_APP_CONTROL_COMPENSATOR_H_ */
