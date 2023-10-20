/*
 * app_comms_crc.cpp
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
 *  ben eater also has a great video on CRC computation on YouTube
 */

#include "app_comms_crc.h"

Comms_CRC::Comms_CRC():
	//empty constructor initializes stuff with defaults
	polynomial(DEFAULT_POLYNOMIAL), seed(DEFAULT_SEED), xor_out(DEFAULT_XOR_OUT)
{}

Comms_CRC::Comms_CRC(const uint16_t _poly, const uint16_t _seed, const uint16_t _xor_out):
	polynomial(_poly), seed(_seed), xor_out(_xor_out)
{
	//generate the lookup table based off the polynomial
	//more or less replicating the solution in the stackoverflow thread
	for(size_t i = 0; i < 256; i++) {

		//naming this bitstream to wrap my head around this better
		uint16_t bitstream = ((uint8_t)i) << 8;
		//shift the data over 8 times
		for(size_t shift_count = 0; shift_count < 8; shift_count++) {
			if(bitstream & 0x8000) //mask the high bit, if it's 1
				bitstream = (bitstream << 1) ^ polynomial;
			else
				bitstream = bitstream << 1;
		}

		//after we finish the shift, the CRC for that particular byte has been computed
		//store that value in the lookup table
		LUT[i] = bitstream;
	}
}

//remember, the high byte of the CRC goes first (i.e. the lower index in the buffer)
//and the low byte goes next (higher index in the buffer)
uint16_t Comms_CRC::compute_crc(const std::span<uint8_t, std::dynamic_extent> buf) {
	//initializing the CRC value with the appropriate seed
	uint16_t crc = seed;

	//run through all the bytes
	for(size_t i = 0; i < buf.size(); i++) {
		uint8_t crced_byte = ((crc >> 8) ^ buf[i]) & 0xFF; //apply accumulated CRC to the byte in the buffer
		uint16_t crc_LUT = LUT[crced_byte]; //look the appropriate CRC value up in the table
		crc = crc_LUT ^ (crc << 8); //ensure the low byte from the previous crc calculation is applied
	}

	//and return the final CRC value
	return crc;
}

bool Comms_CRC::validate_crc(const std::span<uint8_t, std::dynamic_extent> buf) {
	//crc validation just involves running the CRC back through the CRC computation algorithm
	uint16_t pre_crc = compute_crc(buf);

	//xor the computed CRC
	//if the result is something other than zero, the CRC computation is incorrect
	if(pre_crc ^ xor_out)
		return false;
	return true; //CRC computation should be zero
}


