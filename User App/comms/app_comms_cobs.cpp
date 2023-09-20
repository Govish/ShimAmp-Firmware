/*
 * app_cobs.cpp
 *
 *  Created on: Sep 13, 2023
 *      Author: Ishaan
 *
 *  For my own visualization, here's what an unencoded (top) vs encoded message (bot) look like
 *
 *  							[0] [1] [2]       ...		   [input length - 1]
 *  [0] [	 1	  ]	[    2    ] [3] [4] ... [input length + 1] [input length + 2] [input length + 3]
 *  SOF idxOfDelSOF idxOfDelEOF d0	d1	d2	     dn-1			  dn				 EOF
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
	output_encoded[output_length - 1] = Cobs::CHAR_END_OF_FRAME;

	//and copy over the unencoded data to the output array
	//dump data at the third position since [0] is SOF, [1] is overhead byte for SOF, [2] is overhead byte for EOF
	std::copy(input_unencoded.begin(), input_unencoded.end(), output_encoded.begin() + IDX_START_OF_PAYLOAD);

	//work through the output array to figure out where to put the delimiter indices/offsets
	size_t next_sof_char_index = output_length - 1;
	size_t next_eof_char_index = output_length - 1;

	//start iterating from back to front, starting at the last byte of our unencoded packet
	for(size_t i = output_length - 2; i >= IDX_START_OF_PAYLOAD; i--) {
		//if the character at the particular index matches our SOF
		if(output_encoded[i] == Cobs::CHAR_START_OF_FRAME) {
			//replace the character at that index with an offset to the next SOF character we find
			//make sure to appropriately cast the index variable to a uint8_t
			output_encoded[i] = (uint8_t)(next_sof_char_index - i);

			//and store the current index as the next_delimiter_index
			next_sof_char_index = i;
		}

		//do the same kinda thing, but look for EOF characters
		if(output_encoded[i] == Cobs::CHAR_END_OF_FRAME) {
			output_encoded[i] = (uint8_t)(next_eof_char_index - i);
			next_eof_char_index = i;
		}

	}

	//at index [1], point to the first SOF char we see in our message
	output_encoded[1] = next_sof_char_index - 1; //subtracting 1 because we're placing this at the first index

	//at index [2], point to the first EOF char we see in our message
	output_encoded[2] = next_eof_char_index - 1; //subtracting 1 because we're placing this at the first index

	//return the size of our encoded array; consistent overhead means this is just a fixed amount smaller than the unencoded size
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

	//sanity check that the final character is an end of frame
	if(input_encoded.back() != Cobs::CHAR_END_OF_FRAME)
		return -1;

	//sanity check that we have enough space in the output buffer to store the decoded message
	if(output_decoded.size() < input_encoded.size() - Cobs::OVERHEAD)
		return -1;

	//first, just copy over the payload into the decoded buffer
	//I think compiler should be able to optimize this process to run significantly faster than iterating
	std::copy(input_encoded.begin() + IDX_START_OF_PAYLOAD, input_encoded.end() - 1, output_decoded.begin());

	//now run elementwise through the encoded message
	//replacing all "encoded" characters in the output with delimiters
	size_t next_sof_char_index = input_encoded[1] + 1; //offset by 1 since we're starting at index 1
	size_t next_eof_char_index = input_encoded[2] + 2; //offset by 2 since we're starting at index 2

	for(size_t i = IDX_START_OF_PAYLOAD; i < input_encoded.size() - 1; i++) {
		//if our index indicates a SOF character in the particular position
		if(next_sof_char_index == i) {
			next_sof_char_index += input_encoded[i];
			output_decoded[i-IDX_START_OF_PAYLOAD] = CHAR_START_OF_FRAME; //indices are offset since we're in the frame of reference of the encoded packet
		}

		//do a similar thing when hunting for EOF characters
		if(next_eof_char_index == i) {
			next_eof_char_index += input_encoded[i];
			output_decoded[i-IDX_START_OF_PAYLOAD] = CHAR_END_OF_FRAME; //indices are offset since we're in the frame of reference of the encoded packet
		}
	}

	//and finally ensure that our delimiter indices point to the last element in our buffer
	if(next_sof_char_index != (input_encoded.size() - 1)) return -1;
	if(next_eof_char_index != (input_encoded.size() - 1)) return -1;

	//return the size of our decoded array; consistent overhead means this is just a fixed amount smaller than the encoded size
	return (int16_t)(input_encoded.size() - Cobs::OVERHEAD);
}

