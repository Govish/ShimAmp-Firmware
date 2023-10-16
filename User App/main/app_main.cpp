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
#include "app_hal_dio.h"
#include "app_pin_mapping.h"
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

//utility includes
#include "app_utils_debug_print.h"
#include "app_utils.h"

DIO user_led(PinMap::status_led);
DIO user_button(PinMap::user_button);

Triggered_ADC test_adc(Triggered_ADC::CHANNEL_3);

//instantiating subsystems
Comms_Exec_Subsystem comms_exec(Comms_Exec_Subsystem::COMMS_EXEC_CONFIG); //create a comms executor based off default config
Power_Stage_Subsystem power_stage_sys(Power_Stage_Subsystem::POWER_STAGE_CHANNEL_0); //instantiate an object that controls power stage 0
std::array power_stage_systems = {&power_stage_sys}; //we have just a single power stage we're controlling (pass to the command handler)

//to make sure ADC is firing as frequently as expected
//Debug_Print db(UART::LPUART);

void app_init() {
	//DIO::init();
	//comms_exec.init(0x00);
	power_stage_sys.init();

	//attach subsystem instances to command and request handlers
	Power_Stage_Command_Handlers::attach_power_stage_systems(power_stage_systems);
	Power_Stage_Request_Handlers::attach_power_stage_systems(power_stage_systems);

	//db.init(); //initialize debug UART
	power_stage_sys.set_switching_frequency(1000000);
	power_stage_sys.set_mode(Power_Stage_Subsystem::Stage_Mode::ENABLED_MANUAL);
	test_adc.init();
	HRPWM::SET_ADC_TRIGGER_FREQUENCY(100000);
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
