/*
 * Device.cpp
 *
 * Created: 06.12.2020 14:44:51
 *  Author: V17
 */ 

#include "Device.h"
#include "Screens/ScreenManager.h"
#include "USART/USART.h"
#include "ADC/ADC.h"
#include "Spi/spi.h"
#include "SCPI/SCPI_Device.h"

#include <stdio.h>

DeviceClass Device;
DevSettingsEEPROMLayout_t EEMEM NonVolatileSettings;

DeviceClass::DeviceClass() :
	PsChannel(PS_MIN_AMPLITUDE, PS_MAX_AMPLITUDE, PS_MIN_LOAD, PS_MAX_LOAD, PS_MIN_OVP_LEVEL_PERCENTAGE, PS_MAX_OVP_LEVEL_PERCENTAGE, PS_MIN_OVP_DELAY, PS_MAX_OVP_DELAY),
	DdsChannel1(DDS_MIN_FREQ, DDS_MAX_FREQ, DDS_MIN_AMPLITUDE, DDS_MAX_AMPLITUDE, DDS_MIN_OFFSET, DDS_MAX_OFFSET),
	DdsChannel2(DDS_MIN_FREQ, DDS_MAX_FREQ, DDS_MIN_AMPLITUDE, DDS_MAX_AMPLITUDE, DDS_MIN_OFFSET, DDS_MAX_OFFSET),
	DmmChannel1(),
	DmmChannel2(),
	Channels{ &PsChannel, &DdsChannel1, &DdsChannel2, &DmmChannel1, &DmmChannel2 }
{
	DeviceControlState = DEV_CTRL_LOCAL;
}

void DeviceClass::Init()
{
	cli();
	Pins_Init();
	SPI_Init();
	DisableDDSTimer();
	Encoder_Init();
	ADC_init();
	Usart0Init(9600);			// Always init with 9600 baud to output the power on message.
	InitDeviceTimer();
	sei();
	
	Usart0TransmitStr("Power On\r\n");
	
	ADC_startConversion();
	MCP4922_DisableLatching();
	
	ScreenManager.Init();
	
	Device.LoadSettings();
	
	#ifdef SCPI_ENABLED
		SCPI_Init_Device();
	#endif
}

void DeviceClass::DeviceMainLoop()
{
	UserInputHandler.ProcessInputs();
	
	if(IsScreenRedrawRequested)
	{
		ScreenManager.DrawAll();
		IsScreenRedrawRequested = false;
	}
	
	if(TimeCounter_AutoSave_ms >= SETTINGS_AUTOSAVE_DELAY_MS)
	{
		TimeCounter_AutoSave_ms = 0;
		if(DevSettingsNeedSaving)
		{
			SaveSettings();
		}
	}
}

// ##### Timer #######################################################################################################################

ISR(TIMER1_COMPA_vect)
{
	Device.DeviceTimerTickISR(DEVICE_TIMER_TICK_INTERVAL_MS);
}

/* Initialize 16-bit Timer/Counter1 */
void DeviceClass::InitDeviceTimer()
{
	TimeCounter_KeyPolling_ms = 0;
	TimeCounter_AutoSave_ms = 0;
	TCCR1B = (1 << WGM12);											// Configure for CTC mode
	TCCR1B |= ((1 << CS10) | (1 << CS11));							// Prescaler 64
	TIMSK1 = (1 << OCIE1A);											// Enable Output Compare A Match Interrupt
	OCR1A = (F_CPU / 64 / (1000 / DEVICE_TIMER_TICK_INTERVAL_MS));	// Set compare register A (USER_TIMER_TICK_FREQ Hz)
}

