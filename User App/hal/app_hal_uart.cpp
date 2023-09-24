/*
 * app_hal_uart.cpp
 *
 *  Created on: Sep 14, 2023
 *      Author: Ishaan
 */

#include "app_hal_uart.h"

#include <algorithm> //for stl versions of memcpy

///========================= initialization of static fields ========================
//UART instance will be initialized in the constructor, so don't need to worry too much about the NULL here
UART::UART_Hardware_Channel UART::LPUART = {&hlpuart1, MX_LPUART1_UART_Init, NULL};

//===================================================================================

UART::UART(	UART_Hardware_Channel& _hardware, const uint8_t _START_OF_FRAME, const uint8_t _END_OF_FRAME,
			std::span<uint8_t, std::dynamic_extent> _txbuf, std::span<uint8_t, std::dynamic_extent> _rxbuf):
	hardware(_hardware), START_OF_FRAME(_START_OF_FRAME), END_OF_FRAME(_END_OF_FRAME), txbuf(_txbuf), rxbuf(_rxbuf)
{
	//point the hardware structure to the particular firmware instance
	hardware.instance = this;
}

void UART::init() {
	//call the appropriate initialization function
	//this is done by cubeMX
	hardware.init_func();

	//start listening for received packets over interrupt
	HAL_UART_Receive_IT(hardware.huart, &received_char, 1);
}

void UART::transmit(const std::span<uint8_t, std::dynamic_extent> bytes_to_tx) {
	//if our transmit buffer isn't big enough to transmit the message, don't even try sending
	if(bytes_to_tx.size() > txbuf.size()) return;

	//only copy over when the previous transmit has finished
	while(hardware.huart->gState != HAL_UART_STATE_READY); //wait for the UART to be ready for transmit

	//copy over the message using stl conventions
	std::copy(bytes_to_tx.begin(), bytes_to_tx.end(), txbuf.begin());

	//fire off a transmit over DMA with the amount of bytes corresponding to the input message
	HAL_UART_Transmit_DMA(hardware.huart, txbuf.data(), (uint16_t)bytes_to_tx.size());
}

size_t UART::get_packet(std::span<uint8_t, std::dynamic_extent> rx_packet) {
	//if we don't have a packet, return 0
	if(!received_packet_pending) return 0;

	//if we do have a packet

	//compute the packet size
	//the size will be rx_buffer_pointer + 1 since SOF will be at 0 and EOF will be at rx_buffer_pointer
	size_t packet_size = rx_buffer_pointer + 1;

	//ensure that we have enough space to put the RX buffer into the rx_packet
	//do this the canonical c++ way using
	if(rx_packet.size() >= packet_size) {
		auto buf_section = rxbuf.subspan(0, packet_size); //grab the relevant slice of the rx buffer
		std::copy(buf_section.begin(), buf_section.end(), rx_packet.begin()); //copy it into the packet passed in
	}

	//indicate that we have serviced the packet and we can start listening again
	received_packet_pending = false;

	return packet_size;
}

void UART::attach_uart_error_callback(const callback_function_t _err_cb) { this->err_cb = _err_cb; }
bool UART::ready_to_send() { return hardware.huart->gState == HAL_UART_STATE_READY; }
bool UART::uart_ok() { return HAL_UART_GetError(hardware.huart) == HAL_UART_ERROR_NONE; }

void UART::RX_interrupt_handler() {
	//we've received a packet and we're waiting for the main thread to process it
	//as such, only modify the buffer once the pending packet has already been dispatched
	if(!received_packet_pending) {

		//check if we've received a start of frame (and that we're not waiting on a packet to be serviced)
		if(received_char == START_OF_FRAME) {
			rxbuf[0] = received_char; //put the SOF in the first spot in our receive buffer
			rx_buffer_pointer = 1; //point to the next free index
			received_sof_good_packet = true; //start listening to the rest of the message
		}

		//alternatively, check if we've received an end-of-frame
		else if(received_char == END_OF_FRAME) {
			//if we've received a start of frame and we have space to add the character to the buffer
			//add the character to the buffer and notify the main thread
			if(received_sof_good_packet) {
				rxbuf[rx_buffer_pointer] = received_char;
				received_packet_pending = true;
			}

			//wait for a new frame to roll in
			received_sof_good_packet = false;
		}

		//in this case, any old character rolls in
		else {
			//write the character to the buffer
			rxbuf[rx_buffer_pointer] = received_char;

			//increment our buffer pointer if we can
			//if we can't, it means we have a bad packet; prevent it from being dispatched
			if(rx_buffer_pointer < rxbuf.size() - 1) rx_buffer_pointer++;
			else received_sof_good_packet = false;
		}
	}

	//listen for more bytes over UART
	HAL_UART_Receive_IT(hardware.huart, &received_char, 1);
}

//just run the error callback when the error handler is invoked
void UART::error_handler() {
	this->err_cb();
}

//======================================= PROCESSOR ISRs ====================================

//call the appropriate RX complete interrupt handler according to which UART caused the interrupt
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if(huart == UART::LPUART.huart) UART::LPUART.instance->RX_interrupt_handler();
}

//call the appropriate RX complete interrupt handler according to which UART caused the interrupt
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if(huart == UART::LPUART.huart) UART::LPUART.instance->error_handler();
}
