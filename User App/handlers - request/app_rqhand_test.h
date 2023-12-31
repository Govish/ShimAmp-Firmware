/*
 * app_rqhand_test.h
 *
 *  Created on: Sep 20, 2023
 *      Author: Ishaan
 *
 *  Request handlers that can be used for testing communications between device and host
 *  All the methods of this class need to be static for them to be accepted as request handlers
 *  As such, we'll delete the create and copy constructors
 */

#ifndef HANDLERS___REQUEST_APP_RQHAND_TEST_H_
#define HANDLERS___REQUEST_APP_RQHAND_TEST_H_

//to get request handler types
#include "app_comms_parser.h"
#include "app_rqhand_mapping.h" //to get the mapping for different request handlers
#include "app_cmhand_test.h" //so we can make it a friend class

#include <string> //need this for the test string
#include <array> //to hold an stl array
#include <utility> //for pair

#include "app_utils.h" //to initialize std::array with string literal

class Test_Request_Handlers {
	friend class Test_Command_Handlers; //so we can share test sequences with this
public:
	static Parser::request_handler_sig_t test_byte;
	static Parser::request_handler_sig_t test_uint32;
	static Parser::request_handler_sig_t test_int32;
	static Parser::request_handler_sig_t test_float;
	static Parser::request_handler_sig_t test_string;

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::request_mapping_t, std::dynamic_extent> request_handlers();

	//delete any constructors
	Test_Request_Handlers() = delete;
	Test_Request_Handlers(Test_Request_Handlers const&) = delete;

private:
	static const uint8_t the_test_byte = (uint8_t)0xAA;
	static const uint32_t the_test_uint32 = (uint32_t)0xFFABCD00; //make sure to test FF and 00 encoding is correct
	static const int32_t the_test_int32 = (int32_t)(-31415);
	static constexpr float the_test_float = 123.25f;
	static constexpr std::array the_test_string = s2a("Congrats! You decoded this message correctly!\r\n"); //store as array to make encoding easy

	static constexpr std::array<Parser::request_mapping_t, 5> REQUEST_HANDLERS = {
			std::make_pair(RQ_Mapping::TEST_BYTE, test_byte),
			std::make_pair(RQ_Mapping::TEST_UINT32, test_uint32),
			std::make_pair(RQ_Mapping::TEST_INT32, test_int32),
			std::make_pair(RQ_Mapping::TEST_FLOAT, test_float),
			std::make_pair(RQ_Mapping::TEST_STRING, test_string)
	};
};



#endif /* HANDLERS___REQUEST_APP_RQHAND_TEST_H_ */