void DeviceClass::DeviceTimerTickISR(uint16_t currentPeriod_ms)
{	
	TimeCounter_KeyPolling_ms += currentPeriod_ms;
	if(TimeCounter_KeyPolling_ms >= KEY_POLLING_DELAY_MS)
	{
		IsScreenRedrawRequested = true;
		
		TimeCounter_KeyPolling_ms = 0;
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
	
	TimeCounter_AutoSave_ms += currentPeriod_ms;			// AutoSave is handled in DeviceMainLoop()
	ScreenManager.DeviceTimerTickISR(currentPeriod_ms);
	PsChannel.DeviceTimerTickISR(currentPeriod_ms);
}

// ##### Device Control State ########################################################################################################

bool DeviceClass::IsUserInputLocked() 
{
	 return (DeviceControlState != DEV_CTRL_LOCAL || strcmp(ScreenManager.UserMessage, "") != 0 || strcmp(ScreenManager.SystemMessage, "") != 0); 
}

void DeviceClass::UpdateControlStateOnUserInput()
{
	if(strcmp(ScreenManager.UserMessage, "") != 0)
	{
		strcpy(ScreenManager.UserMessage, "");
	}
	else
	{
		switch(DeviceControlState)
		{
			case DEV_CTRL_LOCAL: strcpy(ScreenManager.SystemMessage, ""); /* Nothing to do here. Device is already in local state. */ break;
			case DEV_CTRL_REMOTE: DeviceControlState = DEV_CTRL_LOCAL; break;
			case DEV_CTRL_RWLOCK: /* Nothing to do here. It is only possible to return from state RWLock via SCPI command. */ break;
		}
	}	
}

// ##### Communication ###############################################################################################################

void DeviceClass::SetSerialBaudRate(uint32_t baud)
{
	if(baud != 110 && baud != 150 && baud != 300 && baud != 1200 && baud != 2400 && baud != 4800 && baud != 9600 && baud != 19200 && baud != 38400 && baud != 57600)		/* Check for invalid baud rates */
	{
		Usart0TransmitStr("Invalid baud rate selected. Nothing done.\r\n");
		return;
	}
	
	if(SerialBaudRate != baud && baud > 0 && baud < 115200)
	{
		DevSettingsNeedSaving = true;
		char buffer[60];
		sprintf(buffer, "Changing Baud rate from %lu to %lu\r\n", (SerialBaudRate == 0 ? 9600 : SerialBaudRate), baud);
		Usart0TransmitStr(buffer);
		SerialBaudRate = baud;
		Usart0ChangeBaudRate(baud);	
	}
}

void DeviceClass::SetSerialEchoEnabled(bool echoEnabled)
{
	DevSettingsNeedSaving = (SerialEchoEnabled != echoEnabled);
	SerialEchoEnabled = echoEnabled;
}

// ##### Settings ####################################################################################################################

void DeviceClass::SaveSettings()
{
	DevSettingsEEPROMLayout_t settings;
	/* Collect setting from appropriate classes */
	settings.Screens_TabIndex = ScreenManager.TabIndex;
	settings.Screens_Inverted = ScreenManager.DisplayInverted;
	settings.Device_SerialBaudRate = SerialBaudRate;
	settings.Device_SerialEchoEnabled = SerialEchoEnabled;
	
	settings.PS_Voltage = PsChannel.GetAmplitude();
	settings.PS_LoadImpedance = PsChannel.GetLoadImpedance();
	settings.PS_Enabled = PsChannel.GetEnabled();
	settings.PS_OvpLevel = PsChannel.GetOvpLevel();
	settings.PS_OvpState = PsChannel.GetOvpState();
	settings.PS_OvpDelay = PsChannel.GetOvpDelay();
		
	settings.DDS1_Frequency = DdsChannel1.GetFrequency();
	settings.DDS1_SignalForm = DdsChannel1.GetSignalForm();
	settings.DDS1_Amplitude = DdsChannel1.GetAmplitude();
	settings.DDS1_Offset = DdsChannel1.GetOffset();
	settings.DDS1_Enabled = DdsChannel1.GetEnabled();

	settings.DDS2_Frequency = DdsChannel2.GetFrequency();
	settings.DDS2_SignalForm = DdsChannel2.GetSignalForm();
	settings.DDS2_Amplitude = DdsChannel2.GetAmplitude();
	settings.DDS2_Offset = DdsChannel2.GetOffset();
	settings.DDS2_Enabled = DdsChannel2.GetEnabled();

	settings.PowerOnOutputsDisabled = PowerOnOutputsDisabled;

	eeprom_write_block((const void*)&settings, (void*)&NonVolatileSettings, sizeof(DevSettingsEEPROMLayout_t));
	
	DevSettingsNeedSaving = false;
}

void DeviceClass::LoadSettings()
{
	DevSettingsEEPROMLayout_t settings;
	eeprom_read_block((void*)&settings, (const void*)&NonVolatileSettings, sizeof(DevSettingsEEPROMLayout_t));
	
	/* Assign Settings to appropriate classes */
	
	ScreenManager.TabIndex = settings.Screens_TabIndex;
	ScreenManager.SetDisplayInverted(settings.Screens_Inverted);
	
	SetSerialBaudRate(settings.Device_SerialBaudRate);
	SetSerialEchoEnabled(settings.Device_SerialEchoEnabled);
	
	PsChannel.SetAmplitude(settings.PS_Voltage);
	PsChannel.SetLoadImpedance(settings.PS_LoadImpedance);
	PsChannel.SetOvpLevel(settings.PS_OvpLevel);
	PsChannel.SetOvpState(settings.PS_OvpState);
	PsChannel.SetOvpDelay(settings.PS_OvpDelay);
			
	DdsChannel1.SetFrequency(settings.DDS1_Frequency);
	DdsChannel1.SetSignalForm(settings.DDS1_SignalForm);
	DdsChannel1.SetAmplitude(settings.DDS1_Amplitude);
	DdsChannel1.SetOffset(settings.DDS1_Offset);
	
	DdsChannel2.SetFrequency(settings.DDS2_Frequency);
	DdsChannel2.SetSignalForm(settings.DDS2_SignalForm);
	DdsChannel2.SetAmplitude(settings.DDS2_Amplitude);
	DdsChannel2.SetOffset(settings.DDS2_Offset);
	
	PowerOnOutputsDisabled = settings.PowerOnOutputsDisabled;
	
	PsChannel.SetEnabled(PowerOnOutputsDisabled ? false : settings.PS_Enabled);
	DdsChannel1.SetEnabled(PowerOnOutputsDisabled ? false : settings.DDS1_Enabled);
	DdsChannel2.SetEnabled(PowerOnOutputsDisabled ? false : settings.DDS2_Enabled);
	
	DevSettingsNeedSaving = false;
}

void DeviceClass::ResetDevice()
{
	ScreenManager.TabIndex = 0;
	ScreenManager.SetDisplayEnabled(true);
	ScreenManager.SetDisplayInverted(false);
	
	PsChannel.SetAmplitude(5);
	PsChannel.SetEnabled(false);
	PsChannel.SetLoadImpedance(1000000);
	PsChannel.SetOvpLevel(120);
	PsChannel.SetOvpState(false);
	PsChannel.SetOvpDelay(20);
	
	DdsChannel1.SetEnabled(false);
	DdsChannel1.SetFrequency(1000);
	DdsChannel1.SetSignalForm(SINE);
	DdsChannel1.SetAmplitude(10);
	DdsChannel1.SetOffset(0);
	
	DdsChannel2.SetEnabled(false);
	DdsChannel2.SetFrequency(1000);
	DdsChannel2.SetSignalForm(SINE);
	DdsChannel2.SetAmplitude(10);
	DdsChannel2.SetOffset(0);
	
	PowerOnOutputsDisabled = true;
	
	SaveSettings();
}
