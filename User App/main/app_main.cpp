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
#include "app_hal_hrpwm.h"

//for testing, include UART
#include "app_hal_uart.h"

#include "app_utils.h"

HRPWM pwm_pa11(HRPWM::CHANNEL_B2_PA11);

std::array<uint8_t, 256> txbuf;
std::array<uint8_t, 256> rxbuf;
std::array<uint8_t, 256> bytes_to_tx;
UART comms(UART::LPUART, '<', '>', txbuf, rxbuf);


void app_init() {
	DIO::init();
	comms.init();
	pwm_pa11.init();

	pwm_pa11.set_duty(0.22);

	//sweep between period settings for the HRTIM
	for(uint16_t fper = 2730; fper >= 2710; fper -= 1) {
		HRPWM::SET_PERIOD_ALL(fper);
		HRPWM::ENABLE_ALL();

		std::string fsw_string = f2s<4>(5440.0 / (float)fper);
		fsw_string += "MHz switching frequency\r\n";
		std::copy(fsw_string.begin(), fsw_string.end(), bytes_to_tx.begin());

		comms.transmit(spn(bytes_to_tx, fsw_string.size()));

		Timer::delay_ms(5000);
		HRPWM::DISABLE_ALL();
		Timer::delay_ms(1000);
	}
}

void app_loop() {
}
