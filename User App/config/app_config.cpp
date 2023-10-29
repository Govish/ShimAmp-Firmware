/*
 * app_config.cpp
 *
 *  Created on: Oct 19, 2023
 *      Author: Ishaan
 */

#include "app_config.h"

//========================================== DEFAULT CONFIGURATION DEFINITION ==========================================

static const Configuration::Power_Stage_Channel_Config DEFAULT_CONFIG_PS_CHANNEL_0 = {
		.CHANNEL_NO = 0, //configuration describes channel 0

		//current sensing ADC trim values
		.SHUNT_RESISTANCE = 10e-3,
		.FINE_AMP_GAIN_VpV = 100, //INA241x4
		.FINE_GAIN_TRIM = 1,
		.FINE_OFFSET_TRIM = 0,
		.FINE_RANGE_VALID_LOW = 2048 - 1500,
		.FINE_RANGE_VALID_HIGH = 2048 + 1500,
		.COARSE_AMP_GAIN_VpV = 10, //INA241x1
		.COARSE_GAIN_TRIM = 1,
		.COARSE_OFFSET_TRIM = -10, //TODO CONVERT TO AMPS

		//controller parameters
		.K_DC = 1000.0, //controller DC gain ~1000
		.F_CROSSOVER = 20000.0, //current controller should cross over around 20kHz
		.SETPOINT_RECON_BANDWIDTH = 10000.0, //setpoint reconstruction filter should start rolling off here

		//parameters for the shim coil load
		.LOAD_RESISTANCE = 200e-3, //default to 100mR load
		.LOAD_CHARACTERISTIC_FREQ = 20000, //mostly resistive load
};

const Configuration::Configuration_Params Configuration::DEFAULT_CONFIG = {
		//some names, labels, and descriptions
		.CONFIG_STORE_VERSION = 0,
		.CONFIG_NAME = {"Default Configuration - 3T Scanner Testing"},
		.CONFIG_DESC = {
				"This is the Default Configuration. Use this to store user-adjustable parameters that govern the operation of the amplifier. \
				 As of now, I have this configuration file set up for testing with a 3T scanner."
		},

		//global power stage parameters
		.DESIRED_SWITCHING_FREQUENCY = 1.42857142e6, //start with a 1.42857MHz switching frequency (10MHz/7)
		.DESIRED_SAMPLING_FREQUENCY = 150e3, //and a 250kHz current sampling frequency
		.DESIRED_SETPOINT_TICK_FREQUENCY = 40e3, //arbitrary waveforms were sampled at this rate

		//power stage configuration parameters
		.POWER_STAGE_CONFIGS = {
				DEFAULT_CONFIG_PS_CHANNEL_0
		},

		.CONFIG_CRC = 0,
};

//============================================= CONFIGURATION METHODS (PUBLIC) =============================================

Configuration::Configuration() {
	//just load the default configuration upon initialization
	load_default();
}

bool Configuration::load_default() {
	//just copy over the default configuration to the active one
	active = DEFAULT_CONFIG;
	return validate(active); //nothing should really go wrong here
}

bool Configuration::store(size_t config_num) {
	/*TODO*/
	return false;
}

bool Configuration::load(size_t config_num) {
	/*TODO*/
	return false;
}

//============================================= CONFIGURATION METHODS (PRIVATE) =============================================

bool Configuration::validate(Configuration_Params& to_validate) {
	//just check if the CRC is zero; nothing too fancy (or robust I guess)
	return to_validate.CONFIG_CRC == 0;
}
