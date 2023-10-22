/*
 * app_hal_adc.h
 *
 *  Created on: Oct 15, 2023
 *      Author: Ishaan
 *
 *
 */

#ifndef HAL_APP_HAL_ADC_H_
#define HAL_APP_HAL_ADC_H_

#include <utility> //for pair

#include "app_hal_int_utils.h" //for callback type

extern "C" {
	#include "stm32g474xx.h" //for types
	#include "adc.h" //for adc related types and functions
}

class Triggered_ADC {
public:
	//======================================== HARDWARE MAPPING to PHYSICAL UART ====================================

	enum class Input_Mode {
		SINGLE_ENDED,
		DIFFERENTIAL
	};

	struct Triggered_ADC_Hardware_Channel {
		ADC_HandleTypeDef* const hadc;
		const callback_function_t init_func;
		const Input_Mode in_mode; //channel is either single-ended or differential
		uint16_t adc_val;
		bool recently_updated;
		callback_function_t interrupt_callback;
	};

	static Triggered_ADC_Hardware_Channel CHANNEL_3;

	//=================== REGISTER DEFS ==================

	static const uint32_t CLEAR_ALL_INTERRUPTS = 0x7FF; //write this to ADC_ISR to clear all interrupts

	//===============================================================================================================

	Triggered_ADC(Triggered_ADC_Hardware_Channel& _hardware); //constructor

	/*
	 * Initialization routine does the following:
	 *  - initializes hardware through HAL functions
	 *  - configures the channel in single-ended or differential mode accordingly
	 *  - calibrates the particular ADC channel
	 *  - configures the particular ADC channel in BULB sampling mode
	 *  - Enables the ADC operation in triggered + interrupt mode
	 */
	void init();

	//map a callback function that called during a conversion complete interrupt
	void attach_cb(callback_function_t cb);

	//adjust the gain and the offset of the ADC
	//just modifying the control gains and offset since we'll have to do computation with this anyway
	//GAIN in units ratio that's multiplying the existing gain
	//OFFSET in unit `ADC_counts` that's summing into the existing offset
	void trim(float _gain_trim, float _offset_trim);
	std::pair<float, float> get_trim(); //returns gain_trim, offset_trim

	//get whether there has been a new ADC reading since the previous value had been read out
	bool get_updated();

	//get the ADC value; heavily optimize since we'll be running this in the control loop
	uint16_t __attribute__((optimize("O3"))) get_val(const bool clear_flag = true); //optionally dont' clear the `recently_triggered` flag

	//for control loop, get the voltage --> ADC code transfer coefficients (takes into account single-ended vs differential mode operation)
	//pair is in the form of <gain, offset>, such that gain, offset satisfy the following equation (adc voltage input, adc code output)
	// ADC_CODE = `gain` * TERMINAL_VOLTAGE + `offset`
	//heavily optimizing since will be called from interrupt context (though this is a relatively simple function)
	std::pair<float, float> get_gain_offset(); //[gain, offset]

private:

	//================== ADC CONVERSION CONSTANTS ===============

	//conversion factor between ADC values and voltages
	static constexpr float ADC_REFERENCE_VOLTAGE = 3.3;
	static constexpr float ADC_MAX_CODE = (float)0xFFF0; //left aligned data, 12 bits extended to 16 bits

	//================== MEMBER VARIABLES ===============

	//store gain and offset values for each ADC instance
	//will initialize these to different values based off of differential or single-ended operation
	float gain_v_to_counts;
	float offset_counts;

	//additionally, store the trim value related to the ADC channel
	float gain_trim = 1;
	float offset_trim = 0;

	//store a reference to the hardware
	//adc readings and recently updated flags will be stored with the hardware instance
	//	\==> this is done so we don't have to expose a function the hardware calls to update the read ADC value
	Triggered_ADC_Hardware_Channel& hardware;
};



#endif /* HAL_APP_HAL_ADC_H_ */
