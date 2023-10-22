/*
 * app_setpoint_waveform_dc.cpp
 *
 *  Created on: Oct 21, 2023
 *      Author: Ishaan
 */

#include "app_setpoint_waveform_dc.h"

//establish boundaries for reasonable current regulation values in the constructor
DC_Waveform::DC_Waveform(float _MAX_MAG_SETPOINT):
	MAX_MAG_SETPOINT(_MAX_MAG_SETPOINT)
{}

//configure the waveform with a DC current value
//return true if this setpoint is within limits
bool DC_Waveform::configure(float setpoint) {
	//bounds check the setpoint
	if(setpoint < -MAX_MAG_SETPOINT || setpoint > MAX_MAG_SETPOINT) return false;

	//if the setpoint value is within bounds, hold onto it
	dc_value = setpoint;
	return true; //everything went right
}

//just return the DC value when called for
float DC_Waveform::next() {
	return dc_value;
}

