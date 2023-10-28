/*
 * app_cmhand_sampler.h
 *
 *  Created on: Oct 26, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___COMMAND_APP_CMHAND_SAMPLER_H_
#define HANDLERS___COMMAND_APP_CMHAND_SAMPLER_H_

#include <utility> //for make pair

//to get request handler types
#include "app_comms_parser.h"
#include "app_cmhand_mapping.h" //to get the mapping for different command handlers

#include "app_power_stage_top_level.h" //to host an array of power stage controls

class Sampler_Command_Handlers
{
public:

	//### NOTE: these are all FUNCTION DEFINITIONS with the appropriate signature of a command handler
	static Parser::command_handler_sig_t trim_fine;
	static Parser::command_handler_sig_t trim_coarse;
	static Parser::command_handler_sig_t set_fine_limits;
	//###

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::command_mapping_t, std::dynamic_extent> command_handlers();

	//pass stl container of instantiated power stage systems
	static void attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages);

	//delete any constructors
	Sampler_Command_Handlers() = delete;
	Sampler_Command_Handlers(Sampler_Command_Handlers const&) = delete;

private:
	static std::span<Power_Stage_Subsystem*, std::dynamic_extent> stages; //have a container that holds a handful of power stages

	static constexpr std::array<Parser::command_mapping_t, 3> COMMAND_HANDLERS = {
			std::make_pair(CM_Mapping::SAMPLER_TRIM_FINE, trim_fine),
			std::make_pair(CM_Mapping::SAMPLER_TRIM_COARSE, trim_coarse),
			std::make_pair(CM_Mapping::SAMPLER_SET_FINE_LIMITS, set_fine_limits),
	};
};





#endif /* HANDLERS___COMMAND_APP_CMHAND_SAMPLER_H_ */
