/*
 * app_power_stage_top_level.cpp
 *
 *  Created on: Oct 5, 2023
 *      Author: Ishaan
 */

#include "app_power_stage_top_level.h"

#include <functional> //for std::ref


//============================= STATIC MEMBER INITIALIZATION ============================

size_t Power_Stage_Subsystem::INSTANCE_COUNT = 0;
std::array<Power_Stage_Subsystem*, Configuration::POWER_STAGE_COUNT> Power_Stage_Subsystem::ALL_POWER_STAGES = {};

Power_Stage_Subsystem::Channel_Hardware_Details Power_Stage_Subsystem::POWER_STAGE_CHANNEL_0 = {
		.pos_channel = HRPWM::CHANNEL_B2_PA11,
		.neg_channel = HRPWM::CHANNEL_B1_PA10,
		.en_pin_name = PinMap::status_led, /*TODO*/
		.en_active_high = true,

		.ifine = Triggered_ADC::CHANNEL_3,
		.icoarse = Triggered_ADC::CHANNEL_3, /*TODO*/
};

//================================= PUBLIC MEMBER FUNCTIONS =============================

//constructor
Power_Stage_Subsystem::Power_Stage_Subsystem(Power_Stage_Subsystem::Channel_Hardware_Details& hardware_details, Configuration::Configuration_Params& _config, const size_t _CHANNEL_NUM):
		//create a power stage instance and a corresponding wrapper
		stage(hardware_details.pos_channel, hardware_details.neg_channel, hardware_details.en_pin_name, hardware_details.en_active_high),
		stage_wrapper(stage),

		//create a sampler and its corresponding wrapper
		current_sampler(hardware_details.ifine, hardware_details.icoarse, _config, _CHANNEL_NUM),
		current_sampler_wrapper(current_sampler),

		//instantiate the regulator and pass it the power stage and sampler
		regulator(stage, current_sampler, _config, _CHANNEL_NUM),

		config(_config), //reference the active configuration happening here
		CHANNEL_NUM(_CHANNEL_NUM) //and store the channel number we've been labeled
{
	//create a pointer to the instance we just created (helps when calling all instances from static function)
	ALL_POWER_STAGES[INSTANCE_COUNT] = this;
	INSTANCE_COUNT++;

	//start the power stage off in this mode
	operating_mode = Stage_Mode::UNINITIALIZED; //start off with the stage being uninitialized
}

//################### INIT AND LOOP FUNCTIONS ###################
void Power_Stage_Subsystem::init() {
	//initialize the power stage
	//starts the stage off as disabled
	stage.init();

	//initialize the sampler
	current_sampler.init();

	//initialize our regulator
	regulator.init();

	//and initialize the controller and switching frequencies to what the config says
	set_operating_frequencies(config.DESIRED_SWITCHING_FREQUENCY, config.DESIRED_SAMPLING_FREQUENCY);

	//after initialization, power stage system is disabled
	operating_mode = Stage_Mode::DISABLED;
}

void Power_Stage_Subsystem::loop() {
	/*TODO, if anything*/
	//maybe check if autotuning has completed, then go back into disabled mode
}

//##################### POWER STAGE SWITCHING/SAMPLING FREQUENCY FUNCTIONS #####################

bool Power_Stage_Subsystem::recompute_rates(float fsw_hz, float fc_hz) {
	//update the configuration with our desired frequencies (if we didn't pass in bogus values)
	//have to update configs here since config is tied to an instance
	if(fsw_hz > 0) config.DESIRED_SWITCHING_FREQUENCY = fsw_hz;
	if(fc_hz > 0) config.DESIRED_SAMPLING_FREQUENCY = fc_hz;

	//additionally, recompute some regulation constants given this new switching frequency
	//this will take care of adjusting the ADC trigger frequency and any other regulation-related functions
	return regulator.recompute_rate();
}

//forwards to `set_operating_frequencies`
bool Power_Stage_Subsystem::set_controller_frequency(float fc_hz) {
	//don't update the switching frequency
	return set_operating_frequencies(-1, fc_hz);
}

