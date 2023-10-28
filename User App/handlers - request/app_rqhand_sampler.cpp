/*
 * app_rqhand_sampler.cpp
 *
 *  Created on: Oct 26, 2023
 *      Author: Ishaan
 */



#include "app_rqhand_sampler.h"

#include "app_power_stage_sampler.h" //to get access to sampler functions
#include "app_utils.h" //for float packing

//================================================= STATIC MEMBER INITIALIZATION =================================================

//initialze as empty at the start; expect to populate this before any commands get processed
std::span<Power_Stage_Subsystem*, std::dynamic_extent> Sampler_Request_Handlers::stages{};

//======================================================== GETTER AND SETTER METHODS/UTILITIES ===================================================

//return some kinda stl-compatible container
//that contains all the request or command handlers defined in this class
std::span<const Parser::command_mapping_t, std::dynamic_extent> Sampler_Request_Handlers::request_handlers() {
	return Sampler_Request_Handlers::REQUEST_HANDLERS;
}

//pass stl container of instantiated power stage systems
void Sampler_Request_Handlers::attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages) {
	//just copy over the span passed into the member variable
	stages = _stages;
}

//======================================================== THE ACTUAL REQUEST HANDLERS ===================================================

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = current readout
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Request_Handlers::read_current(const std::span<uint8_t, std::dynamic_extent> rx_payload,
																				std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::SAMPLER_READ_CURRENT, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the sampler instance and read the current
	Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();
	float current_reading = sampler.get_current_reading();

	//everything's kosher --> encode the channel + its current reading into the tx payload
	tx_payload[0] = RQ_Mapping::SAMPLER_READ_CURRENT; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(current_reading, tx_payload.subspan(2, 4)); //pack the reading into the next couple of bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a three-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = gain_fine
 * tx_packet[6:9] = offset_fine
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Request_Handlers::get_trim_fine(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 10, 2, RQ_Mapping::SAMPLER_GET_TRIM_FINE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the sampler instance and read the trim values
	Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();
	auto [gain, offset] = sampler.get_trim_fine();

	//everything's kosher --> encode the channel and its trim into the tx payload
	tx_payload[0] = RQ_Mapping::SAMPLER_GET_TRIM_FINE; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(gain, tx_payload.subspan(2, 4)); //put the gain at index 2, taking 4 bytes
	pack(offset, tx_payload.subspan(6, 4)); //put the offset at index 2, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 10); //and return an ack message along with a six-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = gain_coarse
 * tx_packet[6:9] = offset_coarse
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Request_Handlers::get_trim_coarse(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 10, 2, RQ_Mapping::SAMPLER_GET_TRIM_COARSE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the sampler instance and read the trim values
	Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();
	auto [gain, offset] = sampler.get_trim_coarse();

	//everything's kosher --> encode the channel and its trim into the tx payload
	tx_payload[0] = RQ_Mapping::SAMPLER_GET_TRIM_COARSE; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(gain, tx_payload.subspan(2, 4)); //put the gain at index 2, taking 4 bytes
	pack(offset, tx_payload.subspan(6, 4)); //put the offset at index 6, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 10); //and return an ack message along with a six-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = lower boundary count
 * tx_packet[6:9] = higher boundary count
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Request_Handlers::get_fine_limits(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 10, 2, RQ_Mapping::SAMPLER_GET_FINE_LIMITS, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the sampler instance and read the limits
	Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();
	auto [low_lim, high_lim] = sampler.get_trim_coarse();

	//everything's kosher --> encode the channel and its trim into the tx payload
	tx_payload[0] = RQ_Mapping::SAMPLER_GET_FINE_LIMITS; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack((uint32_t)low_lim, tx_payload.subspan(2, 4)); //put the lower limit at index 2, taking 4 bytes
	pack((uint32_t)high_lim, tx_payload.subspan(6, 4)); //put the upper limit at index 6, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 10); //and return an ack message along with a six-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = fine channel raw readout
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Request_Handlers::read_fine_raw(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::SAMPLER_READ_FINE_RAW, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the sampler instance and read the current
	Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();
	uint32_t raw_reading = sampler.read_fine_raw();

	//everything's kosher --> encode the channel + its raw reading into the tx payload
	tx_payload[0] = RQ_Mapping::SAMPLER_READ_FINE_RAW; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(raw_reading, tx_payload.subspan(2, 4)); //pack the reading into the next couple of bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a three-byte payload
}

/*
 * rx_packet[1] = channel
 *
 * tx_packet[1] = channel
 * tx_packet[2:5] = coarse channel raw readout
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Request_Handlers::read_coarse_raw(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::SAMPLER_READ_COARSE_RAW, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//get the sampler instance and read the current
	Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();
	uint32_t raw_reading = sampler.read_coarse_raw();

	//everything's kosher --> encode the channel + its raw reading into the tx payload
	tx_payload[0] = RQ_Mapping::SAMPLER_READ_COARSE_RAW; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(raw_reading, tx_payload.subspan(2, 4)); //pack the reading into the next couple of bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a three-byte payload
}

