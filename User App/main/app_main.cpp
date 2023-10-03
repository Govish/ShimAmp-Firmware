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
#include "app_hal_hrpwm.h"

//subsystem includes
#include "app_comms_top_level.h"

DIO user_led(PinMap::status_led);
DIO user_button(PinMap::user_button);
Comms_Exec_Subsystem& comms_exec = Comms_Exec_Subsystem::get_instance(); //get the singleton instance

HRPWM pwm_pa11(HRPWM::CHANNEL_B2_PA11);


void app_init() {
	DIO::init();
	comms_exec.init(0x00);
	pwm_pa11.init();

	HRPWM::SET_PERIOD_ALL(4352); //1.25MHz
	pwm_pa11.set_duty(0.5);
	HRPWM::ENABLE_ALL();
	Timer::delay_ms(5000);
}

//void app_loop() {
//	if(user_button.read())
//		user_led.set();
//	else user_led.clear();
//
//	//handle the communication + command/request execution
//	comms_exec.loop();
//}

void app_loop() {
	for(float i = 0; i <= 1; i += 0.125) {
		pwm_pa11.set_duty(i);
		Timer::delay_ms(1000);
	}
	HRPWM::DISABLE_ALL();
	HRPWM::SET_PERIOD_ALL(4352 >> 1);
	HRPWM::ENABLE_ALL();

	for(float i = 0; i <= 1; i += 0.125) {
		pwm_pa11.set_duty(i);
		Timer::delay_ms(1000);
	}
	HRPWM::DISABLE_ALL();
	HRPWM::SET_PERIOD_ALL(4352);
	HRPWM::ENABLE_ALL();
}
