/*
 * app_control_biquad.h
 *
 *  Created on: Oct 18, 2023
 *      Author: Ishaan
 *
 *  Implement these IIR biquad filters using a variant of Direct Form I (DF1)
 *  	(see https://en.wikipedia.org/wiki/Digital_biquad_filter)
 *  This form seems to have the best numerical stability
 *
 */

#ifndef CONTROL_APP_CONTROL_BIQUAD_H_
#define CONTROL_APP_CONTROL_BIQUAD_H_


class Biquad {
public:

	//==================================== CREATING + STORING BIQUAD PARAMETERS ================================

	//biquad numerator and denominator coefficients
	//follow the naming convention of DF1
	struct Biquad_Params {
		//a_0 is normalized to 1
		float a_1;
		float a_2;
		float b_0;
		float b_1;
		float b_2;
		bool is_nonzero() { return a_1 || a_2 || b_0 || b_1 || b_2; }; //biquad is useful controller if there's at least one non-zero term
	};

	//========================================= BIQUAD CLASS ===============================================

	//don't really do anything in the constructor, just initialize some defaults into here
	Biquad();

	//having separate update_params function to dynamically set filter coefficients
	//mostly so we can change sampling frequency on the fly
	//NOTE: UPDATING PARAMS RESETS THE BIQUAD; DO SO ONLY WHEN BIQUAD IS INACTIVE
	virtual void update_params(const Biquad_Params new_params); //allow overrides
	Biquad_Params get_params();

	//run one IIR computation step
	//pass the input, filter will return an output
	virtual float compute(float input); //allow for override in any derived types

	//forces the filter to a steady-state value
	//default assumes the input is 0; but can force steady state to any input value
	virtual void reset(float ss_in = 0); //allow overrides

protected: //want these to be accessible by derived classes
	float xm1, xm2, ym1, ym2; //IIR memory variables; previous inputs and outputs of the filter

	Biquad_Params params; //maintain actual biquad constants
	float dc_gain; //convenience variable

};



#endif /* CONTROL_APP_CONTROL_BIQUAD_H_ */
