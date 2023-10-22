/*
 * app_setpoint_waveform_template.h
 *
 *  Created on: Oct 21, 2023
 *      Author: Ishaan
 *
 *  All types of waveform/waveshapes need to provide methods for these instances
 */

#ifndef SETPOINT_APP_SETPOINT_WAVEFORM_TEMPLATE_H_
#define SETPOINT_APP_SETPOINT_WAVEFORM_TEMPLATE_H_

class Waveform {
public:
	//nothing spicy in the default constructor
	Waveform() {}

	//this function will be called at the waveform sampling frequency (independent of sampling frequency)
	//useful for arbitrary waveform synthesis
	//will be gated based off whether the setpoint controller has been triggered or not
	virtual void tick() {}

	//this function will be called at the controller frequency
	//and is used to get the actual setpoint value
	//this value will be run through a band-limiting filter
	//a call of this function corresponds to an advance of a filter timestep
	virtual float __attribute__((optimize("O3"))) next() {return 0.0f;}

private:
};



#endif /* SETPOINT_APP_SETPOINT_WAVEFORM_TEMPLATE_H_ */
