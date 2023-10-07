/*
 * app_pin_mapping.cpp
 *
 *  Created on: Mar 11, 2023
 *      Author: Ishaan
 */

#include "app_pin_mapping.h"

//====================== BEGIN PIN MAPPING DEFINITIONS ==========================

const PinMap::DIO_Hardware_Channel PinMap::status_led = {PORT_A, 5}; //port bit configuration for LED (xx corresponds to speed value): 010xx00
const PinMap::DIO_Hardware_Channel PinMap::user_button = {PORT_C, 13};

//====================== END PIN MAPPING DEFINITIONS ============================

