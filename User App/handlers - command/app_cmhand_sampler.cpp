/*
 * app_cmhand_sampler.cpp
 *
 *  Created on: Oct 26, 2023
 *      Author: Ishaan
 */



#include "app_cmhand_sampler.h"

#include "app_power_stage_sampler.h" //need this for sampler wrapper
#include "app_utils.h" //for unpacking functions


//================================================= STATIC MEMBER INITIALIZATION =================================================

//initialze as empty at the start; expect to populate this before any commands get processed
std::span<Power_Stage_Subsystem*, std::dynamic_extent> Sampler_Command_Handlers::stages{};

//======================================================== GETTER AND SETTER METHODS/UTILITIES ===================================================

//return some kinda stl-compatible container
//that contains all the request or command handlers defined in this class
std::span<const Parser::command_mapping_t, std::dynamic_extent> Sampler_Command_Handlers::command_handlers() {
	return Sampler_Command_Handlers::COMMAND_HANDLERS;
}

//pass stl container of instantiated power stage systems
void Sampler_Command_Handlers::attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages) {
	//just copy over the span passed into the member variable
	stages = _stages;
}

//======================================================== THE ACTUAL COMMAND HANDLERS ===================================================

/*
 * trim the coarse range on channel `rx_payload[1]` by the following parameters:
 * 	gain :	`rx_payload[2:5]
 * 	offset:	`rx_payload[6:9]
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Command_Handlers::trim_coarse(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																				std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
		uint8_t tx_len;
		if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 10, CM_Mapping::SAMPLER_TRIM_COARSE, tx_len))
			return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

		//grab the particular channel we want to disable and the drive bytes
		size_t channel = rx_payload[1];
		float gain_trim = unpack_float(rx_payload.subspan(2, 4));
		float offset_trim = unpack_float(rx_payload.subspan(6, 4));

		//check if we can index into the appropriate channel number
		if(channel >= stages.size()) {
			tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
			return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
		}

		//grab the reference to the sampler hardware
		Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();

		//try to apply the trimming to the coarse range
		if(!sampler.trim_coarse(gain_trim, offset_trim)) {
			tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED; //could be due to out of range or some other issue with the power stage
			return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
		}

		//respond with an ACK if driving the power stage was successful
		tx_payload[0] = CM_Mapping::SAMPLER_TRIM_COARSE;
		return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * trim the fine range on channel `rx_payload[1]` by the following parameters:
 * 	gain :	`rx_payload[2:5]
 * 	offset:	`rx_payload[6:9]
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Command_Handlers::trim_fine(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																				std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 10, CM_Mapping::SAMPLER_TRIM_FINE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable and the drive bytes
	size_t channel = rx_payload[1];
	float gain_trim = unpack_float(rx_payload.subspan(2, 4));
	float offset_trim = unpack_float(rx_payload.subspan(6, 4));

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the reference to the sampler hardware
	Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();

	//try to apply the trimming to the coarse range
	if(!sampler.trim_fine(gain_trim, offset_trim)) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED; //could be due to out of range or some other issue with the power stage
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//respond with an ACK if driving the power stage was successful
	tx_payload[0] = CM_Mapping::SAMPLER_TRIM_FINE;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * set the readout limits on channel `rx_payload[1]` by the following parameters:
 * 	MINIMUM VALUE :	`rx_payload[2:5]`
 * 	MAXIMUM VALUE:	`rx_payload[6:9]`
 */
std::pair<Parser::MessageType_t, size_t> Sampler_Command_Handlers::set_fine_limits(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 10, CM_Mapping::SAMPLER_SET_FINE_LIMITS, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable and the drive bytes
	size_t channel = rx_payload[1];
	uint32_t min_val = unpack_uint32(rx_payload.subspan(2, 4));
	uint32_t max_val = unpack_uint32(rx_payload.subspan(6, 4));

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the reference to the sampler hardware
	Sampler_Wrapper& sampler = stages[channel]->get_sampler_instance();

	//try to apply the trimming to the coarse range
	if(!sampler.set_limits_fine(min_val, max_val)) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED; //could be due to out of range or some other issue with the power stage
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//respond with an ACK if driving the power stage was successful
	tx_payload[0] = CM_Mapping::SAMPLER_SET_FINE_LIMITS;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}


