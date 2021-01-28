/*
 * PowerSupply.cpp
 *
 * Created: 23.09.2020 19:47:47
 *  Author: V17
 */ 

#include "PowerSupply.h"
#include "../Device.h"

PowerSupplyClass PowerSupply;

void PowerSupplyClass::UpdateOutput()
{	
	if(OutputEnabled)
	{
		if(LoadImpedance == 0) { LoadImpedance = PS_MIN_LOAD_IMPEDANCE; }
			
		float voltage = ((LoadImpedance + PS_INTERNAL_IMPEDANCE) / LoadImpedance) * Voltage;				//Vset = ((Rload + Rinternal) / Rload) * Vout		
		MCP4921_Voltage_Set(voltage / 2);		// divided by two because of OpAmp in circuit that has an amplification of 2
	}
	else
	{
		MCP4921_Voltage_Set(0);
	}
}

void PowerSupplyClass::SetVoltage(float voltage)
{
	Device.DevSettingsNeedSaving = (Voltage != voltage);
	Voltage = voltage;
	UpdateOutput();
}
void PowerSupplyClass::SetOutputEnabled(bool outputEnabled)
{
	// This parameter is not saved for security reasons.
	OutputEnabled = outputEnabled;
	UpdateOutput();
}
void PowerSupplyClass::SetLoadImpedance(float loadImpedance)
{
	Device.DevSettingsNeedSaving = (LoadImpedance != loadImpedance);
	LoadImpedance = loadImpedance;
	UpdateOutput();
}

void PSUpdateOutputCallbackFunction()
{
	PowerSupply.UpdateOutput();
}