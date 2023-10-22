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
namespace CM_Mapping {
	enum CM_Mapping : uint8_t {
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
		SETPOINT_SOFT_TRIGGER	= (uint8_t)0x60,
		SETPOINT_DISARM			= (uint8_t)0x61,
		SETPOINT_RESET			= (uint8_t)0x62,
		SETPOINT_DRIVE_DC		= (uint8_t)0x63,

	};
}

#endif /* HANDLERS___COMMAND_APP_CMHAND_MAPPING_H_ */
