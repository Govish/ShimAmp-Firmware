/*
 * app_comms_top_level.cpp
 *
 *  Created on: Sep 20, 2023
 *      Author: Ishaan
 */

#include "app_comms_top_level.h"
#include "app_utils.h" //for span indexing utils

//================ REQUEST HANDLER INCLUDES ==============
#include "app_rqhand_test.h"
#include "app_rqhand_power_stage_status.h"
#include "app_rqhand_setpoint.h"
#include "app_rqhand_control.h"
#include "app_rqhand_sampler.h"

//================ COMMAND HANDLER INCLUDES ==============
#include "app_cmhand_test.h"
#include "app_cmhand_power_stage_ctrl.h"
#include "app_cmhand_setpoint.h"
#include "app_cmhand_control.h"
#include "app_cmhand_sampler.h"


//================================= DEFINING STANDARD CONFIGURATION ==============================

Comms_Exec_Subsystem::Configuration_Details Comms_Exec_Subsystem::COMMS_CHANNEL_0 = {
		//run the main communication system off of LPUART
		.uart_channel = UART::LPUART,
};

//======================================= PUBLIC METHODS =====================================

//Constructor
Comms_Exec_Subsystem::Comms_Exec_Subsystem(Configuration_Details& config_details):
		serial_comms(config_details.uart_channel, Cobs::CHAR_START_OF_FRAME, Cobs::CHAR_END_OF_FRAME, serial_tx_buffer, serial_rx_buffer),
		cobs(),
		crc(), //use default CRC parameters (CRC-16/AUG-CCITT)
		parser(crc)
{}

//call this in `app_init()`
void Comms_Exec_Subsystem::init(uint8_t device_address) {
	//initialize the serial communications
	serial_comms.init();
	//TODO: attach any serial error handlers

	//forward the device address to the parser
	parser.set_address(device_address);

	//TODO: register all command and request callbacks here
	for(auto& [rq_code, rq_callback] : Test_Request_Handlers::request_handlers()) parser.attach_request_cb(rq_code, rq_callback);
	for(auto& [rq_code, rq_callback] : Power_Stage_Request_Handlers::request_handlers()) parser.attach_request_cb(rq_code, rq_callback);
	for(auto& [rq_code, rq_callback] : Setpoint_Request_Handlers::request_handlers()) parser.attach_request_cb(rq_code, rq_callback);
	for(auto& [rq_code, rq_callback] : Controller_Request_Handlers::request_handlers()) parser.attach_request_cb(rq_code, rq_callback);
	for(auto& [rq_code, rq_callback] : Sampler_Request_Handlers::request_handlers()) parser.attach_request_cb(rq_code, rq_callback);

	for(auto& [cm_code, cm_callback] : Test_Command_Handlers::command_handlers()) parser.attach_command_cb(cm_code, cm_callback);
	for(auto& [cm_code, cm_callback] : Power_Stage_Command_Handlers::command_handlers()) parser.attach_command_cb(cm_code, cm_callback);
	for(auto& [cm_code, cm_callback] : Setpoint_Command_Handlers::command_handlers()) parser.attach_command_cb(cm_code, cm_callback);
	for(auto& [cm_code, cm_callback] : Controller_Command_Handlers::command_handlers()) parser.attach_command_cb(cm_code, cm_callback);
	for(auto& [cm_code, cm_callback] : Sampler_Command_Handlers::command_handlers()) parser.attach_command_cb(cm_code, cm_callback);

}

//call this in `app_loop()`
//NOTE: CODE MAY BLOCK IF ANY OF THE COMMAND OR REQUEST HANDLERS BLOCK
void Comms_Exec_Subsystem::loop() {
	/*
	 * Implement the simplest of state machines --> two different states to allow for non-blocking transmit
	 * if IS_WAITING_RECEIVE is true, we're waiting for some packet to be received over UART
	 * if IS_WAITING_RECEIVE is false, we're waiting for the transmitter to finish transmitting its previous packet; can only transmit once transmit is complete
	 */

	static bool IS_WAITING_RECEIVE = true; //remember this for successive function calls
	static int16_t tx_encoded_packet_length; //need to save this across the two states; save between successive function calls too

	if(IS_WAITING_RECEIVE) {
		//check if we have a packet
		size_t rx_encoded_packet_length = serial_comms.get_packet(rx_encoded_packet);
		if(!rx_encoded_packet_length) return; //if we don't have a packet, exit the function

		//we have a packet - decode it
		//TODO: handle COBS decoding error maybe
		int16_t decoded_packet_length = cobs.decode(spn(rx_encoded_packet, rx_encoded_packet_length), rx_decoded_packet);
		if(decoded_packet_length < 0) return; //ran into an error decoding the packet, don't continue

		//if the cobs decode was successful
		//parse the decoded packet, execute the corresponding command or request (if applicable) and respond as necessary
		size_t response_packet_length = parser.parse_buffer(spn(rx_decoded_packet, (size_t)decoded_packet_length), tx_unencoded_packet);
		if(!response_packet_length) return; //if we don't need to respond with anything, then just return

		//encode the response packet
		//TODO: handle COBS encoding error maybe
		tx_encoded_packet_length = cobs.encode(spn(tx_unencoded_packet, response_packet_length), tx_encoded_packet);
		if(tx_encoded_packet_length < 0) return; //ran into an error encoding the packet, don't continue
		IS_WAITING_RECEIVE = false; //waiting to transmit
	}

	//splitting the `false` case into a separate conditional so we can try the transmit in the same function call
	//allows us to receive and dispatch a message in a single call if the transmit isn't blocking
	if(!IS_WAITING_RECEIVE) {
		if(!serial_comms.ready_to_send()) return; //if we're busy transmitting something, just return

		//if we're able to transmit, send the encoded packet
		serial_comms.transmit(spn(tx_encoded_packet, tx_encoded_packet_length));
		IS_WAITING_RECEIVE = true; //go back to waiting for a packet
	}
}

