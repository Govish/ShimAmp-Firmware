/*
 * app_rqhand_sampler.h
 *
 *  Created on: Oct 26, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___REQUEST_APP_RQHAND_SAMPLER_H_
#define HANDLERS___REQUEST_APP_RQHAND_SAMPLER_H_


//to get request handler types
#include "app_comms_parser.h"
#include "app_rqhand_mapping.h" //to get the mapping for different request handlers

#include <span> //for stl span functions
#include <utility> //for pair

#include "app_power_stage_top_level.h" //to host an array of power stage controls

class Sampler_Request_Handlers
{
public:

	//### NOTE: these are all FUNCTION DEFINITIONS with the appropriate signature of a request handler
	static Parser::request_handler_sig_t read_current;
	static Parser::request_handler_sig_t get_trim_fine;
	static Parser::request_handler_sig_t get_trim_coarse;
	static Parser::request_handler_sig_t get_fine_limits;
	static Parser::request_handler_sig_t read_fine_raw;
	static Parser::request_handler_sig_t read_coarse_raw;
	//###

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::command_mapping_t, std::dynamic_extent> request_handlers();

	//pass stl container of instantiated power stage systems
	static void attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages);

	//delete any constructors
	Sampler_Request_Handlers() = delete;
	Sampler_Request_Handlers(Sampler_Request_Handlers const&) = delete;

private:
	//I could establish some kinda friendship between the request and command handlers for the power stage
	//but this keeps the interface a little more explicit which is chill
	static std::span<Power_Stage_Subsystem*, std::dynamic_extent> stages; //have a container that holds a handful of power stages

	static constexpr std::array<Parser::command_mapping_t, 6> REQUEST_HANDLERS = {
			std::make_pair(RQ_Mapping::SAMPLER_READ_CURRENT, read_current),
			std::make_pair(RQ_Mapping::SAMPLER_GET_TRIM_FINE, get_trim_fine),
			std::make_pair(RQ_Mapping::SAMPLER_GET_TRIM_COARSE, get_trim_coarse),
			std::make_pair(RQ_Mapping::SAMPLER_GET_FINE_LIMITS, get_fine_limits),
			std::make_pair(RQ_Mapping::SAMPLER_READ_FINE_RAW, read_fine_raw),
			std::make_pair(RQ_Mapping::SAMPLER_READ_COARSE_RAW, read_coarse_raw),
	};
};



#endif /* HANDLERS___REQUEST_APP_RQHAND_SAMPLER_H_ */
