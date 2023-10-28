/*
 * app_rqhand_control.h
 *
 *  Created on: Oct 26, 2023
 *      Author: Ishaan
 */

#ifndef HANDLERS___REQUEST_APP_RQHAND_CONTROL_H_
#define HANDLERS___REQUEST_APP_RQHAND_CONTROL_H_


//to get request handler types
#include "app_comms_parser.h"
#include "app_rqhand_mapping.h" //to get the mapping for different request handlers

#include <span> //for stl span functions
#include <utility> //for pair

#include "app_power_stage_top_level.h" //to host an array of power stage controls

class Controller_Request_Handlers
{
public:

	//### NOTE: these are all FUNCTION DEFINITIONS with the appropriate signature of a request handler
	static Parser::request_handler_sig_t get_rate;
	static Parser::request_handler_sig_t get_crossover;
	static Parser::request_handler_sig_t get_dc_gain;
	static Parser::request_handler_sig_t get_load_res;
	static Parser::request_handler_sig_t get_load_natural_freq;
	//###

	//return some kinda stl-compatible container
	//that contains all the request or command handlers defined in this class
	static std::span<const Parser::command_mapping_t, std::dynamic_extent> request_handlers();

	//pass stl container of instantiated power stage systems
	static void attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages);

	//delete any constructors
	Controller_Request_Handlers() = delete;
	Controller_Request_Handlers(Controller_Request_Handlers const&) = delete;

private:
	//I could establish some kinda friendship between the request and command handlers for the power stage
	//but this keeps the interface a little more explicit which is chill
	static std::span<Power_Stage_Subsystem*, std::dynamic_extent> stages; //have a container that holds a handful of power stages

	static constexpr std::array<Parser::command_mapping_t, 5> REQUEST_HANDLERS = {
			std::make_pair(RQ_Mapping::CONTROL_GET_FREQUENCY, get_rate),
			std::make_pair(RQ_Mapping::CONTROL_GET_CROSSOVER, get_crossover),
			std::make_pair(RQ_Mapping::CONTROL_GET_DC_GAIN, get_dc_gain),
			std::make_pair(RQ_Mapping::LOAD_GET_DC_RESISTANCE, get_load_res),
			std::make_pair(RQ_Mapping::LOAD_GET_NATURAL_FREQ, get_load_natural_freq),
	};
};






#endif /* HANDLERS___REQUEST_APP_RQHAND_CONTROL_H_ */
