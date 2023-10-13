/*
 * app_power_stage_top_level.h
 *
 *  Created on: Oct 5, 2023
 *      Author: Ishaan
 */

#ifndef POWER_STAGE_CONTROL_APP_POWER_STAGE_TOP_LEVEL_H_
#define POWER_STAGE_CONTROL_APP_POWER_STAGE_TOP_LEVEL_H_

//HAL type includes
#include "app_hal_hrpwm.h"
#include "app_hal_dio.h"

//Higher level functions related to power stage control/regulation
#include "app_power_stage_drive.h"

class Power_Stage_Subsystem {
public:
	//======================================================= CONFIGURATION DETAILS STRUCT =======================================================
	//following the paradigm of passing pre-instantiated configuration information to the constructor
	struct Configuration_Details {
		//how the hardware connects to the power stage IC
		const HRPWM::HRPWM_Hardware_Channel& pos_channel;
		const HRPWM::HRPWM_Hardware_Channel& neg_channel;
		const PinMap::DIO_Hardware_Channel& en_pin_name;
		const bool en_active_high;
	};
	static Configuration_Details POWER_STAGE_CHANNEL_0;

	//============================================= ENUM CLASS TO ENABLE DIFFERENT OPERATING MODES OF STAGE =============================================

	//use C-style enum to allow for implicit conversion to uint8_t
	//since it's defined within a class, namespace conflicts should be a bit more difficult
	enum Stage_Mode {
		UNINITIALIZED 		= (uint8_t)0xFF,
		DISABLED			= (uint8_t)0x00,
		ENABLED_AUTO		= (uint8_t)0x01,
		ENABLED_MANUAL		= (uint8_t)0x02,
		ENABLED_AUTOTUNING	= (uint8_t)0x03,
	};

	//======================================================= PUBLIC METHODS =======================================================

	//constructor; delete copy constructor and assignment operator to avoid weird hardware conflicts
	Power_Stage_Subsystem(Configuration_Details& config_details);
	Power_Stage_Subsystem(Power_Stage_Subsystem const&) = delete;
	void operator=(Power_Stage_Subsystem const&) = delete;

	//have a special function that sets the operating mode of the power stage
	//modes defined by the enum class above
	bool set_mode(Stage_Mode mode); //returns true if successfully set
	Stage_Mode get_mode(); //equivalent getter method

	void init(); //call from the setup function
	void loop(); //call from the loop function

	//have a special method that tweaks the switching frequency of ALL power stages
	//basically a higher level function that tunnels down to the HAL level (along with changing parameters at this higher level)
	//also includes a corresponding getter method
	static bool set_switching_frequency(float fsw_hz);
	static float get_switching_frequency();

	//### GETTER METHODS ###
	//these methods will return a pointer to the constituent instances of the power stage controller
	//  \--> IF IT IS SAFE TO DO SO (i.e. the correct mode is enabled)
	//if not these functions will return A NULL POINTER, i.e. CALLING FUNCTIONS MUST CHECK FOR NULL POINTERS

	//return a pointer to the power stage instance for manual control (access controlled appropriately)
	Power_Stage_Wrapper& get_direct_stage_control_instance();

private:
	//##### all these objects will be initialized in the constructor of `Comms_Exec_Subsystem` #####

	//======================== Everything Power-stage Related =========================
	DIO en_pin;
	HRPWM chan_pwm_pos; //pwm that drives the positive-side half-bridge
	HRPWM chan_pwm_neg; //pwm that drives the negative-side half-bridge
	Power_Stage stage; //manage a power stage
	Power_Stage_Wrapper stage_wrapper; //wrap the power stage with this function when handing off to the public

	/*
	 * TODO:
	 *  - regulator
	 *  - setpoint controller
	 *  - ADC
	 *  - etc.
	 */
	//==================== State-esque member variables ======================
	Stage_Mode operating_mode = Stage_Mode::DISABLED; //start with the stage disabled
};


#endif /* POWER_STAGE_CONTROL_APP_POWER_STAGE_TOP_LEVEL_H_ */