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

//HAL includes
#include "app_hal_dio.h"
#include "app_pin_mapping.h"
#include "app_hal_timing.h"

//subsystem includes
#include "app_comms_top_level.h"
#include "app_power_stage_top_level.h"

//command/request handler includes
#include "app_cmhand_power_stage_ctrl.h"
#include "app_rqhand_power_stage_status.h"


DIO user_led(PinMap::status_led);
DIO user_button(PinMap::user_button);

//instantiating subsystems
Comms_Exec_Subsystem comms_exec(Comms_Exec_Subsystem::COMMS_EXEC_CONFIG); //create a comms executor based off default config
Power_Stage_Subsystem power_stage_sys(Power_Stage_Subsystem::POWER_STAGE_CHANNEL_0); //instantiate an object that controls power stage 0
std::array power_stage_systems = {&power_stage_sys}; //we have just a single power stage we're controlling (pass to the command handler)

void app_init() {
	//DIO::init();
	comms_exec.init(0x00);
	power_stage_sys.init();

	//attach subsystem instances to command and request handlers
	Power_Stage_Command_Handlers::attach_power_stage_systems(power_stage_systems);
	Power_Stage_Request_Handlers::attach_power_stage_systems(power_stage_systems);
}

void app_loop() {
//	if(user_button.read())
//		user_led.set();
//	else user_led.clear();

	//handle the communication + command/request execution
	comms_exec.loop();

	//call the loop function for all power stage subsystems
	for(Power_Stage_Subsystem* stage : power_stage_systems) stage->loop();
}

//void app_loop() {
//	for(float i = 0; i <= 1; i += 0.125) {
//		pwm_pa11.set_duty(i);
//		Timer::delay_ms(1000);
//	}
//	HRPWM::DISABLE_ALL();
//	HRPWM::SET_FSW(625e3);
//	HRPWM::ENABLE_ALL();
//
//	for(float i = 0; i <= 1; i += 0.125) {
//		pwm_pa11.set_duty(i);
//		Timer::delay_ms(1000);
//	}
//	HRPWM::DISABLE_ALL();
//	HRPWM::SET_FSW(1.25e6);
//	HRPWM::ENABLE_ALL();
//}
