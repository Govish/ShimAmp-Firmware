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

#include "app_hal_adc.h" //just for testing
#include "app_hal_hrpwm.h" //just for testing
// #include "app_hal_uart.h" //just for testing

//subsystem includes
#include "app_comms_top_level.h"
#include "app_power_stage_top_level.h"

//command/request handler includes
#include "app_cmhand_power_stage_ctrl.h"
#include "app_rqhand_power_stage_status.h"

//configuration + utility includes
#include "app_config.h"
#include "app_utils_debug_print.h"
#include "app_utils.h"

Triggered_ADC test_adc(Triggered_ADC::CHANNEL_3);

//instantiating a configuration to pass to other subsystems
Configuration config;

//instantiating subsystems
Comms_Exec_Subsystem comms_exec(Comms_Exec_Subsystem::COMMS_CHANNEL_0); //create a comms executor on hardware channel 0
Power_Stage_Subsystem power_stage_sys(Power_Stage_Subsystem::POWER_STAGE_CHANNEL_0, config.active, 0); //instantiate an object that controls power stage 0
std::array<Power_Stage_Subsystem*, config.POWER_STAGE_COUNT> power_stage_systems = {&power_stage_sys}; //we have just a single power stage we're controlling (pass to the command handler)

//to make sure ADC is firing as frequently as expected
//Debug_Print db(UART::LPUART);

//TODO: setpoint controller, comms handlers for ADC reading + setpoint stuff + tuning stuff

void app_init() {
	//DIO::init();
	comms_exec.init(0x00); //comms ID
	power_stage_sys.init();

	//attach subsystem instances to command and request handlers
	Power_Stage_Command_Handlers::attach_power_stage_systems(power_stage_systems);
	Power_Stage_Request_Handlers::attach_power_stage_systems(power_stage_systems);

	//db.init(); //initialize debug UART
	power_stage_sys.set_mode(Power_Stage_Subsystem::Stage_Mode::ENABLED_MANUAL);
	test_adc.init();
	power_stage_sys.set_controller_frequency(100000);
}

void app_loop() {
	//handle the communication + command/request execution
	comms_exec.loop();

	//call the loop function for all power stage subsystems
	for(Power_Stage_Subsystem* stage : power_stage_systems) stage->loop();

//	//print out the ADC reading if it's new
//	auto [val, updated] = test_adc.get_val();
//	if(updated) {
//		std::string db_out = std::to_string(val);
//		db_out += "\r\n";
//		db.print(db_out);
//	}
}
