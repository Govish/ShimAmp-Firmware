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

namespace CM_Mapping {
	enum CM_Mapping {
		TEST_BYTE 		= (uint8_t)0x00,
		TEST_UINT32 	= (uint8_t)0x01,
		TEST_INT32 		= (uint8_t)0x02,
		TEST_FLOAT 		= (uint8_t)0x03,
		TEST_STRING 	= (uint8_t)0x04,
	};
}

#endif /* HANDLERS___COMMAND_APP_CMHAND_MAPPING_H_ */