//forwards to `set_operating_frequencies`
bool Power_Stage_Subsystem::set_switching_frequency(float fsw_hz) {
	//don't update the controller frequency
	return set_operating_frequencies(fsw_hz, -1);
}

//returns true if successfully set
//Power Stage and HRPWM will manage bounds checking and safety of operation
bool Power_Stage_Subsystem::set_operating_frequencies(float fsw_hz, float fc_hz) {
	//attempt to set the switching frequency if we wanna update this
	if(fsw_hz > 0)
		if(!Power_Stage::SET_FSW(fsw_hz))
			return false; //don't proceed if that didn't go right

	//attempt to set the controller/sampling frequency if we wanna update this
	if(fc_hz > 0)
		if(!HRPWM::SET_ADC_TRIGGER_FREQUENCY(fc_hz))
			return false;

	//globally updated the frequencies that we wanted, now just update the instances
	for(Power_Stage_Subsystem* sys : ALL_POWER_STAGES) {
		//if updating a single channel doesn't go right, just leave
		if(sys != nullptr)
			if(!sys->recompute_rates(fsw_hz, fc_hz)) return false;
		//TODO: gracefully handle resetting everything if a single update doesn't work?
	}

	//all controllers were successfully updated
	return true;
}

//get the configured switching frequency of the power stage
float Power_Stage_Subsystem::get_switching_frequency() {
	return Power_Stage::GET_FSW();
}

//get the actual frequency of the controller
float Power_Stage_Subsystem::get_controller_frequency() {
	return HRPWM::GET_ADC_TRIGGER_FREQUENCY();
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

		case Stage_Mode::DISABLED: //============================= DISABLE THE POWER STAGE ============================
			//disable and lock out the power stage
			stage_wrapper.IS_LOCKED_OUT = true;
			stage.disable();

			//disable the regulator
			regulator.disable();

			//update the state
			operating_mode = Stage_Mode::DISABLED;
			break;

		case Stage_Mode::ENABLED_MANUAL: //====================== ENABLE STAGE FOR MANUAL CONTROL ======================
			//we can only go to ENABLED from DISABLED and nothing else
			if(operating_mode != Stage_Mode::DISABLED) return false;

			stage.enable(); //enable the power stage
			stage_wrapper.IS_LOCKED_OUT = false; //allow manual writes to the stage
			operating_mode = Stage_Mode::ENABLED_MANUAL;
			break;

		case Stage_Mode::ENABLED_AUTO: //====================== ENABLE STAGE UNDER AUTOMATIC CONTROL ======================
			//we can only go to ENABLED from DISABLED and nothing else
			if(operating_mode != Stage_Mode::DISABLED) return false;

			//ensure power stage is locked out to external writes
			stage_wrapper.IS_LOCKED_OUT = true;

			//just need to enable the regulator, this should take care of everything
			regulator.enable();

			return true;
			break;

		case Stage_Mode::ENABLED_AUTOTUNING: //====================== AUTOTUNE THIS PARTICULAR POWER STAGE CHANNEL ======================
			//we can only go to ENABLED from DISABLED and nothing else
			if(operating_mode != Stage_Mode::DISABLED) return false;

			stage_wrapper.IS_LOCKED_OUT = true; //lock out external writes to the power stage

			//need to figure out if I need to disable ALL the stages and just tune one channel at a time

			return false; //todo: NOT FULLY IMPLEMENTED YET
			break;
	}

	return true;
}

//######################## GETTER FUNCTIONS #######################

Power_Stage_Subsystem::Stage_Mode Power_Stage_Subsystem::get_mode() {
	return operating_mode;
}

Power_Stage_Wrapper& Power_Stage_Subsystem::get_direct_stage_control_instance() {
	return stage_wrapper; //access controlled version of the power stage
}

Sampler_Wrapper& Power_Stage_Subsystem::get_sampler_instance() {
	return current_sampler_wrapper; //access controlled verison of the sampler
}
