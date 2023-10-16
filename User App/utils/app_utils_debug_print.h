/*
 * app_utils_debug_print.h
 *
 *  Created on: Oct 16, 2023
 *      Author: Ishaan
 *
 *  Use this to print to a serial monitor
 */

#ifndef UTILS_APP_UTILS_DEBUG_PRINT_H_
#define UTILS_APP_UTILS_DEBUG_PRINT_H_

#include <array> //to create TX and RX buffers
#include <string> //to pass strings to and from

#include "app_hal_uart.h" //print over some kinda UART connection

class Debug_Print {
public:
	Debug_Print(UART::UART_Hardware_Channel& hw);

	//delete copy constructor and assignment operator
	Debug_Print(Debug_Print const&) = delete;
	void operator=(Debug_Print const&) = delete;

	//call to initialize the UART
	void init();

	//print a line to the serial port (make sure it can fit in the tx buffer in one go)
	//BLOCKING FUNCTION CALL UNTIL LINE CAN SEND
	void print(std::string text);

	//read a complete line from the serial port
	//ENSURE THAT LINES ARE TERMINATED WITH "\r\n" IN THAT ORDER
	//BLOCKING FUNCTION CALL UNTIL LINE IS RECEIVED
	std::string read();

private:
	static const size_t BUFFER_LENGTH = 1024;

	UART debug_serial_port;
	std::array<uint8_t, BUFFER_LENGTH> txbuf; //place for UART to put outgoing serial data
	std::array<uint8_t, BUFFER_LENGTH> rxbuf; //place for UART to put incoming serial data

	std::array<uint8_t, BUFFER_LENGTH> tx_conversion_buffer; //intermediate place to put string bytes before tx
	std::array<uint8_t, BUFFER_LENGTH> rx_intermediate_buffer; //place to get packets from debug uart
};



#endif /* UTILS_APP_UTILS_DEBUG_PRINT_H_ */
