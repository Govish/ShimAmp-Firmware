/*
 * app_cmhand_setpoint.cpp
 *
 *  Created on: Oct 25, 2023
 *      Author: Ishaan
 */

#include "app_cmhand_setpoint.h"

#include "app_setpoint_controller.h" //access setpoint controller functions


//================================================= STATIC MEMBER INITIALIZATION =================================================

//initialze as empty at the start; expect to populate this before any commands get processed
std::span<Power_Stage_Subsystem*, std::dynamic_extent> Setpoint_Command_Handlers::stages{};

//======================================================== GETTER AND SETTER METHODS/UTILITIES ===================================================

//return some kinda stl-compatible container
//that contains all the request or command handlers defined in this class
std::span<const Parser::command_mapping_t, std::dynamic_extent> Setpoint_Command_Handlers::command_handlers() {
	return Setpoint_Command_Handlers::COMMAND_HANDLERS;
}

//pass stl container of instantiated power stage systems
void Setpoint_Command_Handlers::attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages) {
	//just copy over the span passed into the member variable
	stages = _stages;
}

//======================================================== THE ACTUAL COMMAND HANDLERS ===================================================

/*
 * perform a software trigger of the setpoint controller on channel `rx_payload[1]`
 */
std::pair<Parser::MessageType_t, size_t> Setpoint_Command_Handlers::soft_trigger(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																					std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 2, CM_Mapping::SETPOINT_SOFT_TRIGGER, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	/*
	 * ACTUALLY EXECUTE THE COMMAND HERE
	 */
	if(false /*TODO SETPOINT SOFT TRIGGER NOT IMPLEMENTED YET*/) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::SETPOINT_SOFT_TRIGGER;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * disarm the setpoint controller after it had been armed by a waveform upon trigger
 * operate on channel `rx_payload[1]`
 */
std::pair<Parser::MessageType_t, size_t> Setpoint_Command_Handlers::disarm(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 2, CM_Mapping::SETPOINT_DISARM, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	/*
	 * ACTUALLY EXECUTE THE COMMAND HERE
	 */
	if(false /*TODO SETPOINT SOFT TRIGGER NOT IMPLEMENTED YET*/) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::SETPOINT_DISARM;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * RESET the setpoint controller such that we're commanding zero current
 * rx_payload[1] = channel
 */
std::pair<Parser::MessageType_t, size_t> Setpoint_Command_Handlers::reset(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 2, CM_Mapping::SETPOINT_RESET, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable
	size_t channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the setpoint control from the particular power stage channel
	Setpoint_Wrapper& setpoint_controller = stages[channel]->get_setpoint_instance();
	if(!setpoint_controller.reset_setpoint()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED; //setpoint might not be enabled
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::SETPOINT_RESET;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * DRIVE a DC current into the load -- generally useful for commanding currents
 * rx_payload[1] = channel
 * rx_payload[2] = trigger gated (true) or immediate (false)
 * rx_payload[3:6] = current value
 */
std::pair<Parser::MessageType_t, size_t> Setpoint_Command_Handlers::drive_dc(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																				std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 7, CM_Mapping::SETPOINT_DRIVE_DC, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable and the other details
	size_t channel = rx_payload[1];
	bool trigger_gating = rx_payload[2] > 0;
	float current_setpoint = unpack_float(rx_payload.subspan(3, 4));

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the setpoint control instance from the particular power stage channel instance
	Setpoint_Wrapper& setpoint_controller = stages[channel]->get_setpoint_instance();
	if(!setpoint_controller.make_setpoint_dc(trigger_gating, current_setpoint)) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED; //setpoint might not be enabled
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::SETPOINT_DRIVE_DC;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}
