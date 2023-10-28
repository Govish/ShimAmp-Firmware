/*
 * app_cmhand_setpoint.h
 *
 *  Created on: Oct 25, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___COMMAND_APP_CMHAND_SETPOINT_H_
#define HANDLERS___COMMAND_APP_CMHAND_SETPOINT_H_


//to get request handler types
#include "app_comms_parser.h"
#include "app_cmhand_mapping.h" //to get the mapping for different command handlers

#include <string> //need this for the test string
#include <array> //to hold an stl array
#include <utility> //for pair

#include "app_utils.h" //to initialize std::array with string literal

#include "app_power_stage_top_level.h" //to host an array of power stage controls

class Setpoint_Command_Handlers
{
public:

	//### NOTE: these are all FUNCTION DEFINITIONS with the appropriate signature of a command handler
	static Parser::command_handler_sig_t soft_trigger;
	static Parser::command_handler_sig_t disarm;
	static Parser::command_handler_sig_t reset;
	static Parser::command_handler_sig_t drive_dc;
	//###

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::command_mapping_t, std::dynamic_extent> command_handlers();

	//pass stl container of instantiated power stage systems
	static void attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages);

	//delete any constructors
	Setpoint_Command_Handlers() = delete;
	Setpoint_Command_Handlers(Setpoint_Command_Handlers const&) = delete;

private:
	static std::span<Power_Stage_Subsystem*, std::dynamic_extent> stages; //have a container that holds a handful of power stages

	static constexpr std::array<Parser::command_mapping_t, 4> COMMAND_HANDLERS = {
			std::make_pair(CM_Mapping::SETPOINT_SOFT_TRIGGER, soft_trigger),
			std::make_pair(CM_Mapping::SETPOINT_DISARM, disarm),
			std::make_pair(CM_Mapping::SETPOINT_RESET, reset),
			std::make_pair(CM_Mapping::SETPOINT_DRIVE_DC, drive_dc),
	};
};



#endif /* HANDLERS___COMMAND_APP_CMHAND_SETPOINT_H_ */
