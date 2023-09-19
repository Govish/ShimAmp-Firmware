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
 *	A side note for everyone--TRUST ME I know this class really should instantiate a tx/rx buffer visible only to particular class instances
 *		HOWEVER I cannot for the life of me figure out how to do this in a c++ esque way with the following characteristics
 *		- size determined by a parameter passed during instance creation/initialization
 *		- static allocation (at least when an instance is created statically)
 *		- plays nicely with static member variables (that require a pointer to an instance)
 *			\--> this last point is such that hardware ISRs can map to specific instances and their functions
 *		- do this in a nice c++ style way
 *
 *	Continuing this note, I'm handling this by passing a std::span with dynamic extent
 *	dynamic extent means we can handle variable size arrays without templating
 *	this is also applied to other places where an stl container is taken as an argument--std::span generally allows for a clean implementation
 *	Additionally, in all the code examples I've seen, `std::span`s are passed by value rather than reference--just wanted to note that up here
 *
 *	A really cool byproduct of this is that std::arrays can automatically cast to a `std::span` increasing code readability and concise-ness
 */

#ifndef HAL_APP_HAL_UART_H_
#define HAL_APP_HAL_UART_H_

#include <stddef.h> //for size_t
#include <array> //for stl arrays
#include <span> //for c++ style pointer+length data structures

#include "app_hal_int_utils.h" //for callback type

extern "C" {
	#include "stm32g474xx.h" //for types
	#include "usart.h" //for usart related types and functions
}

class UART {
public:
	//======================================== HARDWARE MAPPING to PHYSICAL UART ====================================

	struct UART_Hardware_Channel {
		UART_HandleTypeDef* const huart;
		const callback_function_t init_func;
		UART* instance; //points to the firmware instance corresponding to this hardware
	};
	static UART_Hardware_Channel LPUART;

	//===============================================================================================================

	UART(	UART_Hardware_Channel* const _hardware, const uint8_t _START_OF_FRAME, const uint8_t _END_OF_FRAME,
			std::span<uint8_t, std::dynamic_extent> _txbuf, std::span<uint8_t, std::dynamic_extent> _rxbuf);
	void init(); //initialize the uart peripheral

	//copies bytes into local transmit buffer
	//waits until transmitter is free, returns immediately when DMA is dispatched
	void transmit(const std::span<uint8_t, std::dynamic_extent> bytes_to_tx);

	//returns 0 if no packet received, otherwise copies packet over to rx_packet and returns length of packet
	size_t get_packet(std::span<uint8_t, std::dynamic_extent> rx_packet);

	void attach_uart_error_callback(const callback_function_t _err_cb); //function called when some UART error state occurs; MAY BE CALLED FROM ISR CONTEXT
	bool ready_to_send(); //return true if the transmitter is ready to send data
	bool uart_ok(); //return true if there are no error states in the UART

	//these functions are just called by interrupts; USER SHOULDN'T INTERACT WITH THESE
	void __attribute__((optimize("O3"))) RX_interrupt_handler();
	void __attribute__((optimize("O3"))) error_handler();
private:
	//maintain details relevant to the hardware channel (the class owns this, not the instance)
	UART_Hardware_Channel* const hardware;

	//maintain a definition of start-of-frame and end-of-frame characters
	const uint8_t START_OF_FRAME;
	const uint8_t END_OF_FRAME;

	//maintain a memory locations (and other variables) relevant to transmitting and receiving
	uint8_t received_char; //character we received from the most recent RX interrupt
	size_t rx_buffer_pointer = 0; //will point to the next free memory location in the buffer
	std::span<uint8_t, std::dynamic_extent> txbuf; //instantiating with dynamic extent such that we don't need to know size at compile time
	std::span<uint8_t, std::dynamic_extent> rxbuf; //same as above

	//variables relevant to receiving process
	volatile bool received_sof_good_packet = false; //we've received a SOF character and waiting for an EOF character
	volatile bool received_packet_pending = false; //flag to signal that an entire packet has been received and hasn't been serviced

	//callback function when UART error occurs
	callback_function_t err_cb = empty_cb;
};

#endif /* HAL_APP_HAL_UART_H_ */
