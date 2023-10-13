/*
 * app_cmhand_power_stage_ctrl.cpp
 *
 *  Created on: Oct 5, 2023
 *      Author: Ishaan
 */

#include "app_cmhand_power_stage_ctrl.h"

#include "app_power_stage_drive.h" //need this for power stage wrapper


//================================================= STATIC MEMBER INITIALIZATION =================================================

//initialze as empty at the start; expect to populate this before any commands get processed
std::span<Power_Stage_Subsystem*, std::dynamic_extent> Power_Stage_Command_Handlers::stages{};

//======================================================== GETTER AND SETTER METHODS/UTILITIES ===================================================

//return some kinda stl-compatible container
//that contains all the request or command handlers defined in this class
std::span<const Parser::command_mapping_t, std::dynamic_extent> Power_Stage_Command_Handlers::command_handlers() {
	return Power_Stage_Command_Handlers::COMMAND_HANDLERS;
}

//pass stl container of instantiated power stage systems
void Power_Stage_Command_Handlers::attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages) {
	//just copy over the span passed into the member variable
	stages = _stages;
}

//======================================================== THE ACTUAL COMMAND HANDLERS ===================================================

/*
 * Will either have 1 or 2 bytes
 * If 1 byte:
 * 	disable channel 0
 * If 2 bytes:
 * 	disable channel `rx_packet[1]`
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::disable_stage(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																						std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != 1 && rx_payload.size() != 2) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::STAGE_DISABLE) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the particular channel we want to disable
	size_t channel = 0;
	if(rx_payload.size() == 2) channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//now try to execute the disable command
	if(!stages[channel]->set_mode(Power_Stage_Subsystem::Stage_Mode::DISABLED)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::STAGE_DISABLE;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * Will either have 1 or 2 bytes
 * If 1 byte:
 * 	enable channel 0 in current regulator operation
 * If 2 bytes:
 * 	enable channel `rx_packet[1]` in current regulator operation
 *
 * 	Will start the channel regulating the output to zero current
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::enable_stage_regulator(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																								std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != 1 && rx_payload.size() != 2) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::STAGE_ENABLE_REGULATOR) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the particular channel we want to disable
	size_t channel = 0;
	if(rx_payload.size() == 2) channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//now try to execute the enable_auto command
	if(!stages[channel]->set_mode(Power_Stage_Subsystem::Stage_Mode::ENABLED_AUTO)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::STAGE_ENABLE_REGULATOR;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * Will either have (1 + confirmation_message_size) or (2 + confirmation_message_size) bytes
 * If 1 + confirmation_message_size bytes:
 * 	enable channel 0 in manual control operation
 * If 2 + confirmation_message_size bytes:
 * 	enable channel `rx_packet[1]` in manual control operation
 *
 *
 * 	Will start the channel with both power stages set to zero PWM value
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::enable_stage_manual(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != (1 + man_confirm_message.size()) && rx_payload.size() != (2 + man_confirm_message.size())) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::STAGE_ENABLE_MANUAL) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the particular channel we want to disable and the confirmation message bytes
	std::span<uint8_t, std::dynamic_extent> confirm_bytes;
	size_t channel = 0;
	if(rx_payload.size() == (2 + man_confirm_message.size())) {
		channel = rx_payload[1];
		confirm_bytes = rx_payload.subspan(2); //grab the rest of the bytes
	}
	else
		confirm_bytes = rx_payload.subspan(1); //channel = 0, grab the rest of the bytes

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//check if the confirmation message (does not) matches the expected confirmation
	if(!std::equal(	confirm_bytes.begin(), confirm_bytes.end(),
					Power_Stage_Command_Handlers::man_confirm_message.begin(), Power_Stage_Command_Handlers::man_confirm_message.end()))
	{
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//now try to execute the enable_manual command
	if(!stages[channel]->set_mode(Power_Stage_Subsystem::Stage_Mode::ENABLED_MANUAL)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::STAGE_ENABLE_MANUAL;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * Will either have 1 or 2 bytes
 * If 1 byte:
 * 	autotune controller based on shim coil attached to channel 0
 * If 2 bytes:
 * 	autotune controller based on shim coil attached to channel `rx_packet[1]`
 *
 * 	Will immediately start autotuning if operation is successful
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::enable_stage_autotune(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																								std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != (1 + tune_confirm_message.size()) && rx_payload.size() != (2 + tune_confirm_message.size())) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::STAGE_ENABLE_AUTOTUNING) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the particular channel we want to disable and the confirmation message bytes
	std::span<uint8_t, std::dynamic_extent> confirm_bytes;
	size_t channel = 0;
	if(rx_payload.size() == (2 + tune_confirm_message.size())) {
		channel = rx_payload[1];
		confirm_bytes = rx_payload.subspan(2);
	}
	else
		confirm_bytes = rx_payload.subspan(1);

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//check if the confirmation message (does not) matches the expected confirmation
	if(!std::equal(	confirm_bytes.begin(), confirm_bytes.end(),
					Power_Stage_Command_Handlers::tune_confirm_message.begin(), Power_Stage_Command_Handlers::tune_confirm_message.end()))
	{
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//now try to execute the enable autotuning command
	if(!stages[channel]->set_mode(Power_Stage_Subsystem::Stage_Mode::ENABLED_AUTOTUNING)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::STAGE_ENABLE_AUTOTUNING;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

//========================= MANUAL CONTROL FUNCTIONS =========================

/*
 * Will either have 1 or 2 bytes
 * If 1 byte:
 * 	turn off power stage channel 0
 * If 2 bytes:
 * 	turn off power stage channel `rx_packet[1]`
 *
 * 	Will immediately start autotuning if operation is successful
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::stage_manual_off(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != 1 && rx_payload.size() != 2) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::STAGE_MANUAL_DRIVE_OFF) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the particular channel we want to disable
	size_t channel = 0;
	if(rx_payload.size() == 2) channel = rx_payload[1];

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the low level power stage control instance from the higher level instance
	Power_Stage_Wrapper& stage = stages[channel]->get_direct_stage_control_instance();

	//check if it's (not) safe to manually control the stage
	if(stage.GET_LOCKED_OUT()) {
		tx_payload[0] = Parser::NACK_ERROR_SYSTEM_BUSY;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if it's safe, shut the power stage down
	//and check if doing so was successful
	if(!stage.set_drive_halves(0, 0)) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//respond with an ACK if turning power stage off was successful
	tx_payload[0] = CM_Mapping::STAGE_MANUAL_DRIVE_OFF;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * Will either have 5 or 6 bytes
 * last 4 bytes will always be the float value for which to drive the stage
 * If 5 byte:
 * 	turn off power stage channel 0
 * If 6 bytes:
 * 	turn off power stage channel `rx_packet[1]`
 *
 * 	Will immediately start autotuning if operation is successful
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::stage_manual_drive(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != 5 && rx_payload.size() != 6) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::STAGE_MANUAL_SET_DRIVE) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the particular channel we want to disable
	std::span<uint8_t, std::dynamic_extent> drive_bytes; //have a place to dump the bytes for our drive value
	size_t channel = 0;
	if(rx_payload.size() == 6) {
		channel = rx_payload[1];
		drive_bytes = rx_payload.subspan(2, 4); //grab bytes starting at [2]
	}
	else
		drive_bytes = rx_payload.subspan(1, 4); //grab bytes starting at [1] (channel is 0)

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the low level power stage control instance from the higher level instance
	Power_Stage_Wrapper& stage = stages[channel]->get_direct_stage_control_instance();

	//check if it's (not) safe to manually control the stage
	if(stage.GET_LOCKED_OUT()) {
		tx_payload[0] = Parser::NACK_ERROR_SYSTEM_BUSY;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if it's safe, grab the value we'd like to drive the stage with
	//grab the float that was sent over starting at index 1
	float drive_value = unpack_float(drive_bytes);

	//and write the desired drive value out to the stage
	//and check if doing so was successful
	if(!stage.set_drive(drive_value)) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED; //could be due to out of range or some other issue with the power stage
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//respond with an ACK if driving the power stage was successful
	tx_payload[0] = CM_Mapping::STAGE_MANUAL_SET_DRIVE;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * Will either have 9 or 10 bytes
 * last 8 bytes will always be the float value for which to drive the half-bridges
 * float starting at index [1 or 2] --> positive value of half bridge drive
 * float starting at index [5 or 6] --> positive value of half bridge drive
 *
 * If 9 byte:
 * 	turn off power stage channel 0
 * If 10 bytes:
 * 	turn off power stage channel `rx_packet[1]`
 *
 * 	Will immediately start autotuning if operation is successful
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::stage_manual_duties(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != 9 && rx_payload.size() != 10) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::STAGE_MANUAL_SET_DUTIES) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the particular channel we want to disable
	std::span<uint8_t, std::dynamic_extent> pos_drive_bytes; //have a place to dump the bytes for our positive half-bridge drive value
	std::span<uint8_t, std::dynamic_extent> neg_drive_bytes; //have a place to dump the bytes for our negative half-bridge drive value
	size_t channel = 0;
	if(rx_payload.size() == 10) { //have a channel encoded in our payload
		channel = rx_payload[1];
		pos_drive_bytes = rx_payload.subspan(2, 4); //grab bytes starting at [2]
		neg_drive_bytes = rx_payload.subspan(6, 4); //grab bytes starting at [6]
	}
	else {
		pos_drive_bytes = rx_payload.subspan(1, 4); //grab bytes starting at [1] (channel is 0)
		neg_drive_bytes = rx_payload.subspan(5, 4); //grab bytes starting at [5]
	}

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the low level power stage control instance from the higher level instance
	Power_Stage_Wrapper& stage = stages[channel]->get_direct_stage_control_instance();

	//check if it's (not) safe to manually control the stage
	if(stage.GET_LOCKED_OUT()) {
		tx_payload[0] = Parser::NACK_ERROR_SYSTEM_BUSY;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if it's safe, grab the values we'd like to drive our half bridges with
	//grab the float that was sent over starting at index 1
	float pos_drive_value = unpack_float(pos_drive_bytes);
	float neg_drive_value = unpack_float(neg_drive_bytes);

	//and write the desired drive value out to the stage
	//and check if doing so was successful
	if(!stage.set_drive_halves(pos_drive_value, neg_drive_value)) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED; //could be due to out of range or some other issue with the power stage
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//respond with an ACK if driving the power stage was successful
	tx_payload[0] = CM_Mapping::STAGE_MANUAL_SET_DUTIES;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

//========================= SET SWITCHING FREQUENCY FOR ALL CHANNELS =========================

/*
 * Have 5 bytes total
 * [0] 		- CommandID
 * [1-4]	- Float for switching frequency (in Hz)
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::stage_set_fsw(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																						std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != 5) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::STAGE_SET_FSW) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the float that was sent over starting at index 1
	float fsw_hz = unpack_float(rx_payload.subspan(1, 4));

	//now try to set switching frequency
	//execution can fail if switching frequency out of range OR power stage not disabled
	//it's gonna be more effort than its worth to differentiate between the two, so return a more generic error
	if(!Power_Stage_Subsystem::set_switching_frequency(fsw_hz)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::STAGE_SET_FSW;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

