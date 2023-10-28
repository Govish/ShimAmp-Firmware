/*
 * app_cmhand_test.h
 *
 *  Created on: Oct 1, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___COMMAND_APP_CMHAND_TEST_H_
#define HANDLERS___COMMAND_APP_CMHAND_TEST_H_


//to get request handler types
#include "app_comms_parser.h"
#include "app_cmhand_mapping.h" //to get the mapping for different command handlers

#include <string> //need this for the test string
#include <array> //to hold an stl array
#include <utility> //for pair

#include "app_utils.h" //to initialize std::array with string literal

class Test_Command_Handlers
{
public:

	static Parser::command_handler_sig_t test_byte;
	static Parser::command_handler_sig_t test_uint32;
	static Parser::command_handler_sig_t test_int32;
	static Parser::command_handler_sig_t test_float;
	static Parser::command_handler_sig_t test_string;

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::command_mapping_t, std::dynamic_extent> command_handlers();

	//delete any constructors
	Test_Command_Handlers() = delete;
	Test_Command_Handlers(Test_Command_Handlers const&) = delete;

private:
	static constexpr std::array<Parser::command_mapping_t, 5> COMMAND_HANDLERS = {
			std::make_pair(CM_Mapping::TEST_BYTE, test_byte),
			std::make_pair(CM_Mapping::TEST_UINT32, test_uint32),
			std::make_pair(CM_Mapping::TEST_INT32, test_int32),
			std::make_pair(CM_Mapping::TEST_FLOAT, test_float),
			std::make_pair(CM_Mapping::TEST_STRING, test_string)
	};
};



#endif /* HANDLERS___COMMAND_APP_CMHAND_TEST_H_ */
