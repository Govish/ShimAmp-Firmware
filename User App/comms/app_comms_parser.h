/*
 * app_comms_protocol.h
 *
 *  Created on: Sep 16, 2023
 *      Author: Ishaan
 *
 *  This is the subsystem that converts between packets and messages/commands from the rest of the firmware
 *  It will read packets, execute appropriate commands located in other parts of firmware, and create a response packet
 *
 *  A message packet is formatted as the following (proper specification doc to come):
 *
 *  Index	Abbreviation	Value Range		Description
 *   [0]		ID			0x0 - 0xFF		Node Address
 *   [1]		MTYPE		0x0 - 0x0F		Message type
 *   [2]		PLEN		0x1 - 0xF8		Message Payload Length
 *   [3]		PL0			0x0 - 0xFF		Payload byte 0, typically command or request code
 *   [4]		PL1			0x0 - 0xFF		Payload byte 1
 *    ...
 *   [n-2]		PLn			0x0 - 0xFF		Payload byte n
 *   [n-1]		CRCh		0x0 - 0xFF		CRC high byte
 *   [n]		CRCl		0x0	- 0xFF		CRC low byte
 *
 *
 *   Some notes on the specific bytes of the message:
 *   	- ID
 *   		... is the ID of the node; typically configured via dip switches on the board
 *   		as of now the procol can support 256 nodes
 *
 *   	- MTYPE
 *   		...top 5 bits of this message are RESERVED - will either expand this to more ID bits or other message types (or who knows that's why they're reserved i guess)
 *   		Message types are as follows:
 *   			0x0 --> HOST_COMMAND_ALL_DEVICES: host writes this ADDRESSING ALL DEVICES ON BUS; useful for ARMing all amplifiers on the bus
 *   				\--> NO DEVICES WILL ACK OR NACK THIS MESSAGE!
 *   				\--> formatted just like any other command message
 *   			0x1 --> HOST_COMMAND_TO_DEVICE: host writes this to write parameters to the device; payload contains the particular command and the parameter values associated with the command
 *   			0x2 --> HOST_REQUEST_FROM_DEVICE: host writes this to read parameters to device; payload contains a code corresponding to a value the host wants to read
 *   			0x3 --> reserved
 *   			0x4 --> DEVICE_NACK_HOST_MESSAGE: node responds with this message type when there was some issue with the previous packet (payload will contain message descrption)
 *   			0x5 --> DEVICE_ACK_HOST_COMMAND: node responds with this message when host command successfully received/written; payload mirrors payload written to device
 *   			0x6 --> DEVICE_RESPONSE_HOST_REQUEST:node responds with this message with the data host requested; data delivered in message payload
 *   			0x7 --> reserved
 *
 *   	- PLEN
 *   		...payload length of the particular message packet--all messages have a payload length between 1-248 bytes
 *   			--> 248 computed by subtracting PROTOCOL_OVERHEAD from MAX_UNENCODED_LENGTH
 *   		if payload length out of bounds, this causes device to NACK with a corresponding error message
 *
 *   	- PLx
 *   		...payload bits of the message
 *   		for NACK messages, here are some following payload codes
 *				- 0x0	-->	unknown error
 *   			- 0x01	--> invalid CRC
 *   			- 0x02	--> invalid message size
 *   			- 0x03	--> unknown command code
 *   			- 0x04	--> unknown request code
 *   			- 0x05	--> command value out of range
 *   			- 0x06	--> command execution failed
 *   			- 0x07 	--> system busy
 *
 *   	- CRCh & CRCl
 *   		... are the high and low bytes of CRC-16/AUG-CCITT, a common 16-bit CRC, applied to the message
 *   		the CRC is computed least significant byte to most significant byte, from least significant bit to most significant bit
 *   		crc polynomial: 0x1021
 *   		crc seed: 0x1D0F
 *   		crc xor_out: 0x0000 (i.e. don't need to xor the CRC result
 *
 *
 */

#ifndef COMMS_APP_COMMS_PARSER_H_
#define COMMS_APP_COMMS_PARSER_H_

#include <stddef.h> //for size_t
#include <span> //for stl interfaces
#include <utility> //for pair (for responses from command and request handlers

extern "C" {
	#include "stm32g474xx.h" //for uint8_t type
}


#include "app_comms_cobs.h" //for message lengths and such
#include "app_comms_crc.h" //so we can hang onto a CRC instance

class Parser {
public:

	//================================================= TYPEDEFS =================================================

	//enum types for different messages that can be exchanged
	typedef enum {
		HOST_COMMAND_ALL_DEVICES = 		(uint8_t)0x0,
		HOST_COMMAND_TO_DEVICE = 		(uint8_t)0x1,
		HOST_REQUEST_FROM_DEVICE = 		(uint8_t)0x2,
		DEVICE_NACK_HOST_MESSAGE = 		(uint8_t)0x4,
		DEVICE_ACK_HOST_MESSAGE = 		(uint8_t)0x5,
		DEVICE_RESPONSE_HOST_REQUEST =	(uint8_t)0x6,
	} MessageType_t;
	static constexpr uint8_t MESSAGE_TYPE_MASK = 0x07; //mask the MTYPE packet with this to look up the message type

