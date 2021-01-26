/*
 * ADC.cpp
 *
 * Created: 15.09.2020 19:30:49
 *  Author: V17
 */ 

#include "ADC.h"
#include <avr/interrupt.h>
#include "../Device.h"
#include "../Configuration.h"

void ADC_init()
{
	ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);		// Set ADC clock to 1/128 of XTAL frequency
	ADMUX &= ~(1<<REFS0 | 1<<REFS1);					// Set reference to AREF, Internal Vref turned off
	ADCSRA |= 1<<ADIE;			// Enable ADC Interrupt
	ADCSRA |= 1<<ADEN;			// Enable ADC

	//After ADC_init():
	//Enable Interrupts: sei();
	//Then start first conversion: ADC_startConversion();
}

void ADC_startConversion()
{
	ADCSRA |= 1<<ADSC;			// Start ADC conversion
}

ISR(ADC_vect)
{
	uint16_t adcResult = ADCW;				// ADC conversion result with 10-bit resolution
	float adcVoltage = (float)(adcResult * (AVR_VCC_REF / 1024.0f));		// Vin = ADC * Vref / 1024 ; Vref=5.27V
	char adcChannel = (ADMUX & 0x07);		// Lower 3 bits represent the current ADC channel

	switch (adcChannel)
	{
		case 0:
			Device.DeviceVoltages.PS_CURR = adcVoltage / 2.4;		// Ucurr = R24 * (R22 / R23) * IL	=> IL = Ucurr / (R24 * (R22 / R23))
			Device.DeviceVoltages.PS_POWER = Device.DeviceVoltages.PS_VOLT * Device.DeviceVoltages.PS_CURR;
			break;
		case 1:
			Device.DeviceVoltages.PS_VOLT = adcVoltage * 2;
			Device.DeviceVoltages.PS_POWER = Device.DeviceVoltages.PS_VOLT * Device.DeviceVoltages.PS_CURR;
			break;
		case 2:
			Device.DeviceVoltages.ATX_12V_NEG = -adcVoltage * 2.4;
			break;
		case 3:
			Device.DeviceVoltages.ATX_12V = adcVoltage * 3; // 2.5;
			break;
		case 4:
			Device.DeviceVoltages.ATX_5V = adcVoltage;
			break;
		case 5:
			Device.DeviceVoltages.ATX_3V3 = adcVoltage;
			break;
		case 6:
			Device.DeviceVoltages.DMM1 = adcVoltage * 5.17;
			break;
		case 7:
			Device.DeviceVoltages.DMM2 = adcVoltage * 5.17;
			break;
		default: break;
	}

	adcChannel++;
	if(adcChannel > 7) { adcChannel = 0; }
	ADMUX = (ADMUX & 0xF8) + adcChannel;	// Set lower bits of ADMUX to select ADC channel
	ADCSRA |= 1<<ADSC;						// Start new ADC conversion if not all channels are converted
}