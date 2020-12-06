/*
 * Device.cpp
 *
 * Created: 06.12.2020 14:44:51
 *  Author: V17
 */ 

#include "Device.h"

DeviceClass Device;

bool DeviceClass::IsUserInputLocked() 
{
	 return (DeviceControlState != DEV_CTRL_LOCAL); 
}

void DeviceClass::UpdateControlStateOnUserInput()
{
	switch(DeviceControlState)
	{
		case DEV_CTRL_LOCAL: /* Nothing to do here. Device is already in local state. */ break;
		case DEV_CTRL_REMOTE: DeviceControlState = DEV_CTRL_LOCAL; break;
		case DEV_CTRL_RWLOCK: /* Nothing to do here. It is only possible to return from state RWLock via SCPI command. */ break;
	}
}