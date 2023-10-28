/*
 * app_rqhand_power_stage_status.cpp
 *
 *  Created on: Oct 7, 2023
 *      Author: Ishaan
 */

#include "app_rqhand_power_stage_status.h"

#include "app_power_stage_drive.h" //for power stage wrapper

//================================================= STATIC MEMBER INITIALIZATION =================================================

//initialze as empty at the start; expect to populate this before any commands get processed
std::span<Power_Stage_Subsystem*, std::dynamic_extent> Power_Stage_Request_Handlers::stages{};

//======================================================== GETTER AND SETTER METHODS/UTILITIES ===================================================

//return some kinda stl-compatible container
//that contains all the request or command handlers defined in this class
std::span<const Parser::command_mapping_t, std::dynamic_extent> Power_Stage_Request_Handlers::request_handlers() {
	return Power_Stage_Request_Handlers::REQUEST_HANDLERS;
}

//pass stl container of instantiated power stage systems
void Power_Stage_Request_Handlers::attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages) {
	//just copy over the span passed into the member variable
	stages = _stages;
}

//======================================================== THE ACTUAL REQUEST HANDLERS ===================================================

/*
 * get the `enable_status` of channel `rx_packet[1]`
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Request_Handlers::stage_get_enable_status(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
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

	//grab the power stage instance corresponding to the particular channel
	uint8_t stage_mode = (uint8_t)stages[channel]->get_mode();

	//everything's kosher --> encode the channel enable status value into the tx payload
	tx_payload[0] = RQ_Mapping::STAGE_ENABLE_STATUS; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	tx_payload[2] = stage_mode; //encode the enumeration for the stage enable mode into this byte
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 3); //and return an ack message along with a three-byte payload
}

/*
 * get the drive of channel `rx_packet[1]`
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Request_Handlers::stage_get_drive(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																						std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 6, 2, RQ_Mapping::STAGE_GET_DRIVE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the power stage instance corresponding to the particular channel
	Power_Stage_Wrapper& stage = stages[channel]->get_direct_stage_control_instance();

	//grab the channel drive value from the particular power stage (should always be able to do this)
	float channel_drive = stage.get_drive_duty();

	//everything's kosher --> encode the channel drive value into the tx payload
	tx_payload[0] = RQ_Mapping::STAGE_GET_DRIVE; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(channel_drive, tx_payload.subspan(2, 4)); //put the drive_value starting at index 2, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 6); //and return an ack message along with a six-byte payload
}

/*
 * get the duty cycles of channel `rx_packet[1]`
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Request_Handlers::stage_get_duties(const std::span<uint8_t, std::dynamic_extent> rx_payload,
																						std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 10, 2, RQ_Mapping::STAGE_GET_DUTIES, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the channel we wann query
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the power stage instance corresponding to the particular channel
	Power_Stage_Wrapper& stage = stages[channel]->get_direct_stage_control_instance();

	//grab the channel duty cycle value from the particular power stage (should always be able to do this)
	auto [drive_pos, drive_neg] = stage.get_drive_halves();

	//everything's kosher --> encode the channel drive value into the tx payload
	tx_payload[0] = RQ_Mapping::STAGE_GET_DUTIES; //this is the request we serviced
	tx_payload[1] = (uint8_t)channel; //encode the particular channel this request corresponds to
	pack(drive_pos, tx_payload.subspan(2, 4)); //put the positive duty cycle starting at index 2, taking 4 bytes
	pack(drive_neg, tx_payload.subspan(6, 4)); //put the negative duty cycle starting at index 6, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 10); //and return an ack message along with a six-byte payload
}

/*
 * Get the switching frequency of all channels
 * won't have any other parameters to the request
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Request_Handlers::stage_get_fsw(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																						std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 5, 1, RQ_Mapping::STAGE_GET_FSW, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the switching frequency of all power stages
	float fsw_hz = Power_Stage_Subsystem::get_switching_frequency();

	//everything's kosher --> encode the channel drive value into the tx payload
	tx_payload[0] = RQ_Mapping::STAGE_GET_FSW; //this is the request we serviced
	pack(fsw_hz, tx_payload.subspan(1, 4)); //put the positive switching frequency starting at index 1, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 5); //and return an ack message along with a five-byte payload
}

