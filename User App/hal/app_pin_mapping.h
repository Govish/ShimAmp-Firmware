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

//the values of the enum type correspond to the port offset
typedef enum {
	PORT_A = 0,
	PORT_B = 0x400,
	PORT_C = 0x800,
	PORT_D = 0xC00,
	PORT_E = 0x1000,
	PORT_F = 0x1400,
	PORT_G = 0x1800,
	PORT_H = 0x1C00
} gpio_port_t;

typedef struct {
	const gpio_port_t port;
	const uint32_t pin;
} dio_pin_t;

class PinMap {
public:

	//================= DECLARATION OF PINS--UPDATE HERE =======================

	static const dio_pin_t status_led;
	static const dio_pin_t user_button;

	//==========================================================================

private:
	//shouldn't be able to instantiate this class
	PinMap(){};
};

#endif /* BOARD_HAL_INC_APP_PIN_MAPPING_H_ */
