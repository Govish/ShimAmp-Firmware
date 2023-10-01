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
	enum RQ_Mapping {
		TEST_BYTE 		= (uint8_t)0x00,
		TEST_UINT32 	= (uint8_t)0x01,
		TEST_INT32 		= (uint8_t)0x02,
		TEST_FLOAT 		= (uint8_t)0x03,
		TEST_STRING 	= (uint8_t)0x04,
	};
}

#endif /* HANDLERS___REQUEST_APP_RQHAND_MAPPING_H_ */
