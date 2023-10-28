/*
 * app_main.cpp
 *
 *  Created on: Sep 12, 2023
 *      Author: Ishaan
 */

#include "app_main.h"

//c/c++ includes
#include <array>
#include <algorithm>
#include <string>

//HAL includes
#include "app_hal_timing.h"

//subsystem includes
#include "app_comms_top_level.h"
#include "app_power_stage_top_level.h"

//command/request handler includes
#include "app_cmhand_power_stage_ctrl.h"
#include "app_cmhand_setpoint.h"
#include "app_cmhand_control.h"
#include "app_cmhand_sampler.h"
#include "app_rqhand_power_stage_status.h"
#include "app_rqhand_setpoint.h"
#include "app_rqhand_control.h"
#include "app_rqhand_sampler.h"


//configuration + utility includes
#include "app_config.h"
#include "app_utils_debug_print.h"
#include "app_utils.h"

//instantiating a configuration to pass to other subsystems
Configuration config;

//instantiating subsystems
Comms_Exec_Subsystem comms_exec(Comms_Exec_Subsystem::COMMS_CHANNEL_0); //create a comms executor on hardware channel 0
Power_Stage_Subsystem power_stage_sys(Power_Stage_Subsystem::POWER_STAGE_CHANNEL_0, &config.active, 0); //instantiate an object that controls power stage 0
std::array<Power_Stage_Subsystem*, config.POWER_STAGE_COUNT> power_stage_systems = {&power_stage_sys}; //we have just a single power stage we're controlling (pass to the command handler)

Debug_Print db(UART::UART3);

//TODO: comms handlers for ADC reading + setpoint stuff + tuning stuff, also instantiate some more UARTs and ADCs

void app_init() {
	//DIO::init();
	comms_exec.init(0x00); //comms ID
	power_stage_sys.init();

	//attach subsystem instances to command and request handlers
	Power_Stage_Command_Handlers::attach_power_stage_systems(power_stage_systems);
	Setpoint_Command_Handlers::attach_power_stage_systems(power_stage_systems);
	Controller_Command_Handlers::attach_power_stage_systems(power_stage_systems);
	Sampler_Command_Handlers::attach_power_stage_systems(power_stage_systems);

	Power_Stage_Request_Handlers::attach_power_stage_systems(power_stage_systems);
	Setpoint_Request_Handlers::attach_power_stage_systems(power_stage_systems);
	Controller_Request_Handlers::attach_power_stage_systems(power_stage_systems);
	Sampler_Request_Handlers::attach_power_stage_systems(power_stage_systems);

	db.init(); //initialize debug UART
}

void debug_func() {
	static auto& sampler = power_stage_sys.get_sampler_instance();
	static uint32_t tick = Timer::get_ms();

	if(Timer::get_ms() - tick > 1) {
		std::string text = std::to_string(sampler.read_fine_raw()) + std::string("\t\t") + std::to_string(sampler.read_coarse_raw()) + std::string("\r\n");
		db.print(text);
		tick += 10;
	}
}

void app_loop() {
	//handle the communication + command/request execution
	comms_exec.loop();

	//call the loop function for all power stage subsystems
	for(Power_Stage_Subsystem* stage : power_stage_systems) stage->loop();

	debug_func();
}


