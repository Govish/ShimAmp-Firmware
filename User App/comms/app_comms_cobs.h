/*
 * app_cobs.h
 *
 *  Created on: Sep 13, 2023
 *      Author: Ishaan
 *
 *  This module serves to perform cobs encoding and decoding as a part of the communications subsystem
 *  It also contains information about the start-of-frame and end-of-frame characters, along with message size limits
 *
 *  My general convention throughout the communication subsystem is to share data between objects using `std::span`s
 *  This follows canonical c++ coding techniques, and can leverage memory copying and type casting functions built into the language
 *  It also achieves this without templating, which, IMO, results in clean, concise, readable code
 */

#ifndef INC_APP_COBS_H_
#define INC_APP_COBS_H_

#include <stddef.h> //for size_t
extern "C" {
	#include "stm32g474xx.h" //for uint8_t
}
#include <span> //passing information to and from functions using spans

class Cobs {
private:
	//how many bytes of overhead we need for COBS
	static const size_t OVERHEAD = 3; //start of frame, index of first delimiter, delimiter

public:
	static const size_t MSG_MAX_ENCODED_LENGTH = 256;
	static const size_t MSG_MAX_UNENCODED_LENGTH = MSG_MAX_ENCODED_LENGTH - OVERHEAD;
	static const uint8_t CHAR_DELIMITER = 0x00;
	static const uint8_t CHAR_START_OF_FRAME = 0xFF;
	static const uint8_t CHAR_END_OF_FRAME = CHAR_DELIMITER;

	//pass the unencoded input via a span
	//and dump the encoded output into the memory location pointed by the span
	//return length of the encoded message if encode successful, -1 if not
	static int16_t encode(	const std::span<uint8_t, std::dynamic_extent> input_unencoded,
							std::span<uint8_t, std::dynamic_extent> output_encoded);

	//pass the encoded input via a span
	//and dump the decoded message into the memory location pointed by the span
	//return length of the decoded message if decode successful, -1 if not
	static int16_t decode(	const std::span<uint8_t, std::dynamic_extent> input_encoded,
							std::span<uint8_t, std::dynamic_extent> output_decoded);

};



#endif /* INC_APP_COBS_H_ */
