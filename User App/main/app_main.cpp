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

DIO user_led(PinMap::status_led);
DIO user_button(PinMap::user_button);
Comms_Exec_Subsystem& comms_exec = Comms_Exec_Subsystem::get_instance(); //get the singleton instance

/*TODO: re-validate COBS especially with new encoding scheme*/
/*TODO: re-validate CRC*/
/*TODO: validate parser*/

void app_init() {
	DIO::init();
	comms_exec.init(0x00);
}

void app_loop() {
	if(user_button.read())
		user_led.set();
	else user_led.clear();

	//handle the communication + command/request execution
	comms_exec.loop();
}


