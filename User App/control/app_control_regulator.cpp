/*
 * app_control_regulator.cpp
 *
 *  Created on: Oct 19, 2023
 *      Author: Ishaan
 */


#include "app_control_regulator.h"

Regulator::Regulator(	Power_Stage& _stage, Sampler& _sampler, //TODO: also pass it a setpoint controller, voltage measurement
						Configuration::Configuration_Params& _params, const size_t _index):
	stage(_stage), sampler(_sampler), params(_params),
	comp(),
	index(_index)
{
}

//stages and sampler will already be initialized, so just need to initialize the compensator really
//additionally attach the regulator interrupt to the sampler callback
void Regulator::init() {
	//ensure everything starts off disabled
	enabled = false;
	sampler.disable_callback();

	//update coefficients based off of initial configuration
	recompute_rate();

	//attach regulator callback in using a lambda bind
	sampler.attach_sample_cb([this](){regulate();});
}

//NOTE: SWITCHING FREQUENCY AND SAMPLING FREQUENCY SHOULD BE UPDATED BEFORE CALLING THIS FUNCTION
bool Regulator::recompute_rate() {
	//create an array of forward-path gains of the system
	std::array<float, 4> dc_gains = {
			sampler.get_gain(), //ADC gain (current to ADC output)
			/*<CONTROLLER GOES HERE> (current to power stage counts)*/
			stage.get_gain(),  //power stage gain (counts to duty)
			12.0, //input voltage (duty to Vout); TODO actually reading the input voltage
			1 / params.POWER_STAGE_CONFIGS[index].LOAD_RESISTANCE //coil DC conductance (Vout to current)
	};

	//create appropriate biquad constants given our system parameters
	Biquad::Biquad_Params comp_params;
	if(	params.POWER_STAGE_CONFIGS[index].LOAD_CHARACTERISTIC_FREQ > 	//don't bother cancelling the load pole
		params.POWER_STAGE_CONFIGS[index].F_CROSSOVER * 10)				//if the frequency is way past crossover

		//just make a compensator with a single pole with the specified gain and crossover frequency
		comp_params = comp.make_gains(
				params.POWER_STAGE_CONFIGS[index].K_DC,
				params.POWER_STAGE_CONFIGS[index].F_CROSSOVER,
				dc_gains, //DC gains of the rest of the forward path
				sampler.GET_SAMPLING_FREQUENCY() //pull the actual controller sampling frequency
		);
	else
		//make a compensator with a single pole and a pole-cancelling zero such that we cross over at specified f_c given the DC gain
		comp_params = comp.make_gains(
				params.POWER_STAGE_CONFIGS[index].K_DC,
				params.POWER_STAGE_CONFIGS[index].F_CROSSOVER,
				params.POWER_STAGE_CONFIGS[index].LOAD_CHARACTERISTIC_FREQ,
				dc_gains, //DC gains of the rest of the forward path
				sampler.GET_SAMPLING_FREQUENCY() //pull the actual controller sampling frequency
		);

	//quick sanity check that parameter creation went fine (i.e. we have *some* kinda compensator constants to work with)
	if(!comp_params.is_nonzero()) return false;

	//initialize the compensator with the computed biquad constants
	comp.update_params(comp_params);

	//TODO: update setpoint controller if necessary here

	//indicate update was successful
	return true;
}

void Regulator::trim_gain() {
	//create an array of forward-path gains of the system
	std::array<float, 4> dc_gains = {
			sampler.get_gain(), //ADC gain (current to ADC output)
			/*<CONTROLLER GOES HERE> (current to power stage counts)*/
			stage.get_gain(),  //power stage gain (counts to duty)
			12.0, //input voltage (duty to Vout); TODO actually reading the input voltage
			1 / params.POWER_STAGE_CONFIGS[index].LOAD_RESISTANCE //coil DC conductance (Vout to current)
	};

	//and compute the gain trim given the real forward path gain of the system now
	comp.compute_gain_trim(params.POWER_STAGE_CONFIGS[index].K_DC, dc_gains);
}

//###### ENABLE CONTROL ######

bool Regulator::get_enabled() {
	return enabled;
}

void Regulator::enable() {
	enabled = true;
	sampler.enable_callback(); //get the sampler going
	//ASSUME POWER STAGE IS ENABLED EXTERNALLY, I.E. THROUGH TOP LEVEL
}

void Regulator::disable() {
	//ASSUME POWER STAGE IS DISABLED EXTERNALLY, I.E. THROUGH TOP LEVEL
	sampler.disable_callback(); //stop the sampler callback
	comp.reset(); //reset all memory variables for when we enable next time
	enabled = false;
}

//====================================== PRIVATE METHODS ====================================
void Regulator::regulate() {
	//sanity check that regulator is enabled, shouldn't ever need to depend on this
	if(!enabled) return;

	float setpoint = 0; //TODO: grab the next setpoint

	//grab the input and compute the error given the setpoint
	float current = sampler.get_current_reading();
	float error = setpoint  - current;

	//run the error through the compensator
	float output = comp.compute(error);

	//TODO: add dither maybe

	//throw the output to the power stage (stage will constrain this output)
	stage.set_drive_raw(output);
}
