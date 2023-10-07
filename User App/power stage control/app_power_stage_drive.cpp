/*
 * power_stage.cpp
 *
 *  Created on: Oct 4, 2023
 *      Author: Ishaan
 */

#include <app_power_stage_drive.h>

//drop the passed references into the class
Power_Stage::Power_Stage(HRPWM& _bridge_pos, HRPWM& _bridge_neg, DIO& _bridge_en, bool _EN_ACTIVE_HIGH):
	bridge_pos(_bridge_pos),
	bridge_neg(_bridge_neg),
	bridge_en(_bridge_en),
	EN_ACTIVE_HIGH(_EN_ACTIVE_HIGH)
{}

//initialize hardware as necessary
void Power_Stage::init() {
	bridge_pos.init();
	bridge_neg.init();

	bridge_en.init(); //ensure IO is initialized
	disable(); //disable the power stage on startup
}

//bridge enable controls
//enable/disable the HRTIM as necessary (if any instances needs the timer to be running)
//disable --> enable transition resets the bridge to output to zero volts
void Power_Stage::enable() {
	//enable the PWM channels
	bridge_pos.enable();
	bridge_neg.enable();

	//force the PWM output into a safe state before actually enabling the power stage ICs
	set_drive(0);

	//assert the enable pin headed to the bridge IC
	if(EN_ACTIVE_HIGH) bridge_en.set();
	else bridge_en.clear();
}

void Power_Stage::disable() {
	//disable the bridge IC by deasserting the enable pin
	if(EN_ACTIVE_HIGH) bridge_en.clear();
	else bridge_en.set();

	//disable the PWM channels
	bridge_pos.disable();
	bridge_neg.disable();

	//indicate that the power stage is disabled
	bridge_enabled = false;
}

bool Power_Stage::get_enabled() {
	return bridge_enabled;
}

//setter methods that set the drive state of the power stage
//intended to be interfaced through the controller
bool Power_Stage::set_drive(float drive) {
	//TODO: figure out modulation strategy and fill in
	return false;
}

void Power_Stage::set_drive_raw(int32_t drive) {
	//TODO: figure out modulation strategy and fill in
}

//NOTE: function will
bool Power_Stage::set_drive_halves(float drive_pos, float drive_neg) {
	auto prev_pos_raw_duty = bridge_pos.get_duty_raw(); //read what the positive bridge is set at beforehand
	if(!bridge_pos.set_duty(drive_pos)) //and set the positive bridge to the desired duty cycle value
		return false; //if the setting wasn't successful don't even try to set the negative half

	//setting the positive half was successful, set the negative half now
	if(!bridge_neg.set_duty(drive_neg)) { //check if setting the negative half was successful
		bridge_pos.set_duty_raw(prev_pos_raw_duty); //if not, reset the value in the positive half bridge
		return false;
	}

	return true; //both sides were set successfully
}

float Power_Stage::get_drive_duty() {
	return 0; //TODO: figure out modulation strategy; return appropriate value
}

int32_t Power_Stage::get_drive_raw() {
	return 0; //TODO: figure out modulation strategy
}

std::pair<float, float> Power_Stage::get_drive_halves() {
	float pos_drive = bridge_pos.get_duty();
	float neg_drive = bridge_neg.get_duty();
	return std::make_pair(pos_drive, neg_drive);
}

//setter/getter method for switching frequency
bool Power_Stage::SET_FSW(float fsw_hz) {
	if(fsw_hz < POWER_STAGE_FSW_MIN || fsw_hz > POWER_STAGE_FSW_MAX) return false; //more aggressive circuitry bounds here
	return HRPWM::SET_FSW(fsw_hz);
}

float Power_Stage::GET_FSW() {
	return HRPWM::GET_FSW();
}

//get control parameters relevant to the power stage, i.e.
//maximum allowable raw commanded value (can pass positive <this_value> and negative <this_value>
//forward path gain from the power stage when commanded with the max possible value
std::pair<uint16_t, float> Power_Stage::get_control_parameters() {
	return std::make_pair(0, 0.0f); //TODO: figure out modulation strategy
}


