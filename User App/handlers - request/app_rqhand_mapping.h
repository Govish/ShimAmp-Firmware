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

enum RQ_Mapping {
	TEST_BYTE 		= (size_t)0x00,
	TEST_UINT32 	= (size_t)0x01,
	TEST_INT32 		= (size_t)0x02,
	TEST_FLOAT 		= (size_t)0x03,
	TEST_STRING 	= (size_t)0x04,
};

#endif /* HANDLERS___REQUEST_APP_RQHAND_MAPPING_H_ */
