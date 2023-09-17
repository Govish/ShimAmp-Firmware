/*
 * app_cobs.h
 *
 *  Created on: Sep 13, 2023
 *      Author: Ishaan
 *
 *  This module serves to perform cobs encoding and decoding as a part of the communications subsystem
 *  It also contains information about the start-of-frame and end-of-frame characters
 *
 *  re: the arrays being passed to the functions as parameters, I know this is possible to pass variable sized arrays using templates
 *  	HOWEVER I can't figure out a way to elegantly do this in an application without requiring aggressive stack usage/copying
 *  		OR some kinda sus reinterpret-cast type stuff to make larger arrays get treated like smaller arrays
 *
 *  	As such, I'm encouraging the passing of fixed-size, statically allocated arrays along with a separate `length` parameter
 *  		This ensures fast, predictable performance, along with some inherent foolproofing that could catch errors re: improperly sized arrays
 */

#ifndef INC_APP_COBS_H_
#define INC_APP_COBS_H_

#include <stddef.h> //for size_t
extern "C" {
	#include "stm32g474xx.h" //for uint8_t
}
#include <array> //passing information to and from functions using arrays

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

	//ensure that this function cannot change the input array
	//also *ensure* arrays of adequate size are unconditionally being passed to the function
	//the `input_length` argument tells us how long the message actually is
	//return length of the encoded message if encode successful, -1 if not
	static int16_t encode(	const std::array<uint8_t, MSG_MAX_UNENCODED_LENGTH>& input_unencoded, const size_t input_length,
							std::array<uint8_t, MSG_MAX_ENCODED_LENGTH>& output_encoded);

	//ensure this function cannot change the input array
	//also *ensure* arrays of adequate size are unconditionally being passed to the function
	//the `input_length` argument tells us how long the message actually is
	//return length of the decoded message if decode successful, -1 if not
	static int16_t decode(	const std::array<uint8_t, MSG_MAX_ENCODED_LENGTH>& input_encoded, const size_t input_length,
							std::array<uint8_t, MSG_MAX_UNENCODED_LENGTH>& output_decoded);

};



#endif /* INC_APP_COBS_H_ */
