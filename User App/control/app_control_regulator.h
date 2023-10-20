/*
 * app_control_regulator.h
 *
 *  Created on: Oct 19, 2023
 *      Author: Ishaan
 *
 *  Provides a bit closer to canonical wrapper for a current regulator
 *  The compensator takes a little more generalized of a form, but
 *  the regulator is a little more specific to our current hardware
 */

#ifndef CONTROL_APP_CONTROL_REGULATOR_H_
#define CONTROL_APP_CONTROL_REGULATOR_H_

#include <stddef.h> //for size_t

#include "app_config.h" //access the configuration information
#include "app_control_compensator.h"

#include "app_power_stage_drive.h" //grab stuff related to the output
#include "app_power_stage_sampler.h" //grab stuff related to the input

class Regulator {
public:
	//create a regulator, passing it instances of everything here
	Regulator(	Power_Stage& _stage, Sampler& _sampler, //TODO: also pass it a setpoint controller, supply voltage measurement channel
				Configuration::Configuration_Params& _params, const size_t _index);

	//initialize everything necessary for the current regulator to function
	void init();

	//enable/disable the regulator (and have an appropriate `getter` function)
	void enable();
	void disable();
	bool get_enabled();

	//update the compensator coefficients given a new sampling frequency
	//this function pulls information from the stage and sampler and shouldn't need any parameters
	bool recompute_rate(); //remember to forbid this operation if regulator is enabled

	//call periodically at a lower priority to update the compensator DC gain based off supply voltage
	void trim_gain();

private:
	//================= MAIN REGULATION FUNCTION; CALLED BY SAMPLER ==================
	void regulate();

	//============================ MEMBER CLASS REFERENCES AND INSTANCES ============================
	Power_Stage& stage; //maintain a reference to a power stage
	Sampler& sampler; //maintain a reference to a sampler
	Configuration::Configuration_Params& params; //access the active configuration structure
	Compensator comp; //this class will own the compensator

	//============================ OTHER MEMBER VARIABLES ============================
	const size_t index; //which power stage this regulator corresponds to
	bool enabled = false; //local variable to hold whether the regulator is enabled or not
};



#endif /* CONTROL_APP_CONTROL_REGULATOR_H_ */
