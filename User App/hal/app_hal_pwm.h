/*
 * app_hal_pwm.h
 *
 *  Created on: Mar 26, 2023
 *      Author: Ishaan
 *
 *
 *  NOTE: this isn't "Hard" PWM per se, but it's not "Soft" PWM
 *  It exists in a middle ground where it relies on MCU timer peripherals
 *  basically it relies on MCU timer interrupts. However, instead of being bound to the particular PWM pin,
 *  It leverages the compare and overflow interrupts to set and reset whatever Output pin the user wants
 *  As a result, it's has higher performance/lower overhead than pure soft PWM, but performs worse than true hardware PWM
 *
 */

#ifndef BOARD_HAL_INC_APP_HAL_PWM_H_
#define BOARD_HAL_INC_APP_HAL_PWM_H_

extern "C" {
	#include "stm32g474xx.h"
}
#include "app_hal_int_utils.h"
#include "app_hal_dio.h"

typedef struct {
	bool placeholder;
} bridge_channel_info_t;

class BridgePWM {
public:

	//=================== INFO ABOUT PARTICULAR BRIDGE CONTROL PWM CHANNELS ===================
	static const bridge_channel_info_t CHANNEL_0_POSITIVE;
	static const bridge_channel_info_t CHANNEL_0_NEGATIVE;
	static const bridge_channel_info_t CHANNEL_1_POSITIVE;
	static const bridge_channel_info_t CHANNEL_1_NEGATIVE;
	//=========================================================================================


	BridgePWM(const bridge_channel_info_t _channel_info);

	static void disable(); //call to disable all pwm timers
	static void enable(); //call to enable all pwm timers; will reset the duty cycle of all channels on startup
	static void set_period(uint16_t period); //set the PWM period of all the PWM channels

	void set_pwm(uint16_t duty); //set the duty cycle of a particular channel (with respect to the assigned period
								 //minimal bounds checking to keep the function as lightweight as possible

	void force_deasserted(); //force the channel to be inactive


private:
	//don't allow one of these to be copied, since conflicts could arise when doing so
	BridgePWM(BridgePWM &other){}

	//have a place to store the channel information of the particular BridgePWM instance
	//lets us access the hardware resources corresponding to a particular half-bridge
	bridge_channel_info_t channel_info;
};

#endif /* BOARD_HAL_INC_APP_HAL_PWM_H_ */
