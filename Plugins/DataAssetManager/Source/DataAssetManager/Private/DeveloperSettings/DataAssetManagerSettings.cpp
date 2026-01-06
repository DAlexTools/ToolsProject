// Fill out your copyright notice in the Description page of Project Settings.


#include "DeveloperSettings/DataAssetManagerSettings.h"

#define LOCTEXT_NAMESPACE "DataAssetManager"

UDataAssetManagerSettings::UDataAssetManagerSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("DataAssetManager");
}

#if WITH_EDITOR
FText UDataAssetManagerSettings::GetSectionText() const
{
	return LOCTEXT("SettingsDisplayName", "DataAssetManager");
}
#endif
#undef LOCTEXT_NAMESPACE