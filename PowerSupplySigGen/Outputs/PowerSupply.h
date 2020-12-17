/*
 * PowerSupply.h
 *
 * Created: 23.09.2020 19:47:56
 *  Author: V17
 */ 


#ifndef POWERSUPPLY_H_
#define POWERSUPPLY_H_

#include "DAC_MCP492x.h"
#include "../Configuration.h"

class PowerSupplyClass
{
	public:
		float Voltage;
		bool OutputEnabled;
		float LoadImpedance;
		
		PowerSupplyClass() 
		{}
			
		void UpdateOutput();
		void SetVoltage(float voltage);
		void SetOutputEnabled(bool outputEnabled);
		void SetLoadImpedance(float loadImpedance);
};

extern PowerSupplyClass PowerSupply;

void PSUpdateOutputCallbackFunction();

#endif /* POWERSUPPLY_H_ */