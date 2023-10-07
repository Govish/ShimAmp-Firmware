/*
 * app_rqhand_power_stage_status.h
 *
 *  Created on: Oct 7, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___REQUEST_APP_RQHAND_POWER_STAGE_STATUS_H_
#define HANDLERS___REQUEST_APP_RQHAND_POWER_STAGE_STATUS_H_


//to get request handler types
#include "app_comms_parser.h"
#include "app_rqhand_mapping.h" //to get the mapping for different request handlers

#include <span> //for stl span functions
#include <utility> //for pair

#include "app_utils.h" //to initialize std::array with string literal

#include "app_power_stage_top_level.h" //to host an array of power stage controls

class Power_Stage_Request_Handlers
{
public:

	//### NOTE: these are all FUNCTION DEFINITIONS with the appropriate signature of a request handler
	static Parser::request_handler_sig_t stage_get_enable_status;
	static Parser::request_handler_sig_t stage_get_drive;
	static Parser::request_handler_sig_t stage_get_duties;
	static Parser::request_handler_sig_t stage_get_fsw;
	//###

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::command_mapping_t, std::dynamic_extent> request_handlers();

	//pass stl container of instantiated power stage systems
	static void attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages);

	//delete any constructors
	Power_Stage_Request_Handlers() = delete;
	Power_Stage_Request_Handlers(Power_Stage_Request_Handlers const&) = delete;

private:
	//I could establish some kinda friendship between the request and command handlers for the power stage
	//but this keeps the interface a little more explicit which is chill
	static std::span<Power_Stage_Subsystem*, std::dynamic_extent> stages; //have a container that holds a handful of power stages

	static constexpr std::array<Parser::command_mapping_t, 4> REQUEST_HANDLERS = {
			make_pair(RQ_Mapping::STAGE_ENABLE_STATUS, stage_get_enable_status),
			make_pair(RQ_Mapping::STAGE_GET_DRIVE, stage_get_drive),
			make_pair(RQ_Mapping::STAGE_GET_DUTIES, stage_get_duties),
			make_pair(RQ_Mapping::STAGE_GET_FSW, stage_get_fsw),
	};
};



#endif /* HANDLERS___REQUEST_APP_RQHAND_POWER_STAGE_STATUS_H_ */
