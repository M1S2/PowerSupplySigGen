/*
 * Device.cpp
 *
 * Created: 06.12.2020 14:44:51
 *  Author: V17
 */ 

#include "Device.h"
#include "USART/USART.h"
#include "ADC/ADC.h"
#include "Spi/spi.h"
#include "SCPI/SCPI_Device.h"

#include <stdio.h>

DeviceClass Device;
DevSettingsEEPROMLayout_t EEMEM NonVolatileSettings;
DeviceCalibrationFactors_t EEMEM NonVolatileSettings_CalibrationFactors;
#ifdef DDS_USER_DEFINED_WAVEFORMS_ENABLED
	DevSettingsUserDDSWaveformEEPROMLayout_t EEMEM NonVolatileSettings_DDSUserWaveforms;
#endif

const char* DeviceControlStateNames[] = { "LOC", "REM", "RWL" };
const char* DevicePowerUpOutputEnabledStateNames[] = { "OFF", "LAST", "ON" };
const char* DeviceBaudRateNames[] = { "110", "150", "300", "1200", "2400", "4800", "9600", "19200", "38400", "57600" };
	
DeviceClass::DeviceClass() :
	PsChannel(PS_MIN_VOLTAGE, PS_MAX_VOLTAGE, PS_MIN_CURRENT, PS_MAX_CURRENT, PS_MIN_OVP_LEVEL_PERCENTAGE, PS_MAX_OVP_LEVEL_PERCENTAGE, PS_MIN_OVP_DELAY, PS_MAX_OVP_DELAY, PS_MIN_OCP_LEVEL_PERCENTAGE, PS_MAX_OCP_LEVEL_PERCENTAGE, PS_MIN_OCP_DELAY, PS_MAX_OCP_DELAY, PS_MIN_OPP_LEVEL, PS_MAX_OPP_LEVEL, PS_MIN_OPP_DELAY, PS_MAX_OPP_DELAY),
	DdsChannel1(1, DDS_MIN_FREQ, DDS_MAX_FREQ, DDS_MIN_AMPLITUDE, DDS_MAX_AMPLITUDE, DDS_MIN_OFFSET, DDS_MAX_OFFSET),
	DdsChannel2(2, DDS_MIN_FREQ, DDS_MAX_FREQ, DDS_MIN_AMPLITUDE, DDS_MAX_AMPLITUDE, DDS_MIN_OFFSET, DDS_MAX_OFFSET),
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
	Encoder_Init();
	ADC_init();
	Usart0Init(9600);			// Always init with 9600 baud to output the power on message.
	InitDeviceTimer();
	sei();
	
	DisableDDSTimer();
	Usart0TransmitStr("Power On\r\n");
	
	ADC_startConversion();
	MCP4922_DisableLatching();
	
	ScreenManager.Init();
	
	LoadSettings();
	
	#ifdef SCPI_ENABLED
		SCPI_Init_Device();
	#endif
}

void DeviceClass::DeviceMainLoop()
{	
	UserInputHandler.ProcessInputs();
	
	if(TimeCounter_ScreenRedraw_ms >= SCREEN_REDRAW_DELAY_MS)
	{
		ScreenManager.DrawAll();
		TimeCounter_ScreenRedraw_ms = 0;
	}
	
	if(TimeCounter_AutoSave_ms >= SETTINGS_AUTOSAVE_DELAY_MS)
	{
		TimeCounter_AutoSave_ms = 0;
		if(_settingsChanged)
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
	TimeCounter_ScreenRedraw_ms = 0;
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
		TimeCounter_KeyPolling_ms = 0;
		Keys_t key = KeyPad_GetKeys();
		if(key != KEYNONE)
		{
			UserInputHandler.EnqueueKeyInput(key);
		}
		if(Encoder_IsButtonPressed())
		{
			UserInputHandler.EnqueueKeyInput(KEYOK);
		}
	}
	
	TimeCounter_ScreenRedraw_ms += currentPeriod_ms;		// Screen redraw is handled in DeviceMainLoop()	
	TimeCounter_AutoSave_ms += currentPeriod_ms;			// AutoSave is handled in DeviceMainLoop()
	ScreenManager.DeviceTimerTickISR(currentPeriod_ms);
	PsChannel.DeviceTimerTickISR(currentPeriod_ms);
}

