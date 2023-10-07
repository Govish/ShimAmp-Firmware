/*
 * power_stage.h
 *
 *  Created on: Oct 4, 2023
 *      Author: Ishaan
 */

#ifndef POWER_STAGE_CONTROL_APP_POWER_STAGE_DRIVE_H_
#define POWER_STAGE_CONTROL_APP_POWER_STAGE_DRIVE_H_


#include <utility> //for std::pair

#include "app_hal_hrpwm.h" //to control each half bridge
#include "app_hal_dio.h" //to control the enable pin of a bridge

/* Uncommenting this because it causes linking issues (cyclical include basically)
 * I think I could forward declare the two classes, run this include, then define the classes and it might be kosher
 * idk so far seems to compile fine so not doing anything */
//#include "app_power_stage_top_level.h" //give this class friend status to the wrapper

/*
 * TODO: IMPLEMENT FAULT LINE AS EXTI OR DIRECT FAULT INPUT TO THE HRTIM
 * ATTACH A FAULT HANDLER CALLBACK FUNCTION
 *
 */

class Power_Stage {
public:
	//constructor takes references to GPIO and PWM hardware
	//delete copy constructor and assignment operator to avoid weird hardware conflicts
	Power_Stage(HRPWM& _bridge_pos, HRPWM& _bridge_neg, DIO& _bridge_en, bool _EN_ACTIVE_HIGH = true);
	Power_Stage(Power_Stage const&) = delete;
	void operator=(Power_Stage const&) = delete;

	void init(); //initialize hardware as necessary

	//bridge enable controls
	//enable/disable the HRTIM as necessary (if any instances needs the timer to be running)
	//disable --> enable transition resets the bridge to output to zero volts
	//NOTE: I don't think this class has the authority to enable/disable the HRTIM itself--other systems depend on it
	void enable();
	void disable();
	bool get_enabled();

	//setter methods that set the drive state of the power stage
	//intended to be interfaced through the controller
	bool set_drive(float drive); // -1 to 1, full negative to full positive, true if set successfully
	void set_drive_raw(int32_t drive); //basically a direct register write with polarity; minimal bounds checking for performance
	bool set_drive_halves(float drive_pos, float drive_neg); //drive each individual bridge half with particular duties, 0-1; true if set successfully
	float get_drive_duty(); //-1 to 1, whatever the bridge is currently being driven with
	int32_t get_drive_raw(); //read right from the registers, and do a little conversion to sign this number
	std::pair<float, float> get_drive_halves(); //[pos, neg] duty cycles for each half bridge, 0-1

	//setter/getter method for switching frequency (with more aggressive bounds checking)
	static bool SET_FSW(float fsw_hz); //alias for HRPWM::SET_FSW() essentially; returns true if successful
	static float GET_FSW(); //alias for HRPWM::GET_FSW() essentially

	//get control parameters relevant to the power stage, i.e.
	//maximum allowable raw commanded value (can pass positive <this_value> and negative <this_value>
	//forward path gain from the power stage when commanded with the max possible value
	std::pair<uint16_t, float> get_control_parameters();

private:
	static constexpr float POWER_STAGE_FSW_MIN = 500e3; //minimum allowable switching frequency
	static constexpr float POWER_STAGE_FSW_MAX = 2e6; //maximum allowable switching frequency

	HRPWM& bridge_pos; //positive side PWM drive
	HRPWM& bridge_neg; //negative side PWM drive
	DIO& bridge_en;
	const bool EN_ACTIVE_HIGH; //have a flag that tells us whether to drive the EN pin low or high to enable the bridge

	bool bridge_enabled; //flag that gets set/cleared according to power stage being enabled
};

//=========================== WRAPPER INTERFACE THAT IMPLEMENTS LOCK-OUT TYPE FEATURES ==============================
//implements functionality that should only be exposed over to the user interface
//instantiate one of these objects and only allow the `set` functions to work if `IS_LOCKED_OUT` is false
//`IS_LOCKED_OUT` can be controlled by the

class Power_Stage_Wrapper {
	friend class Power_Stage_Subsystem; //allow this class to manage the IS_LOCKED_OUT variable
public:

	inline Power_Stage_Wrapper(Power_Stage& _stage):
		stage(_stage)
	{}

	//##### ACCESS CONTROLLED SETTER METHODS #####
	inline bool set_drive(float drive) {
		if(!IS_LOCKED_OUT) return stage.set_drive(drive);
		else return false;
	}

	inline bool set_drive_halves(float drive_pos, float drive_neg) {
		if(!IS_LOCKED_OUT) return stage.set_drive_halves(drive_pos, drive_neg);
		else return false;
	}

	//##### GETTER METHODS DON'T REQUIRE ACCESS CONTROL #####
	inline float get_drive_duty() {
		return stage.get_drive_duty();
	}

	std::pair<float, float> get_drive_halves() {
		return stage.get_drive_halves();
	}

	//###### AUTOMATICALLY ACCESS CONTROLLED BY THE POWER STAGE ITSELF ######
	static bool SET_FSW(float fsw_hz) {
		return Power_Stage::SET_FSW(fsw_hz);
	}

	static float GET_FSW() {
		return Power_Stage::GET_FSW();
	}

	//##### getter function for the lockout status--allows for a more useful NACK message #####
	inline bool GET_LOCKED_OUT() {
		return IS_LOCKED_OUT;
	}

private:
	Power_Stage& stage;
	bool IS_LOCKED_OUT = true;
};



#endif /* POWER_STAGE_CONTROL_APP_POWER_STAGE_DRIVE_H_ */
