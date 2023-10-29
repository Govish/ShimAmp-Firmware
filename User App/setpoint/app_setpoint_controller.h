/*
 * app_setpoint_controller.h
 *
 *  Created on: Oct 21, 2023
 *      Author: Ishaan
 *
 *
 *  Note: Getting rid of the reconstruction Bessel filter--adds too much computational overhead to be in the ISR
 *  Also, step response seems to be well-behaved enough such that it may not be necessary
 */

#ifndef SETPOINT_APP_SETPOINT_CONTROLLER_H_
#define SETPOINT_APP_SETPOINT_CONTROLLER_H_

//library includes
#include <math.h>

#include "app_config.h" //access the configuration structure
#include "app_power_stage_sampler.h" //read the active sampling rate

//#### WAVEFORM STYLES ####
#include "app_setpoint_waveform_template.h"
#include "app_setpoint_waveform_dc.h" //dc drive

//TODO: HAL include for EXTIs and sampling tick timer

class Setpoint {
public:
	Setpoint(Configuration::Configuration_Params& _params, const size_t _index);

	//initialization function; more or less just need to set up the reconstruction filter
	void init();

	/*
	 * Enable control/status
	 * A disable --> enable transition causes starts the setpoint controller outputting a DC value
	 * A waveform sequence can then be selected and started immediately or started upon an external trigger
	 * Specific behavior of these functions are subject to change depending on most useful functionality
	 */
	void enable();
	void disable();
	bool get_enabled();

	/*
	 * Call this function when the controller rate changes
	 * This will recompute the rate of the active waveform along with
	 * Only valid to call when the sampler is disabled
	 * Returns true if resetting the rate was possible
	 */
	bool recompute_rate();

	/*
	 * Get the next band-limited setpoint value for the controller
	 * Grabs the next point from the waveform generator and runs it through the filter
	 * Call this function at the specified controller rate to achieve expected behavior
	 */
	float __attribute__((optimize("O3"))) next();

	//=============================== WAVEFORM SELECTION FUNCTIONS ================================
	bool reset_setpoint(); //reset the setpoint back to zero, trigger immediately
	bool make_setpoint_dc(bool trigger_gated, float setpoint); //drive just a pure DC current from the amp

private:
	//==================== PRIVATE FUNCTIONS TO PLUMB UP TO I/O =================
	void __attribute__((optimize("O3"))) tick(); //called by a fixed-frequency, active-when-enabled timer interrupt
	void __attribute__((optimize("O3"))) trigger_assert(); //called by EXTI rising edge
	void __attribute__((optimize("O3"))) trigger_deassert(); //called by an EXTI falling edge

	//=========================== STATIC/INSTANCE VARIABLES =====================
	//TODO: implement a trigger and tick function hooked up to EXTIs and Timer Interrupts
	Configuration::Configuration_Params& params; //access the active configuration structure
	Waveform* active_waveform; //waveform that's currently running
	Waveform* trigger_asserted_waveform; //waveform to output after trigger asserted
	Waveform* trigger_deasserted_waveform; //waveform to output after trigger deasserted

	//primitive types
	const size_t index; //which channel index this setpoint controller corresponds to
	bool enabled = false;

	//============= DIFFERENT WAVEFORM FLAVORS; STATICALLY INSTANTIATE ===============
	Waveform zero_drive; //upon enable, point the active waveform to this
	DC_Waveform drive_dc;
};


//=================================================== WRAPPER INTERFACE TO LIMIT ACCESS =========================================================

class Setpoint_Wrapper {
private:
	Setpoint& setpoint;
public:

	//=================== CONSTRUCTORS AND OPERATORS ===================
	inline Setpoint_Wrapper(Setpoint& _setpoint): setpoint(_setpoint) {}

	//delete copy constructor and assignment operator to avoid any weird issues
	Setpoint_Wrapper(Setpoint_Wrapper const&) = delete;
	void operator=(Setpoint_Wrapper const&) = delete;

	//========================= INSTANCE METHODS =========================
	inline bool get_enabled() {return setpoint.get_enabled();}
	inline bool reset_setpoint() {return setpoint.reset_setpoint();}
	inline bool make_setpoint_dc(bool trigger_gated, float sp) {return setpoint.make_setpoint_dc(trigger_gated, sp);}
};
#endif /* SETPOINT_APP_SETPOINT_CONTROLLER_H_ */
