/*
 * app_comms_protocol.cpp
 *
 *  Created on: Sep 16, 2023
 *      Author: Ishaan
 */

#include "app_comms_parser.h"

#include <tuple> //for `std::tie`

//simple constructor that just hangs onto our CRC calculation unit
Parser::Parser(Comms_CRC& _crc_comp): crc_comp(_crc_comp) {}

size_t Parser::parse_buffer(	const std::span<uint8_t, std::dynamic_extent> rx_packet,
								std::span<uint8_t, std::dynamic_extent> tx_packet)
{
	//sanity check that we can even pack a failure message into the tx_buffer
	if(tx_packet.size() < PACKET_OVERHEAD + 1) return 0;

	//grab the core details of the packet
	uint8_t dest_id = rx_packet[ID_INDEX];
	uint8_t message_code = rx_packet[MTYPE_INDEX] & MESSAGE_TYPE_MASK; //keeping this a uint8_t for now
	size_t plen = rx_packet[PLEN_INDEX];

	//check if the crc passes the vibe check
	bool crc_good = crc_comp.validate_crc(rx_packet);

	//if the message ID doesn't match and it's not an ALL_DEVICES command
	//just return since we don't need to process or respond to this message
	if(dest_id != (uint8_t)device_address && message_code != (uint8_t)HOST_COMMAND_ALL_DEVICES)
		return 0;

	//if we're here, we have received a message we need to act on
	MessageType_t response_type;
	size_t response_plen;

	//if the CRC is bad
	if(!crc_good) {
		response_type = DEVICE_NACK_HOST_MESSAGE;
		response_plen = 1;
		tx_packet[PL_START_INDEX] = (uint8_t)NACK_ERROR_INVALID_CRC;
	}

	//if our payload size is outta bounds
	else if(plen < MIN_PAYLOAD_LENGTH || plen > MAX_PAYLOAD_LENGTH) {
		response_type = DEVICE_NACK_HOST_MESSAGE;
		response_plen = 1;
		tx_packet[PL_START_INDEX] = (uint8_t)NACK_ERROR_INVALID_MSG_SIZE;
	}

	//otherwise, everything seems to be good
	else {
		//create some local variables to pass the command or request handlers
		std::span<uint8_t, std::dynamic_extent> rx_payload = rx_packet.subspan(PL_START_INDEX, plen);
		std::span<uint8_t, std::dynamic_extent> tx_payload = tx_packet.subspan(PL_START_INDEX, tx_packet.size() - PACKET_OVERHEAD);
		//use `response_type` defined above as a reponse type parameter
		//use `response_plen` defined above as a response size parameter
		uint8_t command_request_code = rx_payload[0]; //command or request code is found at the beginning of the payload

		//act slightly differently based on the possible message response types
		switch(message_code) {

		//handle an ALL_DEVICES and SINGLE_DEVICE message the same way
			case HOST_COMMAND_ALL_DEVICES:
			case HOST_COMMAND_TO_DEVICE:
				//if the command is mapped, run it and record the function responses
				if(command_handler_map[command_request_code] != nullptr) {
					std::tie(response_type, response_plen) = command_handler_map[command_request_code](rx_payload, tx_payload);
				}
				//if it wasn't mapped, respond with a NACK
				else {
					response_type = DEVICE_NACK_HOST_MESSAGE;
					response_plen = 1;
					tx_packet[PL_START_INDEX] = (uint8_t)NACK_ERROR_UNKNOWN_COMMAND_CODE;
				}

				//that's all we have to do here really
				break;

			//handle a request code
			case HOST_REQUEST_FROM_DEVICE:
				//if the request is mapped, run it and record the function responses
				if(request_handler_map[command_request_code] != nullptr) {
					std::tie(response_type, response_plen) = request_handler_map[command_request_code](rx_payload, tx_payload);
				}
				//if it wasn't mapped, respond with a NACK
				else {
					response_type = DEVICE_NACK_HOST_MESSAGE;
					response_plen = 1;
					tx_packet[PL_START_INDEX] = (uint8_t)NACK_ERROR_UNKNOWN_REQUEST_CODE;
				}

				//that's all we really need to do here
				break;

			//if we're here, we received an invalid message code
			//send a NACK back with the appropriate error code
			default:
				response_type = DEVICE_NACK_HOST_MESSAGE;
				response_plen = 1;
				tx_packet[PL_START_INDEX] = (uint8_t)NACK_ERROR_UNKNOWN_MSG_TYPE;
				break;
		}
	}

	/*TODO: clean up and sanity-check the command/request handler?*/

	//pack the "vitals" of the message appropriately
	tx_packet[ID_INDEX] = dest_id; //message is coming from this device address
	tx_packet[MTYPE_INDEX] = (uint8_t)response_type; //we're sending this kinda message
	tx_packet[PLEN_INDEX] = (uint8_t)response_plen; //and the payload contains this many bytes
	size_t response_length_with_prefix = response_plen + PACKET_PREFIX_OVERHEAD;

	//compute the CRC of the message accordingly
	uint16_t tx_crc = crc_comp.compute_crc(tx_packet.subspan(0, response_length_with_prefix));
	//put the bytes of the crc at the end of the message, high-byte first (Big Endian)
	tx_packet[response_length_with_prefix] = (uint8_t)(0xFF & (tx_crc >> 8));
	tx_packet[response_length_with_prefix + 1] = (uint8_t)(0xFF & (tx_crc));

	//return the length of our response; but don't respond in the case of ALL_DEVICES command
	if(message_code == (uint8_t)HOST_COMMAND_ALL_DEVICES) return 0;
	return (int16_t)response_plen + PACKET_VITALS_OVERHEAD;
}

//quick method to initialize the address of the device
//need to happen outside of the constructor
void Parser::set_address(uint8_t address) {
	device_address = (size_t)address;
}

//bounds check the command code
//and if that checks out, store that in the command handler map
void Parser::attach_command_cb(const size_t command_code, const command_handler_t command_handler) {
	if(command_code < Parser::COMMAND_CODE_MIN) return;
	if(command_code > Parser::COMMAND_CODE_MAX) return;
	//ASSERT(command_handler_map[command_code] == nullptr); //this would be useful to ensure all commands map to unique values
	Parser::command_handler_map[command_code] = command_handler;
}

//bounds check the request code
//and if that checks out, store that in the request handler map
void Parser::attach_request_cb(const size_t request_code, const request_handler_t request_handler) {
	if(request_code < Parser::REQUEST_CODE_MIN) return;
	if(request_code > Parser::REQUEST_CODE_MAX) return;
	//ASSERT(request_handler_map[request_code] == nullptr); //this would be useful to ensure all requests map to unique values
	Parser::request_handler_map[request_code] = request_handler;
}
