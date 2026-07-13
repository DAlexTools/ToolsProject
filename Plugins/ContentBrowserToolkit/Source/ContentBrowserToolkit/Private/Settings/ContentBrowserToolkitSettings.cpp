#include "Settings/ContentBrowserToolkitSettings.h"

const UContentBrowserToolkitSettings* UContentBrowserToolkitSettings::Get()
{
	return GetDefault<UContentBrowserToolkitSettings>();
}

UContentBrowserToolkitSettings* UContentBrowserToolkitSettings::GetMutable()
{
	return GetMutableDefault<UContentBrowserToolkitSettings>();
}
