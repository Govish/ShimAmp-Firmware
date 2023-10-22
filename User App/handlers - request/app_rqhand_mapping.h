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

extern "C" {
	#include "stm32g474xx.h" //for uint8_t
}

//so we don't pollute our global namespace, but can still access the enum in a "sensical" way
namespace RQ_Mapping {
	enum RQ_Mapping : uint8_t {
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
		SAMPLER_READ_FINE_RAW	= (uint8_t)0x41,
		SAMPLER_READ_COARSE_RAW	= (uint8_t)0x42,
		SAMPLER_GET_FINE_LIMITS	= (uint8_t)0x43,

		//sampler status and current output value
		SETPOINT_GET_STATUS		= (uint8_t)0x61,
		SETPOINT_GET_WAVE_TYPE	= (uint8_t)0x62,
		SETPOINT_GET_VALUE		= (uint8_t)0x63,

	};
}

#endif /* HANDLERS___REQUEST_APP_RQHAND_MAPPING_H_ */
