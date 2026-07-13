// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/OutlinerToolkitSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutlinerToolkitSettingsConfigPropertiesTest,
	"OutlinerToolkit.Settings.ConfigProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOutlinerToolkitSettingsConfigPropertiesTest::RunTest(const FString& Parameters)
{
	const UClass* const SettingsClass = UOutlinerToolkitSettings::StaticClass();
	TestNotNull(TEXT("Settings class should exist"), SettingsClass);
	if (!SettingsClass)
	{
		return false;
	}

	TestTrue(TEXT("Settings class should be config-backed"), SettingsClass->HasAnyClassFlags(CLASS_Config));

	const UOutlinerToolkitSettings* const Settings = GetDefault<UOutlinerToolkitSettings>();
	TestNotNull(TEXT("Settings default object should exist"), Settings);
	if (!Settings)
	{
		return false;
	}

	const FIntProperty* const MaterialThresholdProperty = FindFProperty<FIntProperty>(
		SettingsClass,
		GET_MEMBER_NAME_CHECKED(UOutlinerToolkitSettings, TooManyMaterialSlotsThreshold));
	const FIntProperty* const ComponentThresholdProperty = FindFProperty<FIntProperty>(
		SettingsClass,
		GET_MEMBER_NAME_CHECKED(UOutlinerToolkitSettings, TooManyComponentsThreshold));

	TestNotNull(TEXT("Material slot threshold property should exist"), MaterialThresholdProperty);
	TestNotNull(TEXT("Component threshold property should exist"), ComponentThresholdProperty);

	if (MaterialThresholdProperty)
	{
		TestTrue(TEXT("Material slot threshold should be stored in config"), MaterialThresholdProperty->HasAnyPropertyFlags(CPF_Config));
	}

	if (ComponentThresholdProperty)
	{
		TestTrue(TEXT("Component threshold should be stored in config"), ComponentThresholdProperty->HasAnyPropertyFlags(CPF_Config));
	}

	TestTrue(TEXT("Material slot threshold should not be negative"), Settings->TooManyMaterialSlotsThreshold >= 0);
	TestTrue(TEXT("Component threshold should not be negative"), Settings->TooManyComponentsThreshold >= 0);

	return true;
}

#endif
