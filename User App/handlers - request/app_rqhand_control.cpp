/*
 * app_rqhand_control.cpp
 *
 *  Created on: Oct 26, 2023
 *      Author: Ishaan
 */



#include "app_rqhand_control.h"

#include "app_control_regulator.h" //to get access to the regulator
#include "app_utils.h" //for packing

//================================================= STATIC MEMBER INITIALIZATION =================================================

//initialze as empty at the start; expect to populate this before any commands get processed
std::span<Power_Stage_Subsystem*, std::dynamic_extent> Controller_Request_Handlers::stages{};

//======================================================== GETTER AND SETTER METHODS/UTILITIES ===================================================

//return some kinda stl-compatible container
//that contains all the request or command handlers defined in this class
std::span<const Parser::command_mapping_t, std::dynamic_extent> Controller_Request_Handlers::request_handlers() {
	return Controller_Request_Handlers::REQUEST_HANDLERS;
}

//pass stl container of instantiated power stage systems
void Controller_Request_Handlers::attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages) {
	//just copy over the span passed into the member variable
	stages = _stages;
}

//======================================================== THE ACTUAL REQUEST HANDLERS ===================================================

/*
 * Get the controller frequency of all channels
 * won't have any other parameters to the request
 */
std::pair<Parser::MessageType_t, size_t> Controller_Request_Handlers::get_rate(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																				std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 5, 1, RQ_Mapping::CONTROL_GET_FREQUENCY, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the switching frequency of all power stages
	float fc_hz = Power_Stage_Subsystem::get_controller_frequency();

	//everything's kosher --> encode the channel drive value into the tx payload
	tx_payload[0] = RQ_Mapping::STAGE_GET_FSW; //this is the request we serviced
	pack(fc_hz, tx_payload.subspan(1, 4)); //put the positive switching frequency starting at index 1, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 5); //and return an ack message along with a five-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = DC_gain
 */
std::pair<Parser::MessageType_t, size_t> Controller_Request_Handlers::get_dc_gain(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::CONTROL_GET_DC_GAIN, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the regulator instance corresponding to the particular channel
	Regulator_Wrapper& regulator = stages[channel]->get_regulator_instance();

	//grab the grab the forward path gain of the feedback system
	float dc_gain = regulator.get_gain();

	//everything's kosher --> encode the dc gain value into the tx payload
	tx_payload[0] = RQ_Mapping::CONTROL_GET_DC_GAIN; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(dc_gain, tx_payload.subspan(2, 4)); //put the gain value starting at index 2, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a six-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = crossover
 */
std::pair<Parser::MessageType_t, size_t> Controller_Request_Handlers::get_crossover(const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::CONTROL_GET_CROSSOVER, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the regulator instance corresponding to the particular channel
	Regulator_Wrapper& regulator = stages[channel]->get_regulator_instance();

	//grab the grab the crossover frequency of the feedback system
	float crossover = regulator.get_crossover_freq();

	//everything's kosher --> encode the crossover frequency value into the tx payload
	tx_payload[0] = RQ_Mapping::CONTROL_GET_CROSSOVER; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(crossover, tx_payload.subspan(2, 4)); //put the crossover value starting at index 2, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a six-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = load resistance
 */
std::pair<Parser::MessageType_t, size_t> Controller_Request_Handlers::get_load_res(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::LOAD_GET_DC_RESISTANCE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the regulator instance corresponding to the particular channel
	Regulator_Wrapper& regulator = stages[channel]->get_regulator_instance();

	//grab the grab the crossover frequency of the feedback system
	float load_res = regulator.get_crossover_freq();

	//everything's kosher --> encode the dc resistance value into the tx payload
	tx_payload[0] = RQ_Mapping::LOAD_GET_DC_RESISTANCE; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(load_res, tx_payload.subspan(2, 4)); //put the dc resistance value starting at index 2, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a six-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = load natural frequency
 */
std::pair<Parser::MessageType_t, size_t> Controller_Request_Handlers::get_load_natural_freq(const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::LOAD_GET_NATURAL_FREQ, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wanna query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the regulator instance corresponding to the particular channel
	Regulator_Wrapper& regulator = stages[channel]->get_regulator_instance();

	//grab the grab the crossover frequency of the feedback system
	float load_wn = regulator.get_load_natural_freq();

	//everything's kosher --> encode the load natural frequency value into the tx payload
	tx_payload[0] = RQ_Mapping::LOAD_GET_NATURAL_FREQ; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(load_wn, tx_payload.subspan(2, 4)); //put the load natural frequency value starting at index 2, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a six-byte payload
}



