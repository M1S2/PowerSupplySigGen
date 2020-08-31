/*
 * ADC_MCP492x.cpp
 *
 * Created: 31.08.2020 18:42:53
 *  Author: V17
 * see: https://github.com/murjay/MCP4922-DAC-Driver/blob/master/MCP4922_DAC_driver.c
 */ 

#include "ADC_MCP492x.h"
#include "../Spi/spi.h"

static int MCP4921_gain;
static int MCP4922_gain;
static double MCP4921_ref_voltage;
static double MCP4922_ref_voltage;

void MCP4921_init(int gain, double ref_voltage)
{
	MCP4921_gain = gain;
	MCP4921_ref_voltage = ref_voltage;
}

void MCP4922_init(int gain, double ref_voltage)
{
	MCP4922_gain = gain;
	MCP4922_ref_voltage = ref_voltage;
}

void MCP4921_DAC_Set(uint16_t dac_data)
{
	SELECT_MCP4921
		
	uint8_t low_byte=0, high_byte=0;
	high_byte |= (1 << MCP492X_SHDN);					/*Set SHDN bit high for DAC A active operation*/
	high_byte |= (1 << MCP492X_BUFFERED);				/*Enable buffered inputs for Vref*/
	if(MCP4921_gain == 1)
	{
		high_byte |= (1 << MCP492X_GAIN_SELECT_SINGLE);	/*Select single gain*/
	}

	high_byte |= ((dac_data >> 8) & 0x0F);
	low_byte |= dac_data;

	/*send the word*/
	SPI_SendByte(high_byte);
	SPI_SendByte(low_byte);

	DESELECT_MCP4921
}

void MCP4921_Voltage_Set(double voltage)
{
	//VOUT = (GAIN * VREF * D/4096)
	MCP4921_DAC_Set((voltage * 4095) / (MCP4921_ref_voltage * MCP4921_gain));
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
	if(MCP4922_gain == 1)
	{ 
		high_byte |= (1 << MCP492X_GAIN_SELECT_SINGLE);	/*Select single gain*/
	}

	high_byte |= ((dac_data >> 8) & 0x0F);
	low_byte |= dac_data;

	/*send the word*/
	SPI_SendByte(high_byte);
	SPI_SendByte(low_byte);

	DESELECT_MCP4922
}

void MCP4922_Voltage_Set(double voltage, char channel_A_B)
{
	//VOUT = (GAIN * VREF * D/4096)
	MCP4922_DAC_Set((voltage * 4095) / (MCP4922_ref_voltage * MCP4922_gain), channel_A_B);
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