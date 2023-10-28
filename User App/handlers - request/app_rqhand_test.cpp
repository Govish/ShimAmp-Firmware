/*
 * app_rqhand_test.cpp
 *
 *  Created on: Sep 20, 2023
 *      Author: Ishaan
 */

#include "app_rqhand_test.h"
#include "app_utils.h" //for easy casting to span

#include <span> //for span

//=========================================== Request Handler Definitions =================================================

std::pair<Parser::MessageType_t, size_t> Test_Request_Handlers::test_byte(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 2, 1, RQ_Mapping::TEST_BYTE, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//everything's kosher
	tx_payload[0] = RQ_Mapping::TEST_BYTE; //this is the request we serviced
	tx_payload[1] = the_test_byte; //load the test byte into the transmit payload
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 2); //and return an ack message along with a two-byte payload
}

std::pair<Parser::MessageType_t, size_t> Test_Request_Handlers::test_uint32(const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 5, 1, RQ_Mapping::TEST_UINT32, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//everything's kosher
	tx_payload[0] = RQ_Mapping::TEST_UINT32; //this is the request we serviced
	pack(the_test_uint32, tx_payload.subspan(1, 4)); //put the uint32_t starting at index 1, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 5); //and return an ack message along with a five-byte payload
}

std::pair<Parser::MessageType_t, size_t> Test_Request_Handlers::test_int32(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 5, 1, RQ_Mapping::TEST_INT32, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//everything's kosher
	tx_payload[0] = RQ_Mapping::TEST_INT32; //this is the request we serviced
	pack(the_test_int32, tx_payload.subspan(1, 4)); //put the int32 starting at index 1, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 5); //and return an ack message along with a five-byte payload
}

std::pair<Parser::MessageType_t, size_t> Test_Request_Handlers::test_float(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, 5, 1, RQ_Mapping::TEST_FLOAT, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//everything's kosher
	tx_payload[0] = RQ_Mapping::TEST_FLOAT; //this is the request we serviced
	pack(the_test_float, tx_payload.subspan(1, 4)); //put the float starting at index 1, taking 4 bytes
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, 5); //and return an ack message along with a five-byte payload
}

std::pair<Parser::MessageType_t, size_t> Test_Request_Handlers::test_string(const std::span<uint8_t, std::dynamic_extent> rx_payload,
																			std::span<uint8_t, std::dynamic_extent> tx_payload)
{
	//sanity check the received request
	uint8_t tx_len;
	if(!RQ_Mapping::VALIDATE_REQUEST(tx_payload, rx_payload, the_test_string.size() + 1, 1, RQ_Mapping::TEST_STRING, tx_len))
		return std::make_pair(Parser::DEVICE_NACK_HOST_MESSAGE, tx_len);

	//everything's kosher
	tx_payload[0] = RQ_Mapping::TEST_STRING; //this is the request we serviced
	std::copy(the_test_string.begin(), the_test_string.end(), tx_payload.begin() + 1); //copy the test string into the tx payload starting at index 1
	return std::make_pair(Parser::DEVICE_RESPONSE_HOST_REQUEST, the_test_string.size() + 1); //and return an ack message along with a <size+1>-byte payload
}

//================================================== getter functions for container of handlers ===============================================

//automatic conversion to `span` happens which is kinda sick
std::span<const Parser::request_mapping_t, std::dynamic_extent> Test_Request_Handlers::request_handlers() {
	return Test_Request_Handlers::REQUEST_HANDLERS;
}
