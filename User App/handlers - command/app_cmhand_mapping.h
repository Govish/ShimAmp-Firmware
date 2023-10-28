/*
 * app_cmhand_mapping.h
 *
 *  Created on: Oct 1, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___COMMAND_APP_CMHAND_MAPPING_H_
#define HANDLERS___COMMAND_APP_CMHAND_MAPPING_H_

extern "C" {
	#include "stm32g474xx.h" //for uint8_t
}

//dropping in its own namespace in order to not pollute global namespace
//very annoying that there isn't an easy way to check that no enum values overlap
class CM_Mapping {
public:
	enum Map_Code : uint8_t {
		TEST_BYTE 		= (uint8_t)0x00,
		TEST_UINT32 	= (uint8_t)0x01,
		TEST_INT32 		= (uint8_t)0x02,
		TEST_FLOAT 		= (uint8_t)0x03,
		TEST_STRING 	= (uint8_t)0x04,

		//fundamental stage-related commands
		STAGE_DISABLE			= (uint8_t)0x10,
		STAGE_SET_FSW			= (uint8_t)0x11,

		//manual power stage commands
		STAGE_ENABLE_MANUAL		= (uint8_t)0x15,
		STAGE_MANUAL_DRIVE_OFF	= (uint8_t)0x16,
		STAGE_MANUAL_SET_DRIVE	= (uint8_t)0x17,
		STAGE_MANUAL_SET_DUTIES	= (uint8_t)0x18,

		//control related functionality
		STAGE_ENABLE_REGULATOR 	= (uint8_t)0x20,
		CONTROL_SET_FREQUENCY	= (uint8_t)0x21,
		CONTROL_SET_CROSSOVER	= (uint8_t)0x22,
		CONTROL_SET_DC_GAIN		= (uint8_t)0x23,

		//load related functionality
		STAGE_ENABLE_AUTOTUNING	= (uint8_t)0x30,
		LOAD_SET_DC_RESISTANCE	= (uint8_t)0x31,
		LOAD_SET_NATURAL_FREQ	= (uint8_t)0x32,

		//raw value reading/ADC trimming
		//reserving 0x40 for reading current
		SAMPLER_TRIM_FINE		= (uint8_t)0x41,
		SAMPLER_TRIM_COARSE 	= (uint8_t)0x42,
		SAMPLER_SET_FINE_LIMITS	= (uint8_t)0x43,

		//setpoint control functions
		//TODO: SETPOINT TICK FREQUENCY, SETPOINT BANDWIDTH
		SETPOINT_SOFT_TRIGGER	= (uint8_t)0x60,
		SETPOINT_DISARM			= (uint8_t)0x61,
		SETPOINT_RESET			= (uint8_t)0x62,
		SETPOINT_DRIVE_DC		= (uint8_t)0x63,

	};

	//utility function to validate formatting for request handlers
	static inline bool VALIDATE_COMMAND(std::span<uint8_t, std::dynamic_extent> tx_buf,
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

#endif /* HANDLERS___COMMAND_APP_CMHAND_MAPPING_H_ */
