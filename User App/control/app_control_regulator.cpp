/*
 * app_control_regulator.cpp
 *
 *  Created on: Oct 19, 2023
 *      Author: Ishaan
 */


#include "app_control_regulator.h"

Regulator::Regulator(	Power_Stage& _stage, Sampler& _sampler, Setpoint& _setpoint, //TODO: voltage measurement
						Configuration::Configuration_Params& _params, const size_t _index):
	stage(_stage), sampler(_sampler), setpoint(_setpoint), params(_params),
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
	recompute_rate(	params.POWER_STAGE_CONFIGS[index].K_DC,
					params.POWER_STAGE_CONFIGS[index].F_CROSSOVER,
					params.POWER_STAGE_CONFIGS[index].LOAD_RESISTANCE,
					params.POWER_STAGE_CONFIGS[index].LOAD_CHARACTERISTIC_FREQ);

	//attach regulator callback in using a lambda bind
	sampler.attach_sample_cb(Context_Callback_Function<>(this, regulate_forwarder));
}

//################ PARAMETER RECOMPUTATION ################

//NOTE: SWITCHING FREQUENCY AND SAMPLING FREQUENCY SHOULD BE UPDATED BEFORE CALLING THIS FUNCTION
bool Regulator::recompute_rate(	float desired_dc_gain,
								float desired_crossover_freq,
								float load_resistance,
								float load_natural_freq)
{
	//forbid this operation if the regulator is enabled
	if(enabled) return false;

	//create an array of forward-path gains of the system
	std::array<float, 4> dc_gains = {
			sampler.get_gain(), //ADC gain (current to ADC output)
			/*<CONTROLLER GOES HERE> (current to power stage counts)*/
			stage.get_gain(),  //power stage gain (counts to duty)
			12.0, //input voltage (duty to Vout); TODO actually reading the input voltage
			1 / load_resistance //coil DC conductance (Vout to current)
	};

	//create appropriate biquad constants given our system parameters
	Biquad::Biquad_Params comp_params;

	//don't bother cancelling the load pole if it's way beyond crossover frequency
	if(	load_natural_freq > desired_crossover_freq * 10)
		//just make a compensator with a single pole with the specified gain and crossover frequency
		comp_params = comp.make_gains(
				desired_dc_gain, desired_crossover_freq, //desired control gain + bandwidth
				dc_gains, //DC gains of the rest of the forward path
				sampler.GET_SAMPLING_FREQUENCY() //pull the actual controller sampling frequency
		);
	else
		//make a compensator with a single pole and a pole-cancelling zero such that we cross over at specified f_c given the DC gain
		comp_params = comp.make_gains_dumb(
				desired_dc_gain, desired_crossover_freq, load_natural_freq, //desired gain+bandwidth + zero frequency
				dc_gains, //DC gains of the rest of the forward path
				sampler.GET_SAMPLING_FREQUENCY() //pull the actual controller sampling frequency
		);

	//quick sanity check that parameter creation went fine (i.e. we have *some* kinda compensator constants to work with)
	//and generally that our control design seems feasible
	if(!comp_params.is_nonzero()) return false;

	//additionally, update the setpoint controller with any new sampling rates now too
	//this is in the event that sampling frequency was changed-->reconstruction filter coefficients need to be updated
	//ensure that this update goes smoothly
	if(!setpoint.recompute_rate())
		return false;

	//initialize the compensator with the computed biquad constants
	comp.update_params(comp_params);

	//additionally, update our output limits from power stage limits
	float max_drive = stage.get_max_drive_delta();
	comp.set_output_limits(-max_drive, max_drive);

	//update the configuration with these new parameters as well
	params.POWER_STAGE_CONFIGS[index].K_DC = desired_dc_gain;
	params.POWER_STAGE_CONFIGS[index].F_CROSSOVER = desired_crossover_freq;
	params.POWER_STAGE_CONFIGS[index].LOAD_RESISTANCE = load_resistance;
	params.POWER_STAGE_CONFIGS[index].LOAD_CHARACTERISTIC_FREQ = load_natural_freq;

	//indicate update was successful
	return true;
}

