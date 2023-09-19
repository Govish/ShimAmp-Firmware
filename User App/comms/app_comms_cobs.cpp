/*
 * app_cobs.cpp
 *
 *  Created on: Sep 13, 2023
 *      Author: Ishaan
 *
 *  For my own visualization, here's what an unencoded (top) vs encoded message (bot) look like
 *
 *  				[0] [1] [2]       ... 	   [input length - 1]
 *  [0] [    1    ] [2] [3] ... [input length] [input length + 1] [input length + 2]
 *  SOF idxOfDelim1 d0	d1	d2	     dn-1			  dn			   DELIMITER
 */

#include <app_comms_cobs.h>
#include <string.h> //for memcpy

int16_t Cobs::encode(	const std::span<uint8_t, std::dynamic_extent> input_unencoded,
						std::span<uint8_t, std::dynamic_extent> output_encoded)
{
	//sanity check the input length
	if(input_unencoded.size() > Cobs::MSG_MAX_UNENCODED_LENGTH) return -1;

	//create a local variable for output length
	size_t output_length = input_unencoded.size() + Cobs::OVERHEAD;

	//and ensure that the output buffer has enough space to store the encoded message
	if(output_encoded.size() < output_length) return -1;

	//for our COBS encoded message, we'll put our terminating characters in the proper places
	output_encoded[0] = Cobs::CHAR_START_OF_FRAME;
	output_encoded[output_length - 1] = Cobs::CHAR_DELIMITER;
	//and copy over the unencoded data to the output array
	//dump data at the second position since [0] is SOF, and [1] is overhead byte
	std::copy(input_unencoded.begin(), input_unencoded.end(), output_encoded.begin() + 2);

	//work through the output array to figure out where to put the delimiter indices/offsets
	size_t next_delimiter_index = output_length - 1;
	for(size_t i = output_length - 2; i > 1; i--) {
		//if the character at the particular index matches the delimiter
		if(output_encoded[i] == Cobs::CHAR_DELIMITER) {
			//replace the character at that index with an offset to the next delimiter
			//make sure to appropriately cast the index variable to a uint8_t
			output_encoded[i] = (uint8_t)(next_delimiter_index - i);

			//and store the current index as the next_delimiter_index
			next_delimiter_index = i;
		}
	}

	//and finally point to the first delimiter we see in the encoded message
	output_encoded[1] = next_delimiter_index - 1; //subtracting 1 because we're placing this at the first index

	return (int16_t)output_length;
}


int16_t Cobs::decode(	const std::span<uint8_t, std::dynamic_extent> input_encoded,
						std::span<uint8_t, std::dynamic_extent> output_decoded)
{
	//sanity check the input length
	if(input_encoded.size() > Cobs::MSG_MAX_ENCODED_LENGTH || input_encoded.size() < Cobs::OVERHEAD)
		return -1;

	//sanity check that the first character is a start of frame
	if(input_encoded.front() != Cobs::CHAR_START_OF_FRAME)
		return -1;

	//sanity check that the final character is a delimiter
	if(input_encoded.back() != Cobs::CHAR_DELIMITER)
		return -1;

	//sanity check that we have enough space in the output buffer to store the decoded message
	if(output_decoded.size() < input_encoded.size() - Cobs::OVERHEAD)
		return -1;

	//first, just copy over the payload into the decoded buffer
	//I think compiler should be able to optimize this process to run significantly faster than iterating
	std::copy(input_encoded.begin() + 2, input_encoded.end() - 1, output_decoded.begin());

	//now run elementwise through the encoded message
	//replacing all "encoded" characters in the output with delimiters
	size_t next_delimiter_index = input_encoded[1] + 1; //offset by 1 since we're starting at index 1
	for(size_t i = 2; i < input_encoded.size() - 1; i++) {
		//if our index indicates a delimiter in the current position
		if(next_delimiter_index == i) {
			next_delimiter_index += input_encoded[i];
			output_decoded[i-2] = 0;
		}
	}

	//and finally ensure that our next_delimiter_index points at the last element in our buffer
	if(next_delimiter_index != (input_encoded.size() - 1))
		return -1;

	return (int16_t)(input_encoded.size() - Cobs::OVERHEAD);
}

