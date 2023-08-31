#pragma once
#include "PrintUtils.hpp"
#include <BLK360G2.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

struct PreConnect
{
	char name[127 + 1];
	char serialNumber[31 + 1];
	Blk360G2_percent_t percentage;
};

struct BlkConfig
{
	Blk360G2_DeviceInfo deviceInfo;
	Blk360G2_DeviceStatus deviceStatus;
	BlkConfig(Blk360G2_DeviceInfo myDecviceInfo, Blk360G2_DeviceStatus myDeviceStatus)
		:deviceInfo(myDecviceInfo), deviceStatus(myDeviceStatus) {}
};

class OleBLK
{
private:
	Blk360G2_SessionHandle session{ Blk360G2_Handle_Null() };
	Blk360G2_EventQueueHandle queue{ Blk360G2_Handle_Null() };
	Blk360G2_SetupHandle setupToDownload{ Blk360G2_Handle_Null() };
	Blk360G2_ProcessingWorkflowHandle processingWorkflow{ Blk360G2_Handle_Null() };
	Blk360G2_DeviceConfigWorkflowHandle deviceConfigWorkflow{ Blk360G2_Handle_Null() };
	Blk360G2_MeasurementWorkflowHandle measurementWorkflow{ Blk360G2_Handle_Null() };
	Blk360G2_DataManipulationWorkflowHandle dataManipulationWorkflow{ Blk360G2_Handle_Null() };
	Blk360G2_SetupEnumeratorHandle setupsEnumerator{ Blk360G2_Handle_Null() };
	void checkError();
	void cleanup();
public:
	OleBLK() {};
	int ConnectBLK(const char* ip);
	BlkConfig GetDeviveInfo();
	void DoScan();
	void manipulateSetupMetadata(const Blk360G2_SetupHandle& setupHandle, Blk360G2_SetupMetadata& metadata);
	void listSetups();
	Blk360G2_UUID doScan();
};
