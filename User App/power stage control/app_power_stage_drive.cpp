/*
 * power_stage.cpp
 *
 *  Created on: Oct 4, 2023
 *      Author: Ishaan
 */

#include <app_power_stage_drive.h>

#include <algorithm> //for min, max, and clamp

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
	//bounds check
	if(drive < -1 || drive > 1) return false;

	//ensure min and max counts are updated
	get_control_parameters();

	//convert floating point drive into an int16_t drive
	//based off of min/max allowable counts
	float drive_delta = (float)(bridge_max_on_count - bridge_min_on_count);
	int16_t drive_count = (int16_t)(drive_delta * drive);

	//write to the H-bridge with the particular drive value
	set_drive_raw(drive_count);

	//and write was successful
	return true;
}

//At zero level, both bridge halves will be running at their min duty cycle
//in-phase, so will just produce a common-mode voltage at the output (which should be better than differential from a noise perspective)
//the drive will just add to the minimum `on count` to inject current into the coil
void Power_Stage::set_drive_raw(int16_t drive) {
	if(drive >= 0) {
		bridge_pos.set_duty_raw((uint16_t)drive + bridge_min_on_count); //drive positive bridge
		bridge_neg.set_duty_raw(bridge_min_on_count); //idle negative bridge
	}
	else {
		bridge_pos.set_duty_raw(bridge_min_on_count); //idle positive bridge
		bridge_neg.set_duty_raw((uint16_t)(-drive) + bridge_min_on_count); //drive negative bridge
	}
}

//NOTE: 0 means fully off, 1 means "full throttle" BUT NOT NECESSARILY 100% DUTY CYCLE
//this is to respect hardware limits
bool Power_Stage::set_drive_halves(float drive_pos, float drive_neg) {
	//bounds check, ensure drive values are between 0 and 1
	if(drive_pos < 0 || drive_pos > 1) return false;
	if(drive_neg < 0 || drive_neg > 1) return false;

	//ensure min and max counts are updated
	get_control_parameters();

	if(drive_pos == 0) bridge_pos.force_low(); //acceptable to fully turn off half-bridge
	else {
		//compute counts value to write PWM with
		//and clamp it to acceptable values
		uint16_t pos_count = std::clamp((uint16_t)(bridge_max_on_count * drive_pos),
										bridge_min_on_count, bridge_max_on_count);

		//write this raw value to the PWM channel
		//expect value saturation at low duty cycles (in order to respect minimum on-time)
		bridge_pos.set_duty_raw(pos_count);
	}

	if(drive_neg == 0) bridge_neg.force_low(); //acceptable to fully turn off half-bridge
	else {
		//compute counts value to write PWM with
		//and clamp it to acceptable values
		uint16_t neg_count = std::clamp((uint16_t)(bridge_max_on_count * drive_neg),
										bridge_min_on_count, bridge_max_on_count);

		//write this raw value to the PWM channel
		//expect value saturation at low duty cycles (in order to respect minimum on-time)
		bridge_neg.set_duty_raw(neg_count);
	}

	return true; //both sides were set successfully
}

//returns a +/- value that indicates whether stage is driving negative or positive
//normally within [-1 to 1] range but could potentially exceed that if raw drive was commanded incorrectly
float Power_Stage::get_drive_duty() {
	//ensure min and max counts are updated
	get_control_parameters();

	//pull out the raw counts value
	float raw_drive = (float)get_drive_raw();

	//figure out the drive range for "full throttle"
	float max_drive = (float)(bridge_max_on_count - bridge_min_on_count);

	//return a normalized version of the drive amount
	//can theoretically outside of [-1, 1] in weird operating regimes, so just make sure upstream software can handle
	return raw_drive / max_drive;
}

int16_t Power_Stage::get_drive_raw() {
	//get the drive value for each half bridge
	uint16_t pos_drive = bridge_pos.get_duty_raw();
	uint16_t neg_drive = bridge_neg.get_duty_raw();

	//and raw drive will just be the difference between them
	//could *technically* go outta int16_t bounds but should be fine for normal operating periods
	return (int16_t)pos_drive - (int16_t)neg_drive;
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
	//internally update control parameters too
	bridge_period_count = HRPWM::GET_PERIOD();

	//#####compute min/max counts based on fsw and HRTIM period count value
	float fsw = GET_FSW();
	float f_per_count = (float)bridge_period_count;

	//compute min duty cycle (in counts units)
	uint16_t count_min_duty = (uint16_t)(f_per_count * POWER_STAGE_DUTY_MIN);
	uint16_t count_min_ontime = (uint16_t)(f_per_count * POWER_STAGE_TON_MIN * fsw);
	bridge_min_on_count = std::max(count_min_duty, count_min_ontime); //limit to higher of these two values

	//compute max duty cycle (in counds units)
	uint16_t count_max_duty = (uint16_t)(f_per_count * POWER_STAGE_DUTY_MAX);
	uint16_t count_max_ontime = (uint16_t)(f_per_count * POWER_STAGE_TON_MAX * fsw);
	bridge_max_on_count = std::min(count_max_duty, count_max_ontime); //limit to lower of these two values

	//compute the potential range of
	uint16_t drive_delta = bridge_max_on_count - bridge_min_on_count;

	//return the abs() of the maximum power stage drive value
	//and the power stage gain associated with commanding that particular drive value
	return std::make_pair(drive_delta, (float)drive_delta/f_per_count);
}


