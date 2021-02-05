/*
 * PS_Channel.cpp
 *
 * Created: 23.09.2020 19:47:47
 *  Author: V17
 */ 

#include "PS_Channel.h"
#include "../Device.h"

PS_Channel::PS_Channel(float minAmpl, float maxAmpl, float minLoad, float maxLoad) : Channel(POWER_SUPPLY_CHANNEL_TYPE)
{
	Enabled = Parameter<bool>(false, false, true, false, true);
	Amplitude = Parameter<float>(0, minAmpl, maxAmpl, 0, 1);
	LoadImpedance = Parameter<float>(0, minLoad, maxLoad, 0, 1);
}

void PS_Channel::UpdateOutput()
{
	if(GetEnabled())
	{
		if(GetLoadImpedance() == 0) { LoadImpedance.Val = LoadImpedance.Min; }
		
		float voltage = ((GetLoadImpedance() + PS_INTERNAL_IMPEDANCE) / GetLoadImpedance()) * GetAmplitude();				//Vset = ((Rload + Rinternal) / Rload) * Vout
		MCP4921_Voltage_Set(voltage / 2);		// divided by two because of OpAmp in circuit that has an amplification of 2
	}
	else
	{
		MCP4921_Voltage_Set(0);
	}
}

//----------------------------------------------------------------------------------------------------------

bool PS_Channel::SetEnabled(bool enabled)
{
	if (Enabled.Val != enabled)
	{
		Enabled.Val = enabled;
		PSEnabledChanged(this);
	}
	return true;
}

bool PS_Channel::GetEnabled()
{
	return Enabled.Val;
}

//----------------------------------------------------------------------------------------------------------

bool PS_Channel::SetAmplitude(float amplitude)
{
	if (amplitude > Amplitude.Max || amplitude < Amplitude.Min) { return false; }

	if (Amplitude.Val != amplitude)
	{
		Amplitude.Val = amplitude;
		PSAmplitudeChanged(this);
	}
	return true;
}

float PS_Channel::GetAmplitude()
{
	return Amplitude.Val;
}

//----------------------------------------------------------------------------------------------------------

bool PS_Channel::SetLoadImpedance(float loadImpedance)
{
	if (loadImpedance > LoadImpedance.Max || loadImpedance < LoadImpedance.Min) { return false; }

	if (LoadImpedance.Val != loadImpedance)
	{
		LoadImpedance.Val = loadImpedance;
		PSLoadImpedanceChanged(this);
	}
	return true;
}

float PS_Channel::GetLoadImpedance()
{
	return LoadImpedance.Val;
}

//----------------------------------------------------------------------------------------------------------

void PS_Channel::PSEnabledChanged(void* channel)
{
	if (((Channel*)channel)->GetChannelType() != POWER_SUPPLY_CHANNEL_TYPE) { return; }
	PS_Channel* psChannel = (PS_Channel*)channel;
	psChannel->UpdateOutput();
	Device.DevSettingsNeedSaving = true;
}

void PS_Channel::PSAmplitudeChanged(void* channel)
{
	if (((Channel*)channel)->GetChannelType() != POWER_SUPPLY_CHANNEL_TYPE) { return; }
	PS_Channel* psChannel = (PS_Channel*)channel;
	psChannel->UpdateOutput();
	Device.DevSettingsNeedSaving = true;
}

void PS_Channel::PSLoadImpedanceChanged(void* channel)
{
	if (((Channel*)channel)->GetChannelType() != POWER_SUPPLY_CHANNEL_TYPE) { return; }
	PS_Channel* psChannel = (PS_Channel*)channel;
	psChannel->UpdateOutput();
	Device.DevSettingsNeedSaving = true;
}