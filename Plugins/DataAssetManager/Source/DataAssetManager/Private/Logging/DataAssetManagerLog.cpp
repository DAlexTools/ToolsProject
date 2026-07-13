// Copyright (c) 2026 DimAlek. All Rights Reserved.




#include "Logging/DataAssetManagerLog.h"

DEFINE_LOG_CATEGORY(SDataAssetManagerLog);

TAutoConsoleVariable<bool> CVarDebugDataAssetManager(
	TEXT("ShowDebugDataAssetManager"),
	false,
	TEXT("Enables verbose debug logging for the Data Asset Manager plugin."));