// ##### Device Control State ########################################################################################################

void DeviceClass::SetDeviceControlState(DeviceControlStates_t controlState)
{
	DeviceControlState = controlState;
	ScreenManager.ShowHideDeviceRWLMessage(DeviceControlState == DEV_CTRL_RWLOCK);
}

bool DeviceClass::IsUserInputLocked() 
{
	 return (DeviceControlState != DEV_CTRL_LOCAL); 
}

void DeviceClass::UpdateControlStateOnUserInput()
{
	switch(DeviceControlState)
	{
		case DEV_CTRL_LOCAL: /* Nothing to do here. Device is already in local state. */ break;
		case DEV_CTRL_REMOTE: SetDeviceControlState(DEV_CTRL_LOCAL); break;
		case DEV_CTRL_RWLOCK: /* Nothing to do here. It is only possible to return from state RWLock via SCPI command. */ break;
		default: break;
	}
}

// ##### Communication ###############################################################################################################

uint32_t DeviceBaudRateEnumToNumber(DeviceBaudRates_t baudRateEnum)
{
	switch(baudRateEnum)
	{
		case DEV_BAUD_110: return 110;
		case DEV_BAUD_150: return 150;
		case DEV_BAUD_300: return 300;
		case DEV_BAUD_1200: return 1200;
		case DEV_BAUD_2400: return 2400;
		case DEV_BAUD_4800: return 4800;
		case DEV_BAUD_9600: return 9600;
		case DEV_BAUD_19200: return 19200;
		case DEV_BAUD_38400: return 38400;
		case DEV_BAUD_57600: return 57600;
		default: return 9600;
	}
}

void DeviceClass::SetSerialBaudRate(DeviceBaudRates_t baud)
{
	if(SerialBaudRate != baud)
	{		
		uint32_t baudNum = DeviceBaudRateEnumToNumber(baud);
		
		SetSettingsChanged(true);
		char buffer[60];
		sprintf(buffer, "Changing Baud rate to %lu\r\n", baudNum);
		Usart0TransmitStr(buffer);
		SerialBaudRate = baud;
		Usart0ChangeBaudRate(baudNum);	
	}
}

void DeviceClass::SetSerialEchoEnabled(bool echoEnabled)
{
	SetSettingsChanged(SerialEchoEnabled != echoEnabled);
	SerialEchoEnabled = echoEnabled;
}

// ##### Settings ####################################################################################################################

void DeviceClass::SetSettingsChanged(bool settingsChanged)
{
	_settingsChanged = settingsChanged;
	ScreenManager.UpdateSettingsChangedIndicator(settingsChanged);
}

// -----------------------------------------------------------------------------------------------------------------------------------

void DeviceClass::SaveSettings()
{
	DevSettingsEEPROMLayout_t settings;
	/* Collect setting from appropriate classes */
	settings.Screens_Inverted = ScreenManager.DisplayInverted;
	settings.Device_SerialBaudRate = SerialBaudRate;
	settings.Device_SerialEchoEnabled = SerialEchoEnabled;
	
	settings.PS_Voltage = PsChannel.GetVoltage();
	settings.PS_Current = PsChannel.GetCurrent();
	settings.PS_Enabled = PsChannel.GetEnabled();
	settings.PS_OvpLevel = PsChannel.GetOvpLevel();
	settings.PS_OvpState = PsChannel.GetOvpState();
	settings.PS_OvpDelay = PsChannel.GetOvpDelay();
	settings.PS_OcpLevel = PsChannel.GetOcpLevel();
	settings.PS_OcpState = PsChannel.GetOcpState();
	settings.PS_OcpDelay = PsChannel.GetOcpDelay();
	settings.PS_OppLevel = PsChannel.GetOppLevel();
	settings.PS_OppState = PsChannel.GetOppState();
	settings.PS_OppDelay = PsChannel.GetOppDelay();
		
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

	settings.PowerOnOutputsState = PowerOnOutputsState;

	eeprom_write_block((const void*)&settings, (void*)&NonVolatileSettings, sizeof(DevSettingsEEPROMLayout_t));
	
	SetSettingsChanged(false);
}

