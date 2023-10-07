/*
 * app_hal_dio.cpp
 *
 *  Created on: Mar 10, 2023
 *      Author: Ishaan
 */

#include "app_hal_dio.h"
extern "C" {
	#include "gpio.h"
}

#define BSRR_BASE_REG		GPIOA_BASE + 0x18
#define BRR_BASE_REG		GPIOA_BASE + 0x28
#define IDR_BASE_REG		GPIOA_BASE + 0x10

//==================== STATIC MEMBER INITIALIZATION ==================

bool DIO::GPIO_INITIALIZED = false;

//======================= PUBLIC FUNCTIONS =====================

//don't modify the reference
DIO::DIO(const PinMap::DIO_Hardware_Channel& pin_name):
		pin_ref(pin_name),
		PIN_DRIVE_MASK(1 << (pin_name.pin)),
		READ_MASK(1 << pin_name.pin),
		port_BSRR((volatile uint32_t*) (BSRR_BASE_REG + (uint32_t)pin_name.port)),
		port_BRR((volatile uint32_t*) (BRR_BASE_REG + (uint32_t)pin_name.port)),
		port_IDR((volatile uint32_t*) (IDR_BASE_REG + (uint32_t)pin_name.port))
{
	//the bit set register corresponds to the BSRR base register plus the port offset
	//the bit clear register corresponds to the BRR base register plus the port offset
	//encode the port offset into the port enumeration
	//same thing goes with the input data register
}

void DIO::init() {
	if(GPIO_INITIALIZED) return;
	MX_GPIO_Init();
	GPIO_INITIALIZED = true;
}

void DIO::set() const {
	*port_BSRR = PIN_DRIVE_MASK;
}

void DIO::clear() const {
	*port_BRR = PIN_DRIVE_MASK;
}

uint32_t DIO::read() const {
	return ( (*port_IDR) & READ_MASK );
}



