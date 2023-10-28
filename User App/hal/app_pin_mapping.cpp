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
const PinMap::DIO_Hardware_Channel PinMap::stage_enable = {PORT_C, 7};
const PinMap::DIO_Hardware_Channel PinMap::stage_enable_2 = {PORT_A, 7};

//====================== END PIN MAPPING DEFINITIONS ============================

