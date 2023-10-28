/*
 * app_rqhand_mapping.h
 *
 *  Created on: Sep 21, 2023
 *      Author: Ishaan
 *
 *
 *  File that wraps the indices of the request handler into an enum type
 */

#ifndef HANDLERS___REQUEST_APP_RQHAND_MAPPING_H_
#define HANDLERS___REQUEST_APP_RQHAND_MAPPING_H_

#include <span> //for validate function
#include "app_comms_parser.h" //to get NACK types

extern "C" {
	#include "stm32g474xx.h" //for uint8_t
}

//so we don't pollute our global namespace, but can still access the enum in a "sensical" way
class RQ_Mapping {
public:
	enum Map_Code : uint8_t {
		TEST_BYTE 		= (uint8_t)0x00,
		TEST_UINT32 	= (uint8_t)0x01,
		TEST_INT32 		= (uint8_t)0x02,
		TEST_FLOAT 		= (uint8_t)0x03,
		TEST_STRING 	= (uint8_t)0x04,

		//power stage global information
		STAGE_ENABLE_STATUS		= (uint8_t)0x10,
		STAGE_GET_FSW			= (uint8_t)0x11,

		//power stage requests (fair game whenever, not just in manual mode)
		STAGE_GET_DRIVE			= (uint8_t)0x17,
		STAGE_GET_DUTIES		= (uint8_t)0x18,

		//control-related functions
		CONTROL_GET_FREQUENCY	= (uint8_t)0x21,
		CONTROL_GET_CROSSOVER	= (uint8_t)0x22,
		CONTROL_GET_DC_GAIN		= (uint8_t)0x23,

		//load related reads
		LOAD_GET_DC_RESISTANCE	= (uint8_t)0x31,
		LOAD_GET_NATURAL_FREQ	= (uint8_t)0x32,

		//ADC/sampling related functionality
		SAMPLER_READ_CURRENT	= (uint8_t)0x40,
		SAMPLER_GET_TRIM_FINE	= (uint8_t)0x41,
		SAMPLER_GET_TRIM_COARSE	= (uint8_t)0x42,
		SAMPLER_GET_FINE_LIMITS	= (uint8_t)0x43,
		SAMPLER_READ_FINE_RAW	= (uint8_t)0x44,
		SAMPLER_READ_COARSE_RAW	= (uint8_t)0x45,

		//sampler status and current output value
		SETPOINT_GET_STATUS		= (uint8_t)0x61,
		SETPOINT_GET_WAVE_TYPE	= (uint8_t)0x62,
		SETPOINT_GET_VALUE		= (uint8_t)0x63,
	};

	//utility function to validate formatting for request handlers
	static inline bool VALIDATE_REQUEST(std::span<uint8_t, std::dynamic_extent> tx_buf,
										std::span<uint8_t, std::dynamic_extent> rx_buf,
										const size_t tx_buf_min_size,
										const size_t rx_buf_exact_size,
										const Map_Code redirect,
										uint8_t& tx_to_send)
	{
		//check if TX payload size exists at all
		if(tx_buf.size() <= 0) {
			tx_to_send = 0; //can't even send a NACK message
			return false;
		}

		//check if TX buffer can support a message of a particular length
		if(tx_buf.size() < tx_buf_min_size) {
			tx_buf[0] = Parser::NACK_ERROR_INTERNAL_FW; //have room to drop a specific NACK message
			tx_to_send = 1; //send one byte
			return false;
		}

		//check if we've received the correct number of bytes
		if(rx_buf.size() != rx_buf_exact_size) {
			tx_buf[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE; //wrong size of rx message
			tx_to_send = 1;
			return false;
		}

		//check if we've been redirected to the correct handler
		if(rx_buf[0] != (uint8_t)redirect) {
			tx_buf[0] = Parser::NACK_ERROR_INTERNAL_FW; //something weird happened firmware-side
			tx_to_send = 1;
			return false;
		}

		//all checks pass
		return true;
	}
};

#endif /* HANDLERS___REQUEST_APP_RQHAND_MAPPING_H_ */
