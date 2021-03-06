// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Misc/OutputDevice.h"

#include <WorkerSDK/improbable/c_worker.h>

class USpatialWorkerConnection;

class SPATIALGDK_API FSpatialOutputDevice : public FOutputDevice
{
public:
	FSpatialOutputDevice(USpatialWorkerConnection* InConnection, FName LoggerName, int32 InPIEIndex);
	~FSpatialOutputDevice();

	void AddRedirectCategory(const FName& Category);
	void RemoveRedirectCategory(const FName& Category);
	void SetVerbosityFilterLevel(ELogVerbosity::Type Verbosity);
	void Serialize(const TCHAR* InData, ELogVerbosity::Type Verbosity, const FName& Category) override;

	static Worker_LogLevel ConvertLogLevelToSpatial(ELogVerbosity::Type Verbosity);

protected:
	ELogVerbosity::Type FilterLevel;
	TSet<FName> CategoriesToRedirect;
	USpatialWorkerConnection* Connection;
	FName WorkerName;

	int32 PIEIndex;
	bool bLogToSpatial;
};
