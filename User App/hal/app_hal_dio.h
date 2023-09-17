/*
 * app_hal_dio.h
 *
 *  Created on: Mar 10, 2023
 *      Author: Ishaan
 */

#ifndef BOARD_HAL_INC_APP_HAL_DIO_H_
#define BOARD_HAL_INC_APP_HAL_DIO_H_

#include "app_pin_mapping.h" //for digital io pin types
extern "C" {
	#include "stm32g474xx.h" //for uint32_t
}


class DIO {

private:
	//point to a pre-instantiated
	const dio_pin_t &pin_ref;
	const uint32_t PIN_DRIVE_MASK = 0;
	const uint32_t READ_MASK = 0;
	volatile uint32_t* const port_BSRR = 0; //register to set the pin high
	volatile uint32_t* const port_BRR = 0; //register to set the pin low
	volatile uint32_t* const port_IDR = 0; //register to read the pin


public:
	DIO(const dio_pin_t &pin_name);
	static void init();

//heavily optimize these functions for high performance
#pragma GCC push_options
#pragma GCC optimize ("O3")
	void set() const;
	void clear() const;
	uint32_t read() const;
#pragma GCC pop_options
};


#endif /* BOARD_HAL_INC_APP_HAL_DIO_H_ */
