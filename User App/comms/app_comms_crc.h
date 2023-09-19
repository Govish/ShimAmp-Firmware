/*
 * app_comms_crc.h
 *
 *  Created on: Sep 13, 2023
 *      Author: Ishaan
 *
 *  I know that STM32G474 has a built-in CRC calculator; but the hardware is a little restricted in terms of what it can do
 *  this is a relatively fast software implementation that leverages a LUT to:
 *  	- compute a 16-bit CRC given the input byte stream
 *  	- validate a byte stream's CRC value
 *
 *  https://stackoverflow.com/questions/44131951/how-to-generate-16-bit-crc-table-from-a-polynomial
 */

#ifndef COMMS_APP_COMMS_CRC_H_
#define COMMS_APP_COMMS_CRC_H_

#include <stddef.h> //for size_t
#include <span> //for span

extern "C" {
	#include "stm32g474xx.h" //for uint16_t
}

//can't use 'CRC' as a class name because there's a global level define with the same name
class Comms_CRC {

public:
	Comms_CRC(const uint16_t _poly, const uint16_t _seed, const uint16_t _xor_out);

	//provide a c++ style interface to compute and validate a CRC
	uint16_t compute_crc(const std::span<uint8_t, std::dynamic_extent> buf);
	bool validate_crc(const std::span<uint8_t, std::dynamic_extent> buf);

private:
	const uint16_t polynomial; //actual CRC polynomial we'll compute with
	const uint16_t seed; //value CRC computation is initialized with
	const uint16_t xor_out; //value we need to xor the CRC result with when checking

	//lookup table that gets generated based off the polynomial
	uint16_t LUT[256];
};



#endif /* COMMS_APP_COMMS_CRC_H_ */
