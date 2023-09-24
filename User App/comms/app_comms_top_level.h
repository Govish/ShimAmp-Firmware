/*
 * app_comms_top_level.h
 *
 *  Created on: Sep 20, 2023
 *      Author: Ishaan
 *
 *  Top-level file that runs the communications and execution subsystem of the device
 *  Basically just have a singleton class with an `init()` and a `loop()`
 *
 *  This class will own all the statically allocated data structures and class instances
 *  Configuration changes to each of the comms systems must take place here
 *
 *	TODO: include all files that define command and request handlers
 *
 */

#ifndef COMMS_APP_COMMS_TOP_LEVEL_H_
#define COMMS_APP_COMMS_TOP_LEVEL_H_

extern "C" {
	#include "stm32g474xx.h" //for some primitive types
}

//c++ stl containers and stuff relevant to them
#include <array>
#include <span>
#include <utility>
#include <algorithm>

//include libraries for all the submodules within the comms subsystem
#include "app_hal_uart.h"
#include "app_comms_cobs.h"
#include "app_comms_crc.h"
#include "app_comms_parser.h"

class Comms_Exec_Subsystem {

public:
	//since we want to implement as a singleton, delete the copy constructor and assignment operator
	//https://stackoverflow.com/questions/1008019/how-do-you-implement-the-singleton-design-pattern
	Comms_Exec_Subsystem(Comms_Exec_Subsystem const&) = delete;
	void operator=(Comms_Exec_Subsystem const&) = delete;

	//function to access the singleton
	static Comms_Exec_Subsystem& get_instance() {
		static Comms_Exec_Subsystem sys;
		return sys;
	}

	//read in the serial device address and initialize the parser with it
	void init(uint8_t device_address);
	void loop();
private:
	//##### all these objects will be initialized in the constructor of `Comms_Subsystem` #####

	//============================= EVERYTHING SERIAL COMMUNICATION ==================================
	std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> serial_tx_buffer; //place for UART to put outgoing serial data
	std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> serial_rx_buffer; //place for UART to put incoming serial data
	UART serial_comms;

	//=========================== EVERYTHING COBS ENCODING ===========================
	std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> rx_encoded_packet; //place to keep packet received over serial comms
	std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> tx_encoded_packet; //place to keep packet to be sent over serial comms
	Cobs cobs; //instantiate a cobs instance; nothing to construct really

	//=========================== EVERYTHING CRC COMPUTATION ==============================
	//CRC-16/AUG-CCITT, common 16-bit CRC parameters
	const uint16_t crc_poly = 0x1021;
	const uint16_t crc_seed = 0x1D0F;
	const uint16_t crc_xor_out = 0x0000;
	Comms_CRC crc;

	//============================= EVERYTHING PARSER ===========================
	std::array<uint8_t, Cobs::MSG_MAX_UNENCODED_LENGTH> rx_decoded_packet; //place to put the received packet after decoding
	std::array<uint8_t, Cobs::MSG_MAX_UNENCODED_LENGTH> tx_unencoded_packet; //place to put the response packet to be transmitted back to host (before encoding)
	Parser parser;

	//=============================== PRIVATE CONSTRUCTOR FOR SINGLETON ===============================
	Comms_Exec_Subsystem();

};


#endif /* COMMS_APP_COMMS_TOP_LEVEL_H_ */
