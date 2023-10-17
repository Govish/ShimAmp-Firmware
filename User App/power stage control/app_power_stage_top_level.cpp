/*
 * app_power_stage_top_level.cpp
 *
 *  Created on: Oct 5, 2023
 *      Author: Ishaan
 */

#include "app_power_stage_top_level.h"

//============================= STATIC MEMBER INITIALIZATION ============================

Power_Stage_Subsystem::Configuration_Details Power_Stage_Subsystem::POWER_STAGE_CHANNEL_0 = {
		.pos_channel = HRPWM::CHANNEL_B2_PA11,
		.neg_channel = HRPWM::CHANNEL_B1_PA10,
		.en_pin_name = PinMap::status_led, /*TODO*/
		.en_active_high = true,

		.vfine = Triggered_ADC::CHANNEL_3,
		.vcoarse = Triggered_ADC::CHANNEL_3, /*TODO*/
		.ifine = Triggered_ADC::CHANNEL_3, /*TODO*/
		.icoarse = Triggered_ADC::CHANNEL_3, /*TODO*/
};

//================================= PUBLIC MEMBER FUNCTIONS =============================

//constructor
Power_Stage_Subsystem::Power_Stage_Subsystem(Power_Stage_Subsystem::Configuration_Details& config_details):
		//create a power stage instance and a corresponding wrapper
		en_pin(config_details.en_pin_name),
		chan_pwm_pos(config_details.pos_channel),
		chan_pwm_neg(config_details.neg_channel),
		stage(chan_pwm_pos, chan_pwm_neg, en_pin, config_details.en_active_high),
		stage_wrapper(stage),
		vi_sampler(config_details.vfine, config_details.vcoarse, config_details.ifine, config_details.icoarse)
{
	operating_mode = Stage_Mode::UNINITIALIZED; //start off with the stage being uninitialized
}

//################### INIT AND LOOP FUNCTIONS ###################
void Power_Stage_Subsystem::init() {
	//initialize the power stage
	//starts the stage off as disabled
	stage.init();

	//after initialization, power stage system is disabled
	operating_mode = Stage_Mode::DISABLED;
}

void Power_Stage_Subsystem::loop() {
	/*TODO, if anything*/
	//maybe check if autotuning has completed, then go back into disabled mode
}

//##################### POWER STAGE SWITCHING FREQUENCY FUNCTIONS #####################
//returns true if value successfully set
//power stage will manage bounds checking and ensuring safety of operation
bool Power_Stage_Subsystem::set_switching_frequency(float fsw_hz) {
	//TODO: update any ADC sampling/controllers/interrupt type code here
	return Power_Stage::SET_FSW(fsw_hz);
}

//get the configured switching frequency of the power stage
float Power_Stage_Subsystem::get_switching_frequency() {
	return Power_Stage::GET_FSW();
}

//################### OPERATING MODE CONTROL FUNCTIONS ###################
bool Power_Stage_Subsystem::set_mode(Stage_Mode mode) {
	//if we're already in the particular operating mode, just return; no need to update anything
	if(mode == operating_mode) return true;

	//we're changing modes
	switch(mode) {
		case Stage_Mode::UNINITIALIZED:
			return false; //can't go back into unitialized state
			break; //shouldn't run

		case Stage_Mode::DISABLED:
			/*TODO may depend on what state we're coming from */

			stage_wrapper.IS_LOCKED_OUT = true; //lock out external writes to the power stage
			stage.disable(); //always need to disable the power stage
			operating_mode = Stage_Mode::DISABLED;
			break;

		case Stage_Mode::ENABLED_MANUAL:
			//we can only go to ENABLED from DISABLED and nothing else
			if(operating_mode != Stage_Mode::DISABLED) return false;

			stage.enable(); //enable the power stage
			stage_wrapper.IS_LOCKED_OUT = false; //allow manual writes to the stage
			operating_mode = Stage_Mode::ENABLED_MANUAL;
			break;

		case Stage_Mode::ENABLED_AUTO:
			//we can only go to ENABLED from DISABLED and nothing else
			if(operating_mode != Stage_Mode::DISABLED) return false;

			stage_wrapper.IS_LOCKED_OUT = true; //lock out external writes to the power stage

			return false; //todo: NOT FULLY IMPLEMENTED YET
			break;

		case Stage_Mode::ENABLED_AUTOTUNING:
			//we can only go to ENABLED from DISABLED and nothing else
			if(operating_mode != Stage_Mode::DISABLED) return false;

			stage_wrapper.IS_LOCKED_OUT = true; //lock out external writes to the power stage

			return false; //todo: NOT FULLY IMPLEMENTED YET
			break;
	}

	return true;
}

Power_Stage_Subsystem::Stage_Mode Power_Stage_Subsystem::get_mode() {
	return operating_mode;
}

//#### GETTER FUNCTIONS ####
Power_Stage_Wrapper& Power_Stage_Subsystem::get_direct_stage_control_instance() {
	return stage_wrapper; //access controlled version of the power stage
}
