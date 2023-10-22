/*
 * app_setpoint_waveform_dc.h
 *
 *  Created on: Oct 21, 2023
 *      Author: Ishaan
 */

#ifndef SETPOINT_APP_SETPOINT_WAVEFORM_DC_H_
#define SETPOINT_APP_SETPOINT_WAVEFORM_DC_H_

//This is a type of waveform
#include "app_setpoint_waveform_template.h"

class DC_Waveform : public Waveform {
public:
	//establish boundaries for reasonable current regulation values in the constructor
	DC_Waveform(float _MAX_MAG_SETPOINT);

	//configure the waveform with a DC current value
	//return true if this setpoint is within limits
	bool configure(float setpoint);

	//don't need to do anything for tick

	//just return the DC value when called for
	float __attribute__((optimize("O3"))) next() override;

private:
	//remember the safe current limits when we constructed this instance
	const float MAX_MAG_SETPOINT;

	//initialize the DC value to something safe
	float dc_value = 0;
};



#endif /* SETPOINT_APP_SETPOINT_WAVEFORM_DC_H_ */
