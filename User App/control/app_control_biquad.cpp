/*
 * app_control_biquad.cpp
 *
 *  Created on: Oct 18, 2023
 *      Author: Ishaan
 *
 *
 *
 */

#include "app_control_biquad.h"
#include "app_utils.h" //for pi

//================== FILTER CONSTANT CREATION METHODS ================

// ported from https://arachnoid.com/BiQuadDesigner/javascript/BiQuadFilter.js
//I'm not sure whether this implementation does frequency pre-warping, but the error should be small
//for corner frequencies far from nyquist
Biquad::Biquad_Params Biquad::make_lowpass(float corner_freq, float Q, float sampling_freq) {
	//return zero constants if our corner frequency is past nyquist
	//NOTE: IN A HEAVILY OVERDAMPED FILTER, THE FAST POLE CAN POTENTIALLY BE PAST NYQUIST
	//I HAVEN'T SANITY CHECKED THAT EDGE CASE, TODO NOTING THAT HERE
	if(corner_freq * 2 > sampling_freq) return {0};

	float omega = TWO_PI * corner_freq / sampling_freq;
	float alpha = std::sin(omega) / (2.0f * Q);
	float cos_omega = std::cos(omega);
	float a_0 = 1 + alpha;

	//normalize with respect to a_0
	Biquad_Params lp_params = {
			.a_1 = -2.0f * cos_omega / a_0,
			.a_2 = (1 - alpha) / a_0,
			.b_0 = (1 - cos_omega) / (2.0f * a_0),
			.b_1 = (1 - cos_omega) / a_0,
			.b_2 = (1 - cos_omega) / (2.0f * a_0),
	};

	//return the created parameters
	return lp_params;
}

//========================= MEMBER FUNCTIONS ======================

Biquad::Biquad() {
	//initialize filter params to zero
	params = {0};
	dc_gain = 0;

	//force steady-state to a zero-input value
	reset();
}

void Biquad::update_params(Biquad::Biquad_Params new_params) {
	//update the params by copy
	params = new_params;

	//compute the new DC gain; solve biquad polynomial for z = 1
	dc_gain = (params.b_0 + params.b_1 + params.b_2) / (1 + params.a_1 + params.a_2);

	//force a reset given the new parameters
	reset();
}

//simple getter function that returns the biquad coefficients
Biquad::Biquad_Params Biquad::get_params() {
	return params;
}

float Biquad::compute(float input) {
	//run the computation
	float output = input * params.b_0;
	output += xm1 * params.b_1 + xm2 * params.b_2;
	output -= ym1 * params.a_1 - ym2 * params.a_2;

	//shift stuff over by 1
	xm2 = xm1;
	xm1 = input;
	ym2 = ym1;
	ym1 = output;

	//return the computed value
	return output;
}

void Biquad::reset(float ss_in) {
	//previous inputs will just be the steady-state input value
	xm1 = ss_in;
	xm2 = ss_in;

	//and previous outputs will just be the DC value * DC gain
	ym1 = ss_in * dc_gain;
	ym2 = ss_in * dc_gain;
}
