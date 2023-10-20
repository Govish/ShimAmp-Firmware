/*
 * app_control_compensator.cpp
 *
 *  Created on: Oct 18, 2023
 *      Author: Ishaan
 */

#include <algorithm> //for loop through stl containers
#include <cmath>

#include "app_utils.h" //for pi
#include "app_control_compensator.h"

//================================ STATIC METHODS ===============================

//all frequencies in Hz, all gains in normal units (i.e. not in dB)
Compensator::Biquad_Params Compensator::make_gains(	float desired_dc_gain, float f_crossover, float f_zero,
													std::span<float, std::dynamic_extent> other_loop_gains, float fs)
{
	//do some sanity checking
	if(desired_dc_gain <= 1) return {0};

	//in order to compute what we'd like our controller gain to be
	float controller_gain = desired_dc_gain;
	//divide out all the gains from our plant from the DC gain
	for(float gain : other_loop_gains) {
		controller_gain /= gain;
	}

	//compute the radian frequency of our pole given our DC gain and crossover frequency (continuous time)
	float rad_pole_freq = -(f_crossover/desired_dc_gain) * TWO_PI; //putting in left half-plane
	float rad_zero_freq = -f_zero * TWO_PI; //putting in left half-plane

	//sanity check pole and zero frequency to see if they're below nyquist
	if(fs * PI < rad_pole_freq) return {0};
	if(fs * PI < rad_zero_freq) return {0};

	//pole and zero frequencies check out; lets start converting to discrete time
	//by doing some frequency pre-warping
	//I think I'm doing this right (just need to make sure I'm not off by a factor of 2pi
	//https://ccrma.stanford.edu/~jos/fp/Frequency_Warping.html
	float warp_pole = rad_pole_freq / tan(rad_pole_freq/(2*fs));
	float warp_zero = rad_zero_freq / tan(rad_zero_freq/(2*fs));

	//run a bilinear transform, converting continuous time poles, zeros, and DC gains to discrete time
	float pole_discrete = (1 + rad_pole_freq/warp_pole) / (1 - rad_pole_freq/warp_pole);
	float zero_discrete = (1 + rad_zero_freq/warp_zero) / (1 - rad_zero_freq/warp_zero);
	float dc_gain_discrete = controller_gain / ((1 - zero_discrete) / (1 - pole_discrete)); //found by evaluating tf at z = 1

	//build the IIR coefficients from these parameters
	Biquad_Params comp_params = {
			.a_1 = -pole_discrete,
			.a_2 = 0, //don't need this term
			.b_0 = dc_gain_discrete,
			.b_1 = -dc_gain_discrete * zero_discrete,
			.b_2 = 0, //don't need this term
	};

	//and return these created parameters
	return comp_params;
}

//all frequencies in Hz, all gains in normal units (i.e. not in dB)
Compensator::Biquad_Params Compensator::make_gains(	float desired_dc_gain, float f_crossover,
													std::span<float, std::dynamic_extent> other_loop_gains, float fs)
{
	//do some sanity checking
	if(desired_dc_gain <= 1) return {0};

	//in order to compute what we'd like our controller gain to be
	float controller_gain = desired_dc_gain;
	//divide out all the gains from our plant from the DC gain
	for(float gain : other_loop_gains) {
		controller_gain /= gain;
	}

	//compute the radian frequency of our pole given our DC gain and crossover frequency (continuous time)
	float rad_pole_freq = -(f_crossover/desired_dc_gain) * TWO_PI; //putting in left half-plane

	//sanity check pole frequency to see if its below nyquist
	if(fs * PI < rad_pole_freq) return {0};

	//pole frequency check out; lets start converting to discrete time
	//by doing some frequency pre-warping
	//I think I'm doing this right (just need to make sure I'm not off by a factor of 2pi
	//https://ccrma.stanford.edu/~jos/fp/Frequency_Warping.html
	float warp_pole = rad_pole_freq / tan(rad_pole_freq/(2*fs));

	//run a bilinear transform, converting continuous time pole and DC gains to discrete time
	float pole_discrete = (1 + rad_pole_freq/warp_pole) / (1 - rad_pole_freq/warp_pole);
	float dc_gain_discrete = controller_gain * (1 - pole_discrete); //found by evaluating tf at z = 1

	//build the IIR coefficients from these parameters
	Biquad_Params comp_params = {
			.a_1 = -pole_discrete,
			.a_2 = 0, //don't need this term
			.b_0 = dc_gain_discrete,
			.b_1 = 0, //don't need this term
			.b_2 = 0, //don't need this term
	};

	//and return these created parameters
	return comp_params;
}

//================================ INSTANCE METHODS =============================

//just need to implement compute override
float Compensator::compute(float input) {
	//create a local output variable, initialized with just a gain from forward path
	//then perform the recursive filtering (single pole and zero mean just need the z^-1 terms)
	float output = input * params.b_0;
	output += xm1 * params.b_1;
	output -= ym1 * params.a_1;

	//rotate the memory elements--just need to remember the previous iteration values
	xm1 = input;
	ym1 = output;

	//adjust the output scaling by the appropriate trim constant
	//do this after the IIR filter in order to avoid cmplexity associated with rescaling memory values
	float trim_output = output * gain_trim;

	//pop out the computed compensator output
	return trim_output;
}


//and implement computing gain trim
void Compensator::compute_gain_trim(float desired_dc_loop_gain,
									std::span<float, std::dynamic_extent> other_loop_gains)
{
	//have a running product of the forward path gain
	float total_loop_gain = dc_gain; //initialize to control gain

	//take the product of all the gains in our forward path
	for(float gain : other_loop_gains) {
		total_loop_gain *= gain;
	}

	//based on our desired forward path gain and our actual gain, adjust gain trim
	//NOTE: this write should be atomic, i.e. can do this even when `compute` is running in interrupt context
	gain_trim = desired_dc_loop_gain / total_loop_gain;
}