void DeviceClass::SaveSettingsCalibrationFactors()
{
	eeprom_write_block((const void*)&CalibrationFactors, (void*)&NonVolatileSettings_CalibrationFactors, sizeof(DeviceCalibrationFactors_t));
}

#ifdef DDS_USER_DEFINED_WAVEFORMS_ENABLED
void DeviceClass::SaveSettingsDDSUserWaveforms()
{
	DevSettingsUserDDSWaveformEEPROMLayout_t settingsDDSUserWaveform;
	memcpy(settingsDDSUserWaveform.DDS1_UserWaveTable, DdsChannel1.UserWaveTable, (1 << DDS_QUANTIZER_BITS) * sizeof(uint16_t));
	memcpy(settingsDDSUserWaveform.DDS2_UserWaveTable, DdsChannel2.UserWaveTable, (1 << DDS_QUANTIZER_BITS) * sizeof(uint16_t));
	
	eeprom_write_block((const void*)&settingsDDSUserWaveform, (void*)&NonVolatileSettings_DDSUserWaveforms, sizeof(DevSettingsUserDDSWaveformEEPROMLayout_t));
}
#endif

// -----------------------------------------------------------------------------------------------------------------------------------

void DeviceClass::LoadSettings()
{
	LoadSettingsCalibrationFactors();
	
	DevSettingsEEPROMLayout_t settings;
	eeprom_read_block((void*)&settings, (const void*)&NonVolatileSettings, sizeof(DevSettingsEEPROMLayout_t));
	
	/* Assign Settings to appropriate classes */

	ScreenManager.SetDisplayInverted(settings.Screens_Inverted);
	
	SetSerialBaudRate(settings.Device_SerialBaudRate);
	SetSerialEchoEnabled(settings.Device_SerialEchoEnabled);
	
	PsChannel.SetVoltage(isnan(settings.PS_Voltage) ? PsChannel.Voltage.Def : settings.PS_Voltage);
	PsChannel.SetCurrent(isnan(settings.PS_Current) ? PsChannel.Current.Def : settings.PS_Current);
	PsChannel.SetOvpLevel(isnan(settings.PS_OvpLevel) ? PsChannel.OvpLevel.Def : settings.PS_OvpLevel);
	PsChannel.SetOvpState(isnan(settings.PS_OvpState) ? false : settings.PS_OvpState);
	PsChannel.SetOvpDelay(isnan(settings.PS_OvpDelay) ? PsChannel.OvpDelay.Def : settings.PS_OvpDelay);
	PsChannel.SetOcpLevel(isnan(settings.PS_OcpLevel) ? PsChannel.OcpLevel.Def : settings.PS_OcpLevel);
	PsChannel.SetOcpState(isnan(settings.PS_OcpState) ? false : settings.PS_OcpState);
	PsChannel.SetOcpDelay(isnan(settings.PS_OcpDelay) ? PsChannel.OcpDelay.Def : settings.PS_OcpDelay);
	PsChannel.SetOppLevel(isnan(settings.PS_OppLevel) ? PsChannel.OppLevel.Def : settings.PS_OppLevel);
	PsChannel.SetOppState(isnan(settings.PS_OppState) ? false : settings.PS_OppState);
	PsChannel.SetOppDelay(isnan(settings.PS_OppDelay) ? PsChannel.OppDelay.Def : settings.PS_OppDelay);
			
	DdsChannel1.SetFrequency(isnan(settings.DDS1_Frequency) ? DdsChannel1.Frequency.Def : settings.DDS1_Frequency);
	DdsChannel1.SetSignalForm(isnan(settings.DDS1_SignalForm) ? DdsChannel1.SignalForm.Def : settings.DDS1_SignalForm);
	DdsChannel1.SetAmplitude(isnan(settings.DDS1_Amplitude) ? DdsChannel1.Amplitude.Def : settings.DDS1_Amplitude);
	DdsChannel1.SetOffset(isnan(settings.DDS1_Offset) ? DdsChannel1.Offset.Def : settings.DDS1_Offset);
	DdsChannel1.UpdateOriginalWaveTable();
	DdsChannel1.UpdateWaveTable();
		
	DdsChannel2.SetFrequency(isnan(settings.DDS2_Frequency) ? DdsChannel2.Frequency.Def : settings.DDS2_Frequency);
	DdsChannel2.SetSignalForm(isnan(settings.DDS2_SignalForm) ? DdsChannel2.SignalForm.Def : settings.DDS2_SignalForm);
	DdsChannel2.SetAmplitude(isnan(settings.DDS2_Amplitude) ? DdsChannel2.Amplitude.Def : settings.DDS2_Amplitude);
	DdsChannel2.SetOffset(isnan(settings.DDS2_Offset) ? DdsChannel2.Offset.Def : settings.DDS2_Offset);
	DdsChannel2.UpdateOriginalWaveTable();
	DdsChannel2.UpdateWaveTable();
	
	PowerOnOutputsState = settings.PowerOnOutputsState;
						
	PsChannel.SetEnabled(PowerOnOutputsState == DEV_POWERUP_OUTPUTS_OFF ? false : (PowerOnOutputsState == DEV_POWERUP_OUTPUTS_ON ? true : settings.PS_Enabled));
	DdsChannel1.SetEnabled(PowerOnOutputsState == DEV_POWERUP_OUTPUTS_OFF ? false : (PowerOnOutputsState == DEV_POWERUP_OUTPUTS_ON ? true : settings.DDS1_Enabled));
	DdsChannel2.SetEnabled(PowerOnOutputsState == DEV_POWERUP_OUTPUTS_OFF ? false : (PowerOnOutputsState == DEV_POWERUP_OUTPUTS_ON ? true : settings.DDS2_Enabled));
	
	SetSettingsChanged(false);
	
	#ifdef DDS_USER_DEFINED_WAVEFORMS_ENABLED
		LoadSettingsDDSUserWaveforms();
	#endif
}

