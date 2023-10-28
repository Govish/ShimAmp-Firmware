/*
 * app_cmhand_control.h
 *
 *  Created on: Oct 26, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___COMMAND_APP_CMHAND_CONTROL_H_
#define HANDLERS___COMMAND_APP_CMHAND_CONTROL_H_


//to get request handler types
#include "app_comms_parser.h"
#include "app_cmhand_mapping.h" //to get the mapping for different command handlers

#include "app_power_stage_top_level.h" //to host an array of power stage controls

class Controller_Command_Handlers
{
public:

	//### NOTE: these are all FUNCTION DEFINITIONS with the appropriate signature of a command handler
	static Parser::command_handler_sig_t set_controller_rate;
	static Parser::command_handler_sig_t set_controller_crossover;
	static Parser::command_handler_sig_t set_controller_gain;
	static Parser::command_handler_sig_t set_load_resistance;
	static Parser::command_handler_sig_t set_load_natural_freq;
	//###

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::command_mapping_t, std::dynamic_extent> command_handlers();

	//pass stl container of instantiated power stage systems
	static void attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages);

	//delete any constructors
	Controller_Command_Handlers() = delete;
	Controller_Command_Handlers(Controller_Command_Handlers const&) = delete;

private:
	static std::span<Power_Stage_Subsystem*, std::dynamic_extent> stages; //have a container that holds a handful of power stages

	static constexpr std::array<Parser::command_mapping_t, 5> COMMAND_HANDLERS = {
			std::make_pair(CM_Mapping::CONTROL_SET_FREQUENCY, set_controller_rate),
			std::make_pair(CM_Mapping::CONTROL_SET_DC_GAIN, set_controller_crossover),
			std::make_pair(CM_Mapping::CONTROL_SET_CROSSOVER, set_controller_gain),
			std::make_pair(CM_Mapping::LOAD_SET_DC_RESISTANCE, set_load_resistance),
			std::make_pair(CM_Mapping::LOAD_SET_NATURAL_FREQ, set_load_natural_freq),
	};
};



#endif /* HANDLERS___COMMAND_APP_CMHAND_CONTROL_H_ */
