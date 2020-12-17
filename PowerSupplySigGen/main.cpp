/*
 * PowerSupplySigGen.cpp
 *
 * Created: 14.08.2020 19:23:21
 * Author : V17
 */ 

/********************************************************
* ATMega 1284P
* avrdude -p m1284p -P usb -c usbasp 
* -U lfuse:w:0xFF:m (external Crystal 8-16 MHz)
* -U hfuse:w:0xD1:m (JTAG Interface Disable, Preserve EEPROM memory through the Chip Erase cycle)
* -U efuse:w:0xFC:m (Brown-out detection level at VCC=4.3 V)
********************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "Spi/spi.h"
#include "Pins/Pins.h"
#include "GLCD/u8g.h"
#include "Outputs/DAC_MCP492x.h"
#include "KeyPad/KeyPad.h"
#include "Encoder/Encoder.h"
#include "ADC/ADC.h"
#include "USART/USART.h"
#include "Global/DevSettings.h"
#include "Global/DevStatus.h"
#include "UserInputHandler/UserInputHandler.h"
#include "Screens/ScreenManager.h"
#include "SCPI/SCPI_Device.h"
#include "Configuration.h"

#include "Device.h"

u8g_t u8g;

/*typedef enum SignalForms
{
	SINE,
	RECTANGLE,
	TRIANGLE
} SignalForms_t;
const char* SignalFormsNames[] = { "SINE", "RECT", "TRIANGLE" };*/

/*SignalForms_t signalForm;
UserControlEnum<SignalForms_t> ctrlEnum (SCREEN_TAB_WIDTH + 10, 30, &signalForm, SignalFormsNames, 3);*/


DevStatus_t DevStatus;
uint16_t UserTimerTickCounter;

ISR(TIMER1_COMPA_vect)
{
	UserTimerTickCounter++;
	Keys_t key = KeyPad_GetKeys();
	if(key != KEYNONE)
	{
		UserInputHandler.EnqueueKeyInput(key);
	}
	if(Encoder_IsButtonPressed())
	{
		UserInputHandler.EnqueueEncoderButtonInput();
	}
}

/* Initialize 16-bit Timer/Counter1 */
void InitUserTimer()
{
	TCCR1B = (1 << WGM12);							// Configure for CTC mode
	TCCR1B |= ((1 << CS10) | (1 << CS11));			// Prescaler 64
	TIMSK1 = (1 << OCIE1A);							// Enable Output Compare A Match Interrupt
	OCR1A = (F_CPU / 64 / USER_TIMER_TICK_FREQ);	// Set compare register A (USER_TIMER_TICK_FREQ Hz)
}

int main(void)
{
	cli();
	Pins_Init();
	SPI_Init();
	Encoder_Init();
	ADC_init();
	Usart0Init(9600);		/* Always init with 9600 baud to output the power on message. */
	InitUserTimer();
	sei();
	
	Usart0TransmitStr("Power On\r\n");
		
	ADC_startConversion();
	MCP4922_DisableLatching();
	MCP4922_Voltage_Set(2500, 'A');
	MCP4922_Voltage_Set(5000, 'B');
	
	u8g_InitSPI(&u8g, &u8g_dev_s1d15721_hw_spi, PN(1, 7), PN(1, 5), PN(1, 1), PN(1, 0), U8G_PIN_NONE);
	
	ScreenManager.SetU8GLib_Object(&u8g);
	LoadSettings();
		
	UserTimerTickCounter = 0;
	
	SCPI_Init_Device();
	
	for(;;)
	{		
		UserInputHandler.ProcessInputs();	
		
		DevStatus_t devStatusDraw = DevStatus;
		u8g_FirstPage(&u8g);
		do
		{
			ScreenManager.Draw(devStatusDraw);
		} while ( u8g_NextPage(&u8g) );
		u8g_Delay(100);
		
		if((UserTimerTickCounter * (1 / (float)USER_TIMER_TICK_FREQ)) >= SETTINGS_AUTOSAVE_DELAY_SEC)
		{
			if(DevSettingsNeedSaving)
			{
				SaveSettings();
			}
			UserTimerTickCounter = 0;
		}
	}
	
}