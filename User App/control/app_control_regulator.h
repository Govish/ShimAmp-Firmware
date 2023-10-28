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
#include "app_setpoint_controller.h" //grab stuff related to the setpoint

class Regulator {
public:
	//create a regulator, passing it instances of everything here
	Regulator(	Power_Stage& _stage, Sampler& _sampler, Setpoint& _setpoint, //TODO: supply voltage measurement channel
				Configuration::Configuration_Params& _params, const size_t _index);

	//initialize everything necessary for the current regulator to function
	void init();

	//enable/disable the regulator (and have an appropriate `getter` function)
	void enable();
	void disable();
	bool get_enabled();

	//update the compensator coefficients given a new sampling frequency
	//this function pulls information from the stage and sampler and shouldn't need any parameters
	bool recompute_rate(float desired_dc_gain,
						float desired_crossover_freq,
						float load_resistance,
						float load_natural_freq);

	//update control parameters, one variable at a time
	bool update_gain(float new_gain);
	bool update_crossover_freq(float new_crossover_freq);
	bool update_load_resistance(float new_load_resistance);
	bool update_load_natural_freq(float new_load_natural_freq);

	//get control parameters (likely just going to be reading from configuration
	float get_gain();
	float get_crossover_freq();
	float get_load_resistance();
	float get_load_natural_freq();

	//call periodically at a lower priority to update the compensator DC gain based off supply voltage
	void __attribute__((optimize("O3"))) trim_gain(); //still want this to be kinda fast

private:
	//================= MAIN REGULATION FUNCTION; CALLED BY SAMPLER ==================
	static void __attribute__((optimize("O3"))) regulate_forwarder(void* context);
	void __attribute__((optimize("O3"))) regulate();

	//============================ MEMBER CLASS REFERENCES AND INSTANCES ============================
	Power_Stage& stage; //maintain a reference to a power stage
	Sampler& sampler; //maintain a reference to a sampler
	Setpoint& setpoint; //maintain a reference to a setpoint controller
	Configuration::Configuration_Params& params; //access the active configuration structure
	Compensator comp; //this class will own the compensator

	//============================ OTHER MEMBER VARIABLES ============================
	const size_t index; //which power stage this regulator corresponds to
	bool enabled = false; //local variable to hold whether the regulator is enabled or not
};

//=================================================== WRAPPER INTERFACE TO LIMIT ACCESS =========================================================

class Regulator_Wrapper {
private:
	Regulator& regulator;
public:

	//=================== CONSTRUCTORS AND OPERATORS ===================
	inline Regulator_Wrapper(Regulator& _regulator): regulator(_regulator) {}

	//delete copy constructor and assignment operator to avoid any weird issues
	Regulator_Wrapper(Regulator const&) = delete;
	void operator=(Regulator_Wrapper const&) = delete;

	//========================= INSTANCE METHODS =========================
	inline bool get_enabled() {return regulator.get_enabled();}
	inline bool update_gain(float gain) {return regulator.update_gain(gain);}
	inline bool update_crossover_freq(float freq) {return regulator.update_crossover_freq(freq);}
	inline bool update_load_resistance(float res) {return regulator.update_load_resistance(res);}
	inline bool update_load_natural_freq(float freq) {return regulator.update_load_natural_freq(freq);}

	inline float get_gain() {return regulator.get_gain();}
	inline float get_crossover_freq() {return regulator.get_crossover_freq();}
	inline float get_load_resistance() {return regulator.get_load_resistance();}
	inline float get_load_natural_freq() {return regulator.get_load_natural_freq();}
};

#endif /* CONTROL_APP_CONTROL_REGULATOR_H_ */
