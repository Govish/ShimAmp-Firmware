/*
 * app_main.cpp
 *
 *  Created on: Sep 12, 2023
 *      Author: Ishaan
 */

#include "app_main.h"

//c/c++ includes
#include <array>
#include <algorithm>

//HAL includes
#include "app_hal_dio.h"
#include "app_pin_mapping.h"
#include "app_hal_timing.h"
#include "app_hal_uart.h"

//comms includes
#include "app_comms_cobs.h"
#include "app_comms_crc.h"

DIO user_led(PinMap::status_led);
DIO user_button(PinMap::user_button);

//statically creating these buffers, passing them to the UART instance
std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> serial_rx_buffer;
std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> serial_tx_buffer;
UART serial_comms(	&UART::LPUART, '<', '>', serial_tx_buffer, serial_rx_buffer);

//place to dump the received data
std::array<uint8_t, Cobs::MSG_MAX_ENCODED_LENGTH> rx_packet;

/*TODO: re-validate COBS*/
/*TODO: re-validate CRC*/
Comms_CRC crc(0x1021, 0x1D0F, 0x0000); //CRC-16/AUG-CCITT, common 16-bit CRC

void app_init() {
	DIO::init();
	serial_comms.init();
}

void app_loop() {
	if(user_button.read())
		user_led.set();
	else user_led.clear();

	//check if we have a packet ready
	size_t packet_length = serial_comms.get_packet(rx_packet);
	if(packet_length) {
		serial_comms.transmit(std::span(rx_packet.begin() + 1, packet_length - 2));
	}
}


