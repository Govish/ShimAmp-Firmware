/*
 * app_rqhand_setpoint.cpp
 *
 *  Created on: Oct 25, 2023
 *      Author: Ishaan
 */


#include "app_rqhand_setpoint.h"

#include "app_setpoint_controller.h" //to get access to setpoint functions
#include "app_utils.h" //for float packing

//================================================= STATIC MEMBER INITIALIZATION =================================================

//initialze as empty at the start; expect to populate this before any commands get processed
std::span<Power_Stage_Subsystem*, std::dynamic_extent> Setpoint_Request_Handlers::stages{};

//======================================================== GETTER AND SETTER METHODS/UTILITIES ===================================================

//return some kinda stl-compatible container
//that contains all the request or command handlers defined in this class
std::span<const Parser::command_mapping_t, std::dynamic_extent> Setpoint_Request_Handlers::request_handlers() {
	return Setpoint_Request_Handlers::REQUEST_HANDLERS;
}

//pass stl container of instantiated power stage systems
void Setpoint_Request_Handlers::attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages) {
	//just copy over the span passed into the member variable
	stages = _stages;
}

//======================================================== THE ACTUAL REQUEST HANDLERS ===================================================

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2] = status_code
 */
std::pair<Parser::MessageType_t, size_t> Setpoint_Request_Handlers::get_status(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																				std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 3, 2, RQ_Mapping::STAGE_ENABLE_STATUS, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the setpoint controller instance
	Setpoint_Wrapper& setpoint_controller = stages[channel]->get_setpoint_instance();
	uint8_t setpoint_status_code = (uint8_t)0; //TODO

	//everything's kosher --> encode the setpoint status value into the tx payload
	tx_payload[0] = RQ_Mapping::SETPOINT_GET_STATUS; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	tx_payload[2] = setpoint_status_code; //encode the enumeration for the setpoint status into this byte
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 3); //and return an ack message along with a three-byte payload
}


/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:n] = wave_type and params
 */
std::pair<Parser::MessageType_t, size_t> Setpoint_Request_Handlers::get_wave_type(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 3, 2, RQ_Mapping::SETPOINT_GET_WAVE_TYPE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the setpoint controller instance
	Setpoint_Wrapper& setpoint_controller = stages[channel]->get_setpoint_instance();
	uint8_t setpoint_wave_type = (uint8_t)0; //TODO

	//everything's kosher --> encode the setpoint wave type into the tx payload
	tx_payload[0] = RQ_Mapping::SETPOINT_GET_WAVE_TYPE; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	tx_payload[2] = setpoint_wave_type; //TODO: encode wave type and information into here
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 3); //and return an ack message along with a n-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = setpoint value
 */
std::pair<Parser::MessageType_t, size_t> Setpoint_Request_Handlers::get_value(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																				std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::SETPOINT_GET_VALUE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the setpoint controller instance
	Setpoint_Wrapper& setpoint_controller = stages[channel]->get_setpoint_instance();
	float current_setpoint = 0; //TODO pull the active setpoint

	//everything's kosher --> encode the channel enable status value into the tx payload
	tx_payload[0] = RQ_Mapping::SETPOINT_GET_VALUE; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(current_setpoint, tx_payload.subspan(2, 4)); //pack the setpoint into the next couple of bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a six-byte payload
}

