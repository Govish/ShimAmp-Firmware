/*
 * app_cmhand_power_stage_ctrl.h
 *
 *  Created on: Oct 5, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___COMMAND_APP_CMHAND_POWER_STAGE_CTRL_H_
#define HANDLERS___COMMAND_APP_CMHAND_POWER_STAGE_CTRL_H_


//to get request handler types
#include "app_comms_parser.h"
#include "app_cmhand_mapping.h" //to get the mapping for different command handlers

#include <string> //need this for the test string
#include <array> //to hold an stl array
#include <utility> //for pair

#include "app_utils.h" //to initialize std::array with string literal

#include "app_power_stage_top_level.h" //to host an array of power stage controls

class Power_Stage_Command_Handlers
{
public:

	//### NOTE: these are all FUNCTION DEFINITIONS with the appropriate signature of a command handler
	static Parser::command_handler_sig_t disable_stage;
	static Parser::command_handler_sig_t enable_stage_manual;
	static Parser::command_handler_sig_t enable_stage_regulator;
	static Parser::command_handler_sig_t enable_stage_autotune;

	static Parser::command_handler_sig_t stage_manual_off;
	static Parser::command_handler_sig_t stage_manual_drive;
	static Parser::command_handler_sig_t stage_manual_duties;
	static Parser::command_handler_sig_t stage_set_fsw; //a little different than the others; can only be set when stage disabled
	//###

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::command_mapping_t, std::dynamic_extent> command_handlers();

	//pass stl container of instantiated power stage systems
	static void attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages);

	//delete any constructors
	Power_Stage_Command_Handlers() = delete;
	Power_Stage_Command_Handlers(Power_Stage_Command_Handlers const&) = delete;

private:
	static constexpr std::array man_confirm_message = s2a("MANUAL"); //match user confirmation message with this to enable manual control
	static constexpr std::array tune_confirm_message = s2a("TUNE"); //match user confirmation message with this to enable autotuning
	static std::span<Power_Stage_Subsystem*, std::dynamic_extent> stages; //have a container that holds a handful of power stages

	static constexpr std::array<Parser::command_mapping_t, 8> COMMAND_HANDLERS = {
			std::make_pair(CM_Mapping::STAGE_DISABLE, disable_stage),
			std::make_pair(CM_Mapping::STAGE_ENABLE_MANUAL, enable_stage_manual),
			std::make_pair(CM_Mapping::STAGE_ENABLE_REGULATOR, enable_stage_regulator),
			std::make_pair(CM_Mapping::STAGE_ENABLE_AUTOTUNING, enable_stage_autotune),
			std::make_pair(CM_Mapping::STAGE_MANUAL_DRIVE_OFF, stage_manual_off),
			std::make_pair(CM_Mapping::STAGE_MANUAL_SET_DRIVE, stage_manual_drive),
			std::make_pair(CM_Mapping::STAGE_MANUAL_SET_DUTIES, stage_manual_duties),
			std::make_pair(CM_Mapping::STAGE_SET_FSW, stage_set_fsw),
	};
};



#endif /* HANDLERS___COMMAND_APP_CMHAND_POWER_STAGE_CTRL_H_ */