void DeviceClass::LoadSettingsCalibrationFactors()
{
	eeprom_read_block((void*)&CalibrationFactors, (const void*)&NonVolatileSettings_CalibrationFactors, sizeof(DeviceCalibrationFactors_t));
	
	if(CalibrationFactors.Cal_RefVoltage == 0 || isnan(CalibrationFactors.Cal_RefVoltage)) { CalibrationFactors.Cal_RefVoltage = 5; }
	if(CalibrationFactors.Cal_ATX_3V3 == 0 || isnan(CalibrationFactors.Cal_ATX_3V3)) { CalibrationFactors.Cal_ATX_3V3 = 1; }
	if(CalibrationFactors.Cal_ATX_5V == 0 || isnan(CalibrationFactors.Cal_ATX_5V)) { CalibrationFactors.Cal_ATX_5V = 1; }
	if(CalibrationFactors.Cal_ATX_12V == 0 || isnan(CalibrationFactors.Cal_ATX_12V)) { CalibrationFactors.Cal_ATX_12V = 1; }
	if(CalibrationFactors.Cal_ATX_12V_NEG == 0 || isnan(CalibrationFactors.Cal_ATX_12V_NEG)) { CalibrationFactors.Cal_ATX_12V_NEG = 1; }
	if(CalibrationFactors.Cal_DMM1 == 0 || isnan(CalibrationFactors.Cal_DMM1)) { CalibrationFactors.Cal_DMM1 = 1; }
	if(CalibrationFactors.Cal_DMM2 == 0 || isnan(CalibrationFactors.Cal_DMM2)) { CalibrationFactors.Cal_DMM2 = 1; }
	if(CalibrationFactors.Cal_PS_VOLT == 0 || isnan(CalibrationFactors.Cal_PS_VOLT)) { CalibrationFactors.Cal_PS_VOLT = 1; }
	if(CalibrationFactors.Cal_DDS_FREQ == 0 || isnan(CalibrationFactors.Cal_DDS_FREQ)) { CalibrationFactors.Cal_DDS_FREQ = 1; }
}

