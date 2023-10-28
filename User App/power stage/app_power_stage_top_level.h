/*
 * app_power_stage_top_level.h
 *
 *  Created on: Oct 5, 2023
 *      Author: Ishaan
 */

#ifndef POWER_STAGE_CONTROL_APP_POWER_STAGE_TOP_LEVEL_H_
#define POWER_STAGE_CONTROL_APP_POWER_STAGE_TOP_LEVEL_H_

//c++ includes
#include <stddef.h> //for size_t

//HAL type includes
#include "app_hal_hrpwm.h"
#include "app_hal_dio.h"
#include "app_hal_adc.h"

//Higher level functions related to power stage control/regulation
#include "app_power_stage_drive.h"
#include "app_power_stage_sampler.h"
#include "app_control_regulator.h"
#include "app_setpoint_controller.h"

//configuration informatin
#include "app_config.h"

class Power_Stage_Subsystem {
public:
	//======================================================= CONFIGURATION DETAILS STRUCT =======================================================
	//following the paradigm of passing pre-instantiated configuration information to the constructor
	struct Channel_Hardware_Details {
		//how the hardware connects to the power stage IC
		const HRPWM::HRPWM_Hardware_Channel& pos_channel;
		const HRPWM::HRPWM_Hardware_Channel& neg_channel;
		const PinMap::DIO_Hardware_Channel& en_pin_name;
		const bool en_active_high;

		//ADC channels for measuring control variables
		Triggered_ADC::Triggered_ADC_Hardware_Channel& ifine;
		Triggered_ADC::Triggered_ADC_Hardware_Channel& icoarse;
	};
	static Channel_Hardware_Details POWER_STAGE_CHANNEL_0;

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
	//have to take config as a pointer, because I need to point to the original config struct
	Power_Stage_Subsystem(Channel_Hardware_Details& hardware_details, Configuration::Configuration_Params* _config, const size_t _CHANNEL_NUM);
	Power_Stage_Subsystem(Power_Stage_Subsystem const&) = delete;
	void operator=(Power_Stage_Subsystem const&) = delete;

	//have a special function that sets the operating mode of the power stage
	//modes defined by the enum class above
	bool set_mode(Stage_Mode mode); //returns true if successfully set
	Stage_Mode get_mode(); //equivalent getter method

	void init(); //call from the setup function
	void loop(); //call from the loop function

	//have a special method that tweaks the switching frequencies or controller frequencies of ALL power stages
	//basically a higher level function that tunnels down to the HAL level (along with changing parameters at this higher level)
	//static methods to ensure all instances are synchronized with this change
	//the implementation of these functions is a little hacky, but I think they should get the job done
	static bool set_switching_frequency(float fsw_hz);
	static bool set_controller_frequency(float fc_hz);
	static bool set_operating_frequencies(float fsw_hz, float fc_hz);
	static float get_switching_frequency();
	static float get_controller_frequency();

	//### GETTER METHODS ###
	//these methods will return a pointer to the constituent instances of the power stage controller
	//in a wrapper class that is access controlled
	//as such, writes/configuration updates will be forbidden if it's not safe to do so

	//return a reference to the power stage instance for manual control (access controlled appropriately)
	Power_Stage_Wrapper& get_direct_stage_control_instance();

	//return a reference to the sampler
	//allows for current measurement, and ADC trimming
	Sampler_Wrapper& get_sampler_instance();

	//return a reference to the regulator
	//allows updates to control parameters and whether the regulator is running
	Regulator_Wrapper& get_regulator_instance();

	//return a reference to the setpoint contorller
	//allows updates to setpoint/setpoint control while the regulator is running
	Setpoint_Wrapper& get_setpoint_instance();

private:
	//=============================== PRIVATE METHOD TO UPDATE INSTANCES WHEN OPERATING FREQUENCIES UPDATED =====================================
	bool recompute_rates();


	//##### all these objects will be initialized in the constructor of `Comms_Exec_Subsystem` #####
	//======================== Everything Power-stage Related =========================
	Power_Stage stage; //manage a power stage
	Power_Stage_Wrapper stage_wrapper; //wrap the power stage with this function when handing off to the public

	//====================== Everything Sampler Related ====================
	Sampler current_sampler; //instance that reads the current through the output
	Sampler_Wrapper current_sampler_wrapper; //wrap the sampler with this before handing it off to the public

	//====================== Everything Setpoint Controller Related ===================
	Setpoint setpoint; //instance that actually generates the current setpoint
	Setpoint_Wrapper setpoint_wrapper; //wrap the setpoint controller with this before handing it off to the public

	//====================== Everything Regulator Related =====================
	Regulator regulator; //instance that actually does the current regulation
	Regulator_Wrapper regulator_wrapper; //wrap the regulator with this before handing it off to the public


	/*
	 * TODO:
	 *  - dither?
	 */
	//==================== State-esque member variables ======================
	static std::array<Power_Stage_Subsystem*, Configuration::POWER_STAGE_COUNT> ALL_POWER_STAGES; //container of all power stage instances
	static size_t INSTANCE_COUNT; //helps us index into the above array, also holds number of instantiated power stages
	Stage_Mode operating_mode = Stage_Mode::DISABLED; //start with the stage disabled

	//have to make configuration static in order to have a clean implementation of frequency setters
	//configuration will be assigned during construction; so shouldn't be that big of a deal
	static Configuration::Configuration_Params* config; //config instance to access the global config structure
	const size_t CHANNEL_NUM; //basically the channel ID assigned to this power stage--lets us index into configuration (and maybe other things in the future)
};


#endif /* POWER_STAGE_CONTROL_APP_POWER_STAGE_TOP_LEVEL_H_ */
