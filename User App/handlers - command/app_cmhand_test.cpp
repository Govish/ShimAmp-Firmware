/*
 * app_cmhand_test.cpp
 *
 *  Created on: Oct 1, 2023
 *      Author: Ishaan
 */

#include "app_cmhand_test.h"

#include <span> //for span
#include "app_rqhand_test.h" // to access private member variables of this class


//=========================================== Request Handler Definitions =================================================

std::pair<Parser::MessageType_t, size_t> Test_Command_Handlers::test_byte(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != 2) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct command code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::TEST_BYTE) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if we didn't receive the correct byte
	//return a command out of range error message
	if(rx_payload[1] != Test_Request_Handlers::the_test_byte) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if everything's kosher, respond with an ACK
	//with the particular command ID as the payload
	tx_payload[0] = CM_Mapping::TEST_BYTE;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

std::pair<Parser::MessageType_t, size_t> Test_Command_Handlers::test_uint32(const std::span<uint8_t, std::dynamic_extent> rx_payload,
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

	//sanity check that we were redirected from the correct request code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::TEST_UINT32) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//unpack the received uint32 and compare it to the one in our books
	//return a command out of range error message if it's incorrect
	uint32_t rx_uint32 = unpack_uint32(rx_payload.subspan(1, 4));
	if(rx_uint32 != Test_Request_Handlers::the_test_uint32) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if everything's kosher, respond with an ACK
	//with the particular command ID as the payload
	tx_payload[0] = CM_Mapping::TEST_UINT32;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

std::pair<Parser::MessageType_t, size_t> Test_Command_Handlers::test_int32(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
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

	//sanity check that we were redirected from the correct request code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::TEST_INT32) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//unpack the received int32 and compare it to the one in our books
	//return a command out of range error message if it's incorrect
	int32_t rx_int32 = unpack_int32(rx_payload.subspan(1, 4));
	if(rx_int32 != Test_Request_Handlers::the_test_int32) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if everything's kosher, respond with an ACK
	//with the particular command ID as the payload
	tx_payload[0] = CM_Mapping::TEST_INT32;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

std::pair<Parser::MessageType_t, size_t> Test_Command_Handlers::test_float(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
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

	//sanity check that we were redirected from the correct request code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::TEST_FLOAT) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//unpack the received float and compare it to the one in our books
	//return a command out of range error message if it's incorrect
	float rx_float = unpack_float(rx_payload.subspan(1, 4));
	if(rx_float != Test_Request_Handlers::the_test_float) {
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if everything's kosher, respond with an ACK
	//with the particular command ID as the payload
	tx_payload[0] = CM_Mapping::TEST_FLOAT;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

std::pair<Parser::MessageType_t, size_t> Test_Command_Handlers::test_string(const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check that we have space in our transmit payload buffer
	//return a firmware error code if that doesn't match
	if(tx_payload.size() < 1) {
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 0); //don't have anywhere to put the TX message
	}

	//ensure that our payload is the correct size, first
	//if not, return a payload size error message
	if(rx_payload.size() != 1 + Test_Request_Handlers::the_test_string.size()) {
		tx_payload[0] = Parser::NACK_ERROR_INVALID_MSG_SIZE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//sanity check that we were redirected from the correct request code
	//return a firmware error code if that doesn't match
	if(rx_payload[0] != CM_Mapping::TEST_STRING) {
		tx_payload[0] = Parser::NACK_ERROR_INTERNAL_FW;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//do the string comparison using std::equal
	//return a command out of range error message if the strings aren't equal
	if(!std::equal(	rx_payload.begin() + 1, rx_payload.end(),
					Test_Request_Handlers::the_test_string.begin(), Test_Request_Handlers::the_test_string.end()))
	{
		tx_payload[0] = Parser::NACK_ERROR_COMMAND_OUT_OF_RANGE;
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, 1);
	}

	//if everything's kosher, respond with an ACK
	//with the particular command ID as the payload
	tx_payload[0] = CM_Mapping::TEST_STRING;
	return std::make_pair(Parser::DEVICE_ACK_HOST_MESSAGE, 1); //and return an ack message along with the single-byte payload
}

//================================================== getter functions for container of handlers ===============================================

//automatic conversion to `span` happens which is kinda sick
std::span<const Parser::command_mapping_t, std::dynamic_extent> Test_Command_Handlers::command_handlers() {
	return Test_Command_Handlers::COMMAND_HANDLERS;
}

