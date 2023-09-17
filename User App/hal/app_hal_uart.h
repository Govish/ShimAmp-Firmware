/*
 * app_hal_uart.h
 *
 *  Created on: Sep 14, 2023
 *      Author: Ishaan
 *
 *  Notes to myself:
 *  	- Don't need to worry about DMA complete callback for transmit--just check about UART busy basically
 *  	- Call `service_RX_interrupt()` in the rx complete interrupt (wait this should 100% be called by the class itself
 *  	- can technically get a segmentation fault if i pass a buffer that is less than size 2
 *  		- could catch this with a STATIC_ASSERT() or something but I'm making that a future-me problem
 *
 *  I knowwwww rxbuf should really be something that's created locally to this class
 *  	but using templates is sooooo much more cumbersome and unreadable than just passing in some pre-instantiated buffer
 *  	so yeah fight me i guess
 */

#ifndef HAL_APP_HAL_UART_H_
#define HAL_APP_HAL_UART_H_

#include <stddef.h> //for size_t
#include "app_hal_int_utils.h" //for callback type

extern "C" {
	#include "stm32g474xx.h" //for types
	#include "usart.h" //for usart related types and functions
}

class UART {
public:
	//======================================== HARDWARE MAPPING to PHYSICAL UART ====================================
	typedef struct {
		UART_HandleTypeDef* const huart;
		const callback_function_t init_func;
		UART* instance; //points to the firmware instance corresponding to this hardware
	} UART_hardware_channel_t;

	static UART_hardware_channel_t LPUART;
	//===============================================================================================================

	UART(UART_hardware_channel_t* const _hardware, const uint8_t _SOF, const uint8_t _EOF, uint8_t* const _rxbuf, const size_t _rxbuflen);
	void init(); //initialize UART peripheral, feels kinda dumb but just pass the init func
	void transmit(const uint8_t* buf, size_t len); //waits until transmitter is free, returns immediately when DMA is dispatched

	size_t get_packet(uint8_t* const rx_packet); //returns 0 if no packet received, otherwise copies packet over to rx_packet and returns length of packet

	void attach_uart_error_callback(const callback_function_t _err_cb); //function called when some UART error state occurs; MAY BE CALLED FROM ISR CONTEXT

	bool ready_to_send(); //return true if the transmitter is ready to send data
	bool uart_ok(); //return true if there are no error states in the UART

	//these functions are just called by interrupts; USER SHOULDN'T INTERACT WITH THESE
	void __attribute__((optimize("O3"))) RX_interrupt_handler();
	void __attribute__((optimize("O3"))) error_handler();
private:
	//maintain details relevant to the hardware channel (the class owns this, not the instance)
	UART_hardware_channel_t* const hardware;

	//maintain a definition of start-of-frame and end-of-frame characters
	const uint8_t SOF;
	const uint8_t EOF;

	//maintain a memory location where we can dump received characters
	uint8_t received_char; //character we received from the most recent RX interrupt
	size_t rx_buffer_pointer = 0; //will point to the next free memory location in the buffer
	uint8_t* rxbuf; //receive buffer
	const size_t rxbuflen; //length of receive buffer

	//variables relevant to receiving process
	volatile bool received_sof_good_packet = false; //we've received a SOF character and waiting for an EOF character
	volatile bool received_packet_pending = false; //flag to signal that an entire packet has been received and hasn't been serviced

	//callback function when UART error occurs
	callback_function_t err_cb = empty_cb;
};

#endif /* HAL_APP_HAL_UART_H_ */
