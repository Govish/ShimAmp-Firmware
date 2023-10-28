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
 * disable the channel provided in rx_payload[1]
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::disable_stage(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																						std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 2, CM_Mapping::STAGE_DISABLE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable
	size_t channel = rx_payload[1];

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
 * enable regulation on channel `rx_payload[1]`
 *
 * Will start the channel regulating the output to zero current
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::enable_stage_regulator(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																								std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 2, CM_Mapping::STAGE_ENABLE_REGULATOR, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable
	size_t channel = rx_payload[1];

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
 * Will have (2 + confirmation_message_size) bytes
 * enable manual control on channel `rx_payload[1]` and confirm manual control with confirmation message
 *
 * 	Will start the channel with both power stages set to zero PWM value
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::enable_stage_manual(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 2 + man_confirm_message.size(), CM_Mapping::STAGE_ENABLE_MANUAL, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable and the confirmation message bytes
	size_t channel = rx_payload[1];
	std::span<uint8_t, std::dynamic_extent> confirm_bytes = rx_payload.subspan(2); //grab the rest of the bytes

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
 * Autotune controller based on shim coil attached to channel `rx_packet[1]` and confirm with user
 *
 * Will immediately start autotuning if operation is successful
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::enable_stage_autotune(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																								std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 2 + tune_confirm_message.size(), CM_Mapping::STAGE_ENABLE_AUTOTUNING, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable and the confirmation message bytes
	size_t channel = rx_payload[1];
	std::span<uint8_t, std::dynamic_extent> confirm_bytes = rx_payload.subspan(2);

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
 * Turn off power stage channel `rx_packet[1]`
 *
 * 	Will immediately start autotuning if operation is successful
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::stage_manual_off(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 2, CM_Mapping::STAGE_MANUAL_DRIVE_OFF, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable
	size_t channel = rx_payload[1];

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
 *  drive power stage channel `rx_packet[1]` with value rx_packet[2:5]
 *
 * 	Will immediately start autotuning if operation is successful
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::stage_manual_drive(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 6, CM_Mapping::STAGE_MANUAL_SET_DRIVE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable and the commanded stage drive
	size_t channel = rx_payload[1];
	std::span<uint8_t, std::dynamic_extent> drive_bytes = rx_payload.subspan(2, 4); //grab bytes starting at [2]

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
 * drive channel `rx_packet[1]` with positive duty cycle `rx_packet[2:5]` and negative duty cycle `rx_packet[6:9]`
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::stage_manual_duties(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 10, CM_Mapping::STAGE_MANUAL_SET_DUTIES, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable and the drive bytes
	size_t channel = rx_payload[1];
	std::span<uint8_t, std::dynamic_extent>	pos_drive_bytes = rx_payload.subspan(2, 4); //grab bytes starting at [2]
	std::span<uint8_t, std::dynamic_extent>	neg_drive_bytes = rx_payload.subspan(6, 4); //grab bytes starting at [6]

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
 * set the switching frequency (in Hz) to `rx_packet[1:4]`
 */
std::pair<Parser::MessageType_t, size_t> Power_Stage_Command_Handlers::stage_set_fsw(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																						std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 5, CM_Mapping::STAGE_SET_FSW, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

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

