/*
 * app_utils_debug_print.cpp
 *
 *  Created on: Oct 16, 2023
 *      Author: Ishaan
 */


#include "app_utils_debug_print.h"
#include "app_utils.h" //for span casting functions

Debug_Print::Debug_Print(UART::UART_Hardware_Channel& hw):
	debug_serial_port(hw, '\n', '\r', txbuf, rxbuf)
{}

void Debug_Print::init() {
	debug_serial_port.init();
}

void Debug_Print::print(std::string text) {
	//just return if we can't fit our text into our transmit buffer
	if(text.size() > BUFFER_LENGTH) return;

	//copy the string into our intermediate buffer
	std::copy(text.begin(), text.end(), tx_conversion_buffer.begin());

	//transmit the bytes over UART
	debug_serial_port.transmit(spn(tx_conversion_buffer, text.size()));
}

bool Debug_Print::available() {
	return debug_serial_port.available();
}

std::string Debug_Print::read() {
	//poll the UART, waiting until data is available; then copy any data into the RX intermediate buffer
	size_t packet_size = 0;
	do {
		packet_size = debug_serial_port.get_packet(rx_intermediate_buffer);
	} while(packet_size <= 0);

	//build a string outta the received data (minus leading and trailing frame characters)
	return std::string(rx_intermediate_buffer.begin() + 1, rx_intermediate_buffer.begin() + packet_size - 1);
}


