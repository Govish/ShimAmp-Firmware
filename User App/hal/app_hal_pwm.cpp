/*
 * app_hal_pwm.cpp
 *
 *  Created on: Mar 26, 2023
 *      Author: Ishaan
 */

#include "app_hal_pwm.h"
extern "C" {
	#include "hrtim.h"
}

BridgePWM::BridgePWM(const bridge_channel_info_t _channel_info) {

}

//call to disable all pwm timers
void BridgePWM::disable() {

}

//call to enable all pwm timers; will reset the duty cycle of all channels on startup
void BridgePWM::enable() {

}

//set the PWM period of all the PWM channels
void BridgePWM::set_period(uint16_t period) {

}

//set the duty cycle of a particular channel (with respect to the assigned period
//minimal bounds checking to keep the function as lightweight as possible
void BridgePWM::set_pwm(uint16_t duty) {

}

//force the channel to be inactive
void BridgePWM::force_deasserted() {

}
