/*
 * SCPI_Cmds_System.cpp
 *
 * Created: 06.12.2020 14:50:38
 *  Author: V17
 */ 

#include "SCPI_Device.h"
#include "../Device.h"

scpi_result_t scpi_cmd_systemLocal(scpi_t * context)
{
	Device.DeviceControlState = DEV_CTRL_LOCAL;
	return SCPI_RES_OK;
}

scpi_result_t scpi_cmd_systemRemote(scpi_t * context)
{
	Device.DeviceControlState = DEV_CTRL_REMOTE;
	return SCPI_RES_OK;	
}

scpi_result_t scpi_cmd_systemRWLock(scpi_t * context)
{
	Device.DeviceControlState = DEV_CTRL_RWLOCK;
	return SCPI_RES_OK;
}