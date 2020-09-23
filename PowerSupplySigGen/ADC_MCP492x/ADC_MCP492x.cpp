/*
 * ADC_MCP492x.cpp
 *
 * Created: 31.08.2020 18:42:53
 *  Author: V17
 * see: https://github.com/murjay/MCP4922-DAC-Driver/blob/master/MCP4922_DAC_driver.c
 */ 

#include "ADC_MCP492x.h"
#include "../Spi/spi.h"
#include "../Global/DevSettings.h"

void MCP4921_DAC_Set(uint16_t dac_data)
{
	SELECT_MCP4921
		
	uint8_t low_byte=0, high_byte=0;
	high_byte |= (1 << MCP492X_SHDN);					/*Set SHDN bit high for DAC A active operation*/
	high_byte |= (1 << MCP492X_BUFFERED);				/*Enable buffered inputs for Vref*/
	high_byte |= (1 << MCP492X_GAIN_SELECT_SINGLE);		/*Select single gain*/

	high_byte |= ((dac_data >> 8) & 0x0F);
	low_byte |= dac_data;

	/*send the word*/
	SPI_SendByte(high_byte);
	SPI_SendByte(low_byte);

	DESELECT_MCP4921
}

void MCP4921_Voltage_Set(int voltage_mV)
{
	//VOUT = (GAIN * VREF * D/4096)
	MCP4921_DAC_Set((uint16_t)(voltage_mV * (4095.0f / AVR_VCC_MV)));
}

void PS_Output_Set()
{
	if(DevSettings.PS_Output_Enabled)
	{
		MCP4921_Voltage_Set(DevSettings.PS_Voltage_mV / 2);
	}
	else
	{
		MCP4921_Voltage_Set(0);
	}
}

void MCP4922_DAC_Set(uint16_t dac_data, char channel_A_B)
{	
	SELECT_MCP4922
		
	uint8_t low_byte=0, high_byte=0;
	if(channel_A_B == 'B')
	{
		high_byte |= (1 << MCP492X_DACB);				/*Set bit to select DAC B*/
	}
	high_byte |= (1 << MCP492X_SHDN);					/*Set SHDN bit high for DAC A active operation*/
	high_byte |= (1 << MCP492X_BUFFERED);				/*Enable buffered inputs for Vref*/
	high_byte |= (1 << MCP492X_GAIN_SELECT_SINGLE);		/*Select single gain*/

	high_byte |= ((dac_data >> 8) & 0x0F);
	low_byte |= dac_data;

	/*send the word*/
	SPI_SendByte(high_byte);
	SPI_SendByte(low_byte);

	DESELECT_MCP4922
}

void MCP4922_Voltage_Set(int voltage_mV, char channel_A_B)
{
	//VOUT = (GAIN * VREF * D/4096)
	voltage_mV = (voltage_mV + 10000) / 4;
	MCP4922_DAC_Set((uint16_t)(voltage_mV * (4095.0f / AVR_VCC_MV)), channel_A_B);
}

void MCP4922_DisableLatching()
{
	CLEAR_LDAC_MCP4922
}

void MCP4922_LatchOutput()
{
	CLEAR_LDAC_MCP4922
	SET_LDAC_MCP4922
}