#ifdef DDS_USER_DEFINED_WAVEFORMS_ENABLED
void DeviceClass::LoadSettingsDDSUserWaveforms()
{
	DevSettingsUserDDSWaveformEEPROMLayout_t settingsDDSUserWaveform;
	eeprom_read_block((void*)&settingsDDSUserWaveform, (const void*)&NonVolatileSettings_DDSUserWaveforms, sizeof(DevSettingsUserDDSWaveformEEPROMLayout_t));
	
	memcpy(DdsChannel1.UserWaveTable, settingsDDSUserWaveform.DDS1_UserWaveTable, (1 << DDS_QUANTIZER_BITS) * sizeof(uint16_t));
	memcpy(DdsChannel2.UserWaveTable, settingsDDSUserWaveform.DDS2_UserWaveTable, (1 << DDS_QUANTIZER_BITS) * sizeof(uint16_t));	
}
#endif

// -----------------------------------------------------------------------------------------------------------------------------------

void DeviceClass::ResetDevice()
{
	ScreenManager.SetDisplayEnabled(true);
	ScreenManager.SetDisplayInverted(false);
	
	PsChannel.SetVoltage(PsChannel.Voltage.Def);
	PsChannel.SetCurrent(PsChannel.Current.Def);
	PsChannel.SetEnabled(false);
	PsChannel.SetOvpLevel(PsChannel.OvpLevel.Def);
	PsChannel.SetOvpState(PsChannel.OvpState.Def);
	PsChannel.SetOvpDelay(PsChannel.OvpDelay.Def);
	PsChannel.SetOcpLevel(PsChannel.OcpLevel.Def);
	PsChannel.SetOcpState(PsChannel.OcpState.Def);
	PsChannel.SetOcpDelay(PsChannel.OcpDelay.Def);	
	PsChannel.SetOppLevel(PsChannel.OppLevel.Def);
	PsChannel.SetOppState(PsChannel.OppState.Def);
	PsChannel.SetOppDelay(PsChannel.OppDelay.Def);
	
	DdsChannel1.SetEnabled(false);
	DdsChannel1.SetFrequency(DdsChannel1.Frequency.Def);
	DdsChannel1.SetSignalForm(DdsChannel1.SignalForm.Def);
	DdsChannel1.SetAmplitude(DdsChannel1.Amplitude.Def);
	DdsChannel1.SetOffset(DdsChannel1.Offset.Def);
	
	DdsChannel2.SetEnabled(false);
	DdsChannel2.SetFrequency(DdsChannel2.Frequency.Def);
	DdsChannel2.SetSignalForm(DdsChannel2.SignalForm.Def);
	DdsChannel2.SetAmplitude(DdsChannel2.Amplitude.Def);
	DdsChannel2.SetOffset(DdsChannel2.Offset.Def);
	
	PowerOnOutputsState = DEV_POWERUP_OUTPUTS_OFF;
	//Calibration factors are not resetted. A new calibration must be done to change the factors.
	
	SaveSettings();
}
