/*
 * SCPI_Cmds_System.cpp
 *
 * Created: 06.12.2020 14:50:38
 *  Author: V17
 */ 

#include "SCPI_Device.h"
#include "../Device.h"
#include "../Screens/ScreenManager.h"

scpi_result_t scpi_cmd_systemLocal(scpi_t * context)
{
	Device.DeviceControlState = DEV_CTRL_LOCAL;
	strcpy(ScreenManager.SystemMessage, "");
	return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_systemRemote(scpi_t * context)
{
	Device.DeviceControlState = DEV_CTRL_REMOTE;
	strcpy(ScreenManager.SystemMessage, "");
	return SCPI_RES_OK;	
}

scpi_result_t scpi_cmd_systemRWLock(scpi_t * context)
{
	Device.DeviceControlState = DEV_CTRL_RWLOCK;
	strcpy(ScreenManager.SystemMessage, "==== Remote controlled ====\nDevice locked by SYST:RWL command.\nOnly possible to unlock via SYST:LOC command.");
	return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_systemCommunicateSerialBaud(scpi_t * context)
{
	uint32_t baud;
	if(!SCPI_ParamUInt32(context, &baud, TRUE))
	{
		return SCPI_RES_ERR;
	}
	Device.SetBaudRate(baud);
	
	return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_systemCommunicateSerialBaudQ(scpi_t * context)
{
	SCPI_ResultInt(context, Device.BaudRate);
	return SCPI_RES_OK;
}