	//enum type for not-acknowledge responses
	typedef enum {
		NACK_ERROR_UNKNOWN = 				(uint8_t)0x00,
		NACK_ERROR_INTERNAL_FW = 			(uint8_t)0x01,
		NACK_ERROR_INVALID_CRC = 			(uint8_t)0x02,
		NACK_ERROR_UNKNOWN_MSG_TYPE = 		(uint8_t)0x03,
		NACK_ERROR_INVALID_MSG_SIZE = 		(uint8_t)0x04,
		NACK_ERROR_UNKNOWN_COMMAND_CODE = 	(uint8_t)0x05,
		NACK_ERROR_UNKNOWN_REQUEST_CODE = 	(uint8_t)0x06,
		NACK_ERROR_COMMAND_OUT_OF_RANGE = 	(uint8_t)0x07,
		NACK_ERROR_COMMAND_EXEC_FAILED = 	(uint8_t)0x08,
		NACK_ERROR_SYSTEM_BUSY = 			(uint8_t)0x09,
	} NACKErrorTypes_t;

	static constexpr size_t ID_INDEX = 0;
	static constexpr size_t MTYPE_INDEX = 1;
	static constexpr size_t PLEN_INDEX = 2;
	static constexpr size_t PACKET_VITALS_OVERHEAD = 3; //these fields above occupy a total of 3 bytes

	static constexpr size_t PL_START_INDEX = 3;

	static constexpr size_t PACKET_OVERHEAD = PACKET_VITALS_OVERHEAD + 2; //bytes that aren't payload: ID, MTYPE, PLEN, CRCh, CRCl
	static constexpr size_t MAX_PAYLOAD_LENGTH = Cobs::MSG_MAX_UNENCODED_LENGTH - PACKET_OVERHEAD; //maximum length of a packet payload
	static constexpr size_t MIN_PAYLOAD_LENGTH = 1; //every packet has to have at least a single payload byte

	//for our current set of commands, we can handle this range of codes
	//useful for bounds checking the callback attachement functions
	static constexpr size_t COMMAND_CODE_MIN = 0;
	static constexpr size_t COMMAND_CODE_MAX = 0xFF;

	//for our current set of requests, we can handle this range of codes
	//useful for bounds-checking the callback attachement functions
	static constexpr size_t REQUEST_CODE_MIN = 0;
	static constexpr size_t REQUEST_CODE_MAX = 0xFF;

	//###################### TYPES FOR THE COMMAND/REQUEST HANDLING FUNCTIONS ####################

	//function signature of a command handler
	//takes as arguments:
		//--span peeping into the payload of the rx packet; truncated to the valid size
		//--span allowing access a section of the transmit buffer dedicated to the payload
	//reponds with a pair
		//first element is what kinda response to send
		//second element is the payload size
	//if there is an error in processing the command; the command handler is responsible for NACKing and putting a correct NACK reason in the payload
	//NACK types can be found by including this header file in any command handler
	typedef std::pair<MessageType_t, size_t> (*command_handler_t)(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																	std::span<uint8_t, std::dynamic_extent> tx_payload);

	//function signature of a request handler
	//as of now, is the same as a command_handler, but separating for future-proofing
	typedef std::pair<MessageType_t, size_t> (*request_handler_t)(	const std::span<uint8_t, std::dynamic_extent> rx_payload,
																	std::span<uint8_t, std::dynamic_extent> tx_payload);

	//==============================================================================================================

	Parser(Comms_CRC& _crc_comp);

	//set the device address on the multi-drop serial bus
	//can't be passed in via constructor since this is read from dip switches during boot
	void set_address(uint8_t address);

	//continuing the `std::span` interface used in other communication subsystems
	//provides a scalable, c++ style way to interface with the parser
	//returns how many bytes our response packet contains, if any
	int16_t parse_buffer(	const std::span<uint8_t, std::dynamic_extent> rx_packet,
									std::span<uint8_t, std::dynamic_extent> tx_packet);

	//attach command and request handlers to particular command and request codes
	void attach_command_cb(const size_t command_code, const command_handler_t command_handler);
	void attach_request_cb(const size_t request_code, const request_handler_t request_handler);

private:
	//know the address of the particular device on the
	size_t device_address = -1;

	//have access to a crc instance to compute and validate our CRCs
	//owned and initialized by a higher level class
	Comms_CRC crc_comp;

	//have an array of callback functions that map a command/request code to a firmware callback function
	//technically not the most memory efficiency way to map command/requests to firmware functions
	//but I think this is pretty elegant, and we have a decent amount of memory to access
	command_handler_t command_handler_map[COMMAND_CODE_MAX] = {NULL};
	request_handler_t request_handler_map[REQUEST_CODE_MAX] = {NULL};

};

#endif /* COMMS_APP_COMMS_PARSER_H_ */
