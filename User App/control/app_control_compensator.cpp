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

//use the load pole to set the bandwidth of the controller;
//do so by just adding some proportional gain
Compensator::Biquad_Params Compensator::make_gains_dumb(	float desired_dc_gain, float f_crossover, float f_zero,
															std::span<float, std::dynamic_extent> other_loop_gains, float fs)
{
	//do some sanity checking
	if(f_zero > f_crossover) return {0};

	//compute how much DC gain our forward path needs to place our cutoff frequency where we want
	float required_dc_gain = f_crossover / f_zero;

	//and now calculate the required controller gain based off all the gains in our forward path
	float dc_gain_discrete = required_dc_gain;
	for(float gain : other_loop_gains) {
		dc_gain_discrete /= gain;
	}

	//build the IIR coefficients from these parameters
	Biquad_Params comp_params = {
			.a_1 = 0,
			.a_2 = 0, //don't need this term
			.b_0 = dc_gain_discrete,
			.b_1 = 0,
			.b_2 = 0, //don't need this term
	};

	//and return these created parameters
	return comp_params;
}

//all frequencies in Hz, all gains in normal units (i.e. not in dB)
Compensator::Biquad_Params Compensator::make_gains(	float desired_dc_gain, float f_crossover, float f_zero,
													std::span<float, std::dynamic_extent> other_loop_gains, float fs)
{
	//do some sanity checking
	if(desired_dc_gain <= 1) return {0};
	if(f_crossover * 5 > fs) return {0}; //hard to guarantee controller efficacy when crossover is this close to fs

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
	if(f_crossover * 5 > fs) return {0}; //hard to guarantee controller efficacy when crossover is this close to fs

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

//update our bounds variables
void Compensator::set_output_limits(float low_lim, float high_lim) {
	output_min = low_lim;
	output_max = high_lim;
}

//just need to implement compute override
//NOTE: I got rid of gain trim in order to speed up the computations
float Compensator::compute(float input) {
	//create a local output variable, initialized with just a gain from forward path
	//then perform the recursive filtering (single pole and zero mean just need the z^-1 terms)
	float output = input * params.b_0;
	output += xm1 * params.b_1;
	output -= ym1 * params.a_1;

	//constrain the output to our bounds
	output = std::clamp(output, output_min, output_max);

	//rotate the memory elements--just need to remember the previous iteration values
	xm1 = input;
	ym1 = output; //will store our constrained output; avoids wind-up

	//pop out the computed compensator output
	return output; //std::clamp(output, output_min, output_max);
}


