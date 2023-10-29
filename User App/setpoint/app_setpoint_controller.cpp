/*
 * app_setpoint_controller.cpp
 *
 *  Created on: Oct 21, 2023
 *      Author: Ishaan
 */


#include "app_setpoint_controller.h"

Setpoint::Setpoint(Configuration::Configuration_Params& _params, const size_t _index):
	params(_params),
	index(_index),

	//INITIALIZE ALL OUR WAVEFORM GENERATOR INSTANCES
	drive_dc(_params.POWER_STAGE_CONFIGS[_index].CHANNEL_MAX_CURRENT) //initialize with the maximum acceptable current
{
	//initialize our waveform pointers such that they all point to a setpoint drive of 0
	active_waveform = &zero_drive;
	trigger_asserted_waveform = &zero_drive;
	trigger_deasserted_waveform = &zero_drive;
}


//============= PUBLIC METHODS ===========

void Setpoint::init() {
	//initialize the reconstruction filter with the appropriate constants
	recompute_rate(params.POWER_STAGE_CONFIGS[index].SETPOINT_RECON_BANDWIDTH);

	//TODO: initialize trigger and tick hardware
}

//###### ENABLE CONTROL ######
void Setpoint::enable() {
	if(enabled) return;
	enabled = true; //set the enabled flag
}

void Setpoint::disable() {
	if(!enabled) return;

	//reset the active setpoint (and triggered ones) to the zero-drive one
	active_waveform = &zero_drive;
	trigger_asserted_waveform = &zero_drive;
	trigger_deasserted_waveform = &zero_drive;

	//reset the band-limiting filter
	bl_filter.reset();
	enabled = false; //clear the enabled flag
}

bool Setpoint::get_enabled() {
	return enabled;
}

//####### CONTROLLER RATE RECOMPUTATION #######
bool Setpoint::recompute_rate(float _bl_corner_freq) {
	//forbid this operation if we're enabled
	if(enabled) return false;

	//recompute biquad filter constants based on the new sample rate
	Biquad::Biquad_Params bl_filter_params = bl_filter.make_lowpass(_bl_corner_freq,
																	BESSEL_Q,
																	Sampler::GET_SAMPLING_FREQUENCY());
	//check if filter creation failed
	if(!bl_filter_params.is_nonzero()) return false;

	//everything went right; update the filter and the global configuration as necessary
	bl_filter.update_params(bl_filter_params);
	params.POWER_STAGE_CONFIGS[index].SETPOINT_RECON_BANDWIDTH = _bl_corner_freq;

	return true;
}

//###### SETPOINT SERVICE FUNCTION ######
float Setpoint::next() {
	//grab the next value from the active waveform
	float next_wave = active_waveform->next();

	//run it through our filter
	float filt_next_wave = bl_filter.compute(next_wave);

	//and return that filtered value
	//TODO: FIX BESSEL FILTER MAYBE
	//return filt_next_wave;
	return next_wave;
}

//=========================== WAVEFORM INSTANTIATIONS ==========================
bool Setpoint::reset_setpoint() {
	//only allow this when the controller is enabled
	if(!enabled) return false;

	//just force all of our waveforms to be zero
	active_waveform = &zero_drive;
	trigger_asserted_waveform = &zero_drive;
	trigger_deasserted_waveform = &zero_drive;

	//everything went according to plan
	return true;
}

bool Setpoint::make_setpoint_dc(bool trigger_gated, float setpoint) {
	//only allow this when the controller is enabled
	if(!enabled) return false;

	//try to configure the dc drive with the desired dc setpoint
	if(!drive_dc.configure(setpoint)) return false;

	//if that goes according to plan, act on the trigger gating
	if(trigger_gated)
		return false; //NOT IMPLEMENTED YET

	//not trigger gated
	//TODO: DISABLE TRIGGER EXTI
	//just drive DC until we're either disabled or want to set up a new setpoint
	active_waveform = &drive_dc;
	trigger_asserted_waveform = &drive_dc;
	trigger_deasserted_waveform = &drive_dc;

	//everything went according to plan
	return true;
}

//======================== PRIVATE FUNCTIONS + ISRs =======================
//CALLED FROM FIXED_FREQUENCY TIMER_ISR
void Setpoint::tick() {
	active_waveform->tick();
}

//CALLED ON A POSITIVE-GOING EDGE OF TRIGGER INPUT
void Setpoint::trigger_assert() {
	active_waveform = trigger_asserted_waveform;
	//TODO: RESET COUNT OF TICK TIMER
}

//CALLED ON A NEGATIVE_GOING EDGE OF TRIGGER INPUT
void Setpoint::trigger_deassert() {
	active_waveform = trigger_deasserted_waveform;
	//TODO: RESET COUNT OF TICK TIMER
}
