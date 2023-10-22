/*
 * app_config.h
 *
 *  Created on: Oct 19, 2023
 *      Author: Ishaan
 */

#ifndef CONFIG_APP_CONFIG_H_
#define CONFIG_APP_CONFIG_H_

#include <stddef.h> //for size_t
#include <array> //to hold c++ style arrays of stuff

extern "C" {
	#include "stm32g474xx.h" //for uint types
}

class Configuration {
public:

	//======================================= FIELDS WE'LL STORE IN OUR CONFIGURATION =========================================
	//NOTE: DEFAULT CONFIGURATION DEFINED IN `app_config.cpp`!

	//"magic numbers" defining how big we'll allow configuration strings to be
	static const size_t CONFIG_NAME_SIZE = 256;
	static const size_t CONFIG_DESC_SIZE = 1024;
	static const size_t POWER_STAGE_COUNT = 1;
	static constexpr float AMP_MAX_CHANNEL_CURRENT = 10.0f;

	//configuration for each power stage/regulation channel
	struct Power_Stage_Channel_Config {
		uint8_t CHANNEL_NO; //channel corresponding to the particular power stage instance
		static constexpr float CHANNEL_MAX_CURRENT = AMP_MAX_CHANNEL_CURRENT; //maximum current we'll allow the channel to drive

		//current sensing hardware configuration and ADC trim values
		float SHUNT_RESISTANCE; //current shunt resistance value in ohms
		float FINE_AMP_GAIN_VpV; //gain of the fine range current sense amplifier (Volts/Volt)
		float FINE_GAIN_TRIM;
		float FINE_OFFSET_TRIM;
		uint16_t FINE_RANGE_VALID_LOW; //minimum possible code for which the fine range is reliable (EXCLUSIVE)
		uint16_t FINE_RANGE_VALID_HIGH; //maximum possible code for which the fine range is reliable (EXCLUSIVE)
		float COARSE_AMP_GAIN_VpV; //gain of the coarse range current sense amplifier (Volts/Volt)
		float COARSE_GAIN_TRIM;
		float COARSE_OFFSET_TRIM;

		//specific controller parameters
		float K_DC; //controller DC gain, linear scale
		float F_CROSSOVER; //controller crossover frequency, Hz
		float SETPOINT_RECON_BANDWIDTH; //setpoint controller upsampling reconstruction filter bandwidth

		//load parameters
		float LOAD_RESISTANCE; //DC resistance of load, ohms
		float LOAD_CHARACTERISTIC_FREQ; //L-R characteristic frequency, Hz
	};

	struct Configuration_Params {
		uint32_t CONFIG_STORE_VERSION; //maintain a code of the configuration structure version (will be defined in firmware)
		std::array<char, CONFIG_NAME_SIZE> CONFIG_NAME; //user-defined name for the particular configuration
		std::array<char, CONFIG_DESC_SIZE> CONFIG_DESC; //user-description for the particular configuration

		//global power stage/regulator configuration parameters
		float DESIRED_SWITCHING_FREQUENCY; //power stage switching frequency
		float DESIRED_SAMPLING_FREQUENCY; //sampling/controller frequency
		float DESIRED_SETPOINT_TICK_FREQUENCY; //arbitrary waveforms were sampled at this frequency

		//channel-specific power stage configuration
		static const size_t NUM_POWER_STAGES = POWER_STAGE_COUNT;
		std::array<Power_Stage_Channel_Config, POWER_STAGE_COUNT> POWER_STAGE_CONFIGS;

		//have a configuration checksum--useful to validate memory corruption/successful loads
		//a correctly loaded config should have a CRC of 0
		uint16_t CONFIG_CRC;
	};

	//maintain an active configuration that we can read/write to
	Configuration_Params active;

	//======================================= CONFIG OPERATIONS =========================================
	//delete copy constructor and assignment operator to avoid weird stuff going on
	Configuration(Configuration const&) = delete;
	void operator=(Configuration const&) = delete;

	Configuration(); //constructor, so we can pass around an instance
	bool load_default(); //load in the default configuration
	bool load(size_t config_num); //load a particular configuration as specified by `num`
	bool store(size_t config_num); //store active configuration as a particular `num`

private:
	//internal function to make sure a configuration was loaded correctly
	static bool validate(Configuration_Params& to_validate);

	//maintain a default configuration for all configurations
	static const Configuration_Params DEFAULT_CONFIG;
};



#endif /* CONFIG_APP_CONFIG_H_ */
