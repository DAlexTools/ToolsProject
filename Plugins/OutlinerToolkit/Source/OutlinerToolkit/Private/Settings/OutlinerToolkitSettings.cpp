// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Settings/OutlinerToolkitSettings.h"

const UOutlinerToolkitSettings* UOutlinerToolkitSettings::Get()
{
	return GetDefault<UOutlinerToolkitSettings>();
}

UOutlinerToolkitSettings* UOutlinerToolkitSettings::GetMutable()
{
	return GetMutableDefault<UOutlinerToolkitSettings>();
}
