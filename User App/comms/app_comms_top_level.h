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
	//======================================================= CONFIGURATION DETAILS STRUCT =======================================================
	//following the paradigm of passing pre-instantiated configuration information to the constructor
	struct Configuration_Details {
		UART::UART_Hardware_Channel& uart_channel;
	};
	static Configuration_Details COMMS_CHANNEL_0; //our main source of configuration information

	//======================================================= PUBLIC METHODS =======================================================

	//constructor--just takes some general configuration details during instantiation
	Comms_Exec_Subsystem(Configuration_Details& config_details);

	//delete our copy constructor and asssignment operator to avoid any weird hardware issues
	Comms_Exec_Subsystem(Comms_Exec_Subsystem const&) = delete;
	void operator=(Comms_Exec_Subsystem const&) = delete;

	//read in the serial device address and initialize the parser with it
	void init(uint8_t device_address);
	void loop();
private:
	//##### all these objects will be initialized in the constructor of `Comms_Exec_Subsystem` #####

	//============================= EVERYTHING SERIAL COMMUNICATION ==================================
	std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> serial_tx_buffer; //place for UART to put outgoing serial data
	std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> serial_rx_buffer; //place for UART to put incoming serial data
	UART serial_comms;

	//=========================== EVERYTHING COBS ENCODING ===========================
	std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> rx_encoded_packet; //place to keep packet received over serial comms
	std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> tx_encoded_packet; //place to keep packet to be sent over serial comms
	Cobs cobs; //instantiate a cobs instance; nothing to construct really

	//=========================== EVERYTHING CRC COMPUTATION ==============================
	Comms_CRC crc;

	//============================= EVERYTHING PARSER ===========================
	std::array<uint8_t, Cobs::MSG_MAX_UNENCODED_LENGTH> rx_decoded_packet; //place to put the received packet after decoding
	std::array<uint8_t, Cobs::MSG_MAX_UNENCODED_LENGTH> tx_unencoded_packet; //place to put the response packet to be transmitted back to host (before encoding)
	Parser parser;

};


#endif /* COMMS_APP_COMMS_TOP_LEVEL_H_ */
