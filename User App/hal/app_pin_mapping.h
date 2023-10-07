/*
 * app_pin_mapping.h
 *
 *  Created on: Mar 11, 2023
 *      Author: Ishaan
 */

#ifndef BOARD_HAL_INC_APP_PIN_MAPPING_H_
#define BOARD_HAL_INC_APP_PIN_MAPPING_H_

extern "C" {
	#include "stm32g474xx.h" //for uint32_t
}

class PinMap {
public:

	//the values of the enum type correspond to the port offset
	enum GPIO_port {
		PORT_A = 0,
		PORT_B = 0x400,
		PORT_C = 0x800,
		PORT_D = 0xC00,
		PORT_E = 0x1000,
		PORT_F = 0x1400,
		PORT_G = 0x1800,
		PORT_H = 0x1C00
	};

	struct DIO_Hardware_Channel {
		const GPIO_port port;
		const uint32_t pin;
	};

	//================= DECLARATION OF PINS--UPDATE HERE =======================

	static const DIO_Hardware_Channel status_led;
	static const DIO_Hardware_Channel user_button;

	//================================== DELETE CONSTRUCTORS TO PREVENT INSTANTIATION ========================================
	PinMap() = delete;
	PinMap(PinMap const&) = delete;

private:
};

#endif /* BOARD_HAL_INC_APP_PIN_MAPPING_H_ */
