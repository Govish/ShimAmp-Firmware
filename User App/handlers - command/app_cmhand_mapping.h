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

		//commands to disable/enable the power stage in different operating modes
		STAGE_DISABLE			= (uint8_t)0x10,
		STAGE_ENABLE_REGULATOR 	= (uint8_t)0x11,
		STAGE_ENABLE_MANUAL		= (uint8_t)0x12,
		STAGE_ENABLE_AUTOTUNING	= (uint8_t)0x13,

		//set the switching frequency of the power stage
		STAGE_SET_FSW			= (uint8_t)0x14,

		//manual power stage commands
		STAGE_MANUAL_DRIVE_OFF	= (uint8_t)0x16,
		STAGE_MANUAL_SET_DRIVE	= (uint8_t)0x1A,
		STAGE_MANUAL_SET_DUTIES	= (uint8_t)0x1E,
	};
}

#endif /* HANDLERS___COMMAND_APP_CMHAND_MAPPING_H_ */
