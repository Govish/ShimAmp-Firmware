/*
 * app_cmhand_control.cpp
 *
 *  Created on: Oct 26, 2023
 *      Author: Ishaan
 */


#include "app_cmhand_control.h"

#include "app_control_regulator.h" //need this for regulator wrapper
#include "app_utils.h" //for unpacking functions


//================================================= STATIC MEMBER INITIALIZATION =================================================

//initialze as empty at the start; expect to populate this before any commands get processed
std::span<Power_Stage_Subsystem*, std::dynamic_extent> Controller_Command_Handlers::stages{};

//======================================================== GETTER AND SETTER METHODS/UTILITIES ===================================================

//return some kinda stl-compatible container
//that contains all the request or command handlers defined in this class
std::span<const Parser::command_mapping_t, std::dynamic_extent> Controller_Command_Handlers::command_handlers() {
	return Controller_Command_Handlers::COMMAND_HANDLERS;
}

//pass stl container of instantiated power stage systems
void Controller_Command_Handlers::attach_power_stage_systems(std::span<Power_Stage_Subsystem*, std::dynamic_extent> _stages) {
	//just copy over the span passed into the member variable
	stages = _stages;
}

//======================================================== THE ACTUAL COMMAND HANDLERS ===================================================

/*
 * set the controller update rate (and ADC sampling frequency) for ALL channels
 */
std::pair<Parser::MessageType_t, size_t> Controller_Command_Handlers::set_controller_rate(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 5, CM_Mapping::CONTROL_SET_FREQUENCY, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the desired controller frequency by grabbing the float that was sent over starting at index 1
	float fc_hz = unpack_float(rx_payload.subspan(1, 4));

	if(!Power_Stage_Subsystem::set_controller_frequency(fc_hz)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::CONTROL_SET_FREQUENCY;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * set the controller DC gain on channel `rx_payload[1]` to `rx_payload[2:5]`
 */
std::pair<Parser::MessageType_t, size_t> Controller_Command_Handlers::set_controller_gain(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 6, CM_Mapping::CONTROL_SET_DC_GAIN, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to update
	size_t channel = rx_payload[1];
	float dc_gain = unpack_float(rx_payload.subspan(2, 4));

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the regulator that corresponds to the particular channel
	Regulator_Wrapper& regulator = stages[channel]->get_regulator_instance();

	//now try to execute the enable_auto command
	if(!regulator.update_gain(dc_gain)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::CONTROL_SET_DC_GAIN;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * set the controller crossover frequency on channel `rx_payload[1]` to `rx_payload[2:5]` Hz
 */
std::pair<Parser::MessageType_t, size_t> Controller_Command_Handlers::set_controller_crossover(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																								std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 6, CM_Mapping::CONTROL_SET_CROSSOVER, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to update
	size_t channel = rx_payload[1];
	float crossover = unpack_float(rx_payload.subspan(2, 4));

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the regulator that corresponds to the particular channel
	Regulator_Wrapper& regulator = stages[channel]->get_regulator_instance();

	//now try to execute the enable_auto command
	if(!regulator.update_crossover_freq(crossover)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::CONTROL_SET_CROSSOVER;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * set the resistance of the load on channel `rx_payload[1]` to `rx_payload[2:5]` ohms
 */
std::pair<Parser::MessageType_t, size_t> Controller_Command_Handlers::set_load_resistance(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 6, CM_Mapping::LOAD_SET_DC_RESISTANCE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable
	size_t channel = rx_payload[1];
	float res = unpack_float(rx_payload.subspan(2, 4));

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the regulator that corresponds to the particular channel
	Regulator_Wrapper& regulator = stages[channel]->get_regulator_instance();

	//now try to execute the enable_auto command
	if(!regulator.update_load_resistance(res)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::LOAD_SET_DC_RESISTANCE;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

/*
 * set the natural frequency (r/2*pi*l) of the load on channel `rx_payload[1]` to `rx_payload[2:5]` Hz
 */
std::pair<Parser::MessageType_t, size_t> Controller_Command_Handlers::set_load_natural_freq(const std::span<uint8_t, std::dynamic_extent> rx_payload,
																							std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received command
	uint8_t tx_len;
	if(!CM_Mapping::VALIDATE_COMMAND(tx_payload, rx_payload, 1, 6, CM_Mapping::LOAD_SET_NATURAL_FREQ, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//grab the particular channel we want to disable
	size_t channel = rx_payload[1];
	float nat_freq = unpack_float(rx_payload.subspan(2, 4));

	//check if we can index into the appropriate channel number
	if(channel >= stages.size()) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//grab the regulator that corresponds to the particular channel
	Regulator_Wrapper& regulator = stages[channel]->get_regulator_instance();

	//now try to execute the enable_auto command
	if(!regulator.update_load_natural_freq(nat_freq)) { //if execution of this fails
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_EXEC_FAILED;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//execution succeeds
	//respond with ACK, payload contains the particular command ID as the payload
	tx_payload[0] = CM_Mapping::LOAD_SET_NATURAL_FREQ;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

