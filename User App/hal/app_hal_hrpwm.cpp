/*
 * app_hal_hrpwm.cpp
 *
 *  Created on: Sep 30, 2023
 *      Author: Ishaan
 */

#include "app_hal_hrpwm.h"


//=========================================== CLASS MEMBER FUNCTIONS =========================================
void HRPWM::DISABLE_ALL() {

}

void HRPWM::ENABLE_ALL() {

}

void HRPWM::SET_PERIOD_ALL() {

}

//master timer period (raw register read basically, no unit conversion)
uint16_t HRPWM::GET_PERIOD() {

}

//switching frequency in Hz
float HRPWM::GET_FSW() {

}

/*TODO: ADC synchronization and period elapsed callback*/

//=========================================== INSTANCE METHODS =========================================

HRPWM::HRPWM() {

}

void HRPWM::force_low() {

}

//duty cycle 0-1; bounds checked version, returns true if set successfully
bool HRPWM::set_duty(float duty) {

}

//faster, non-bounds-checked version of set_duty(float)
void HRPWM::set_duty(uint16_t duty) {

}

float HRPWM::get_duty() {

}

//faster version of get_duty; no float conversion
uint16_t HRPWM::get_raw_duty() {

}