//just drop a new gain into the regulator
bool Regulator::update_gain(float new_gain) {
	return recompute_rate(	new_gain,
							params.POWER_STAGE_CONFIGS[index].F_CROSSOVER,
							params.POWER_STAGE_CONFIGS[index].LOAD_RESISTANCE,
							params.POWER_STAGE_CONFIGS[index].LOAD_CHARACTERISTIC_FREQ);
}

//just drop a new crossover frequency into the regulator
bool Regulator::update_crossover_freq(float new_crossover_freq) {
	return recompute_rate(	params.POWER_STAGE_CONFIGS[index].K_DC,
							new_crossover_freq,
							params.POWER_STAGE_CONFIGS[index].LOAD_RESISTANCE,
							params.POWER_STAGE_CONFIGS[index].LOAD_CHARACTERISTIC_FREQ);
}

//just drop a new load resistance into the regulator
bool Regulator::update_load_resistance(float new_load_resistance) {
	return recompute_rate(	params.POWER_STAGE_CONFIGS[index].K_DC,
							params.POWER_STAGE_CONFIGS[index].F_CROSSOVER,
							new_load_resistance,
							params.POWER_STAGE_CONFIGS[index].LOAD_CHARACTERISTIC_FREQ);
}

//just drop a new load characteristic frequency into the regulator
bool Regulator::update_load_natural_freq(float new_load_natural_freq) {
	return recompute_rate(	params.POWER_STAGE_CONFIGS[index].K_DC,
							params.POWER_STAGE_CONFIGS[index].F_CROSSOVER,
							params.POWER_STAGE_CONFIGS[index].LOAD_RESISTANCE,
							new_load_natural_freq);
}

//##### GETTER METHODS #####
//as of now, just pass up the values from the configuration
//TODO: maybe properly compute these given DC gains and biquad parameters
float Regulator::get_gain() {
	return params.POWER_STAGE_CONFIGS[index].K_DC;
}

float Regulator::get_crossover_freq() {
	return params.POWER_STAGE_CONFIGS[index].F_CROSSOVER;
}

float Regulator::get_load_resistance() {
	return params.POWER_STAGE_CONFIGS[index].LOAD_RESISTANCE;
}

float Regulator::get_load_natural_freq() {
	return params.POWER_STAGE_CONFIGS[index].LOAD_CHARACTERISTIC_FREQ;
}

//###### ENABLE CONTROL ######

bool Regulator::get_enabled() {
	return enabled;
}

void Regulator::enable() {
	//TODO: POTENTIALLY RECOMPUTE THE REGULATOR GAINS GIVEN THE DC VOLTAGE AT THIS POINT IN TIME

	enabled = true;
	sampler.enable_callback(); //get the sampler going
	setpoint.enable(); //get the setpoint controller going
	//ASSUME POWER STAGE IS ENABLED EXTERNALLY, I.E. THROUGH TOP LEVEL
}

void Regulator::disable() {
	//ASSUME POWER STAGE IS DISABLED EXTERNALLY, I.E. THROUGH TOP LEVEL
	setpoint.disable(); //disable the setpoint controller
	sampler.disable_callback(); //stop the sampler callback
	comp.reset(); //reset all memory variables for when we enable next time
	enabled = false;
}

//====================================== PRIVATE METHODS ====================================
void Regulator::regulate_forwarder(void* context) {
	static_cast<Regulator*>(context)->regulate();
}

//avoid enable sanity checking to reduce overhead
void Regulator::regulate() {
	//grab the next band-limited setpoint target
	float sp = setpoint.next();

	//TODO: compute feed-forward

	//grab the input and compute the error given the setpoint
	float current = sampler.get_current_reading();
	float error = sp  - current;

	//run the error through the compensator
	float output = comp.compute(error);

	//throw the output to the power stage (stage will constrain this output)
	stage.set_drive_raw((int16_t)output);
}
