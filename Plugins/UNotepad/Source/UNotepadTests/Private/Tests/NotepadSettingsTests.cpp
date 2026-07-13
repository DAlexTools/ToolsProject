// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/UNotepadSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadSettingsDefaultsTest,
	"UNotepad.Settings.Defaults.ProvideExpectedEditorOptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const UUNotepadSettings* Settings = UUNotepadSettings::Get();
	TestNotNull(TEXT("Default settings should be available"), Settings);
	TestEqual(TEXT("Settings section text"), Settings->GetSectionText().ToString(), FString(TEXT("UNotepad")));
	TestTrue(TEXT("Configured font size should be clamped into supported range"), Settings->GetClampedEditorFontSize() >= 6.0f && Settings->GetClampedEditorFontSize() <= 32.0f);
	TestTrue(TEXT("Configured tab size should be clamped into supported range"), Settings->GetClampedTabSize() >= 1 && Settings->GetClampedTabSize() <= 16);
	TestTrue(TEXT("Default source extensions should include cpp"), Settings->IsSourceFileExtension(TEXT(".CPP")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadSettingsClampTest,
	"UNotepad.Settings.ClampValues.KeepsEditorOptionsInRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadSettingsClampTest::RunTest(const FString& Parameters)
{
	UUNotepadSettings* Settings = NewObject<UUNotepadSettings>(GetTransientPackage());

	Settings->EditorFontSize = 2.0f;
	Settings->TabSize = 0;
	TestEqual(TEXT("Font size should clamp to minimum"), Settings->GetClampedEditorFontSize(), 6.0f);
	TestEqual(TEXT("Tab size should clamp to minimum"), Settings->GetClampedTabSize(), 1);

	Settings->EditorFontSize = 100.0f;
	Settings->TabSize = 100;
	TestEqual(TEXT("Font size should clamp to maximum"), Settings->GetClampedEditorFontSize(), 32.0f);
	TestEqual(TEXT("Tab size should clamp to maximum"), Settings->GetClampedTabSize(), 16);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadSettingsSourceExtensionsTest,
	"UNotepad.Settings.SourceExtensions.NormalizesAndDeduplicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadSettingsSourceExtensionsTest::RunTest(const FString& Parameters)
{
	UUNotepadSettings* Settings = NewObject<UUNotepadSettings>(GetTransientPackage());
	Settings->SourceFileExtensions = {
		TEXT(" .CPP "),
		TEXT(".cpp"),
		TEXT(""),
		TEXT("H")
	};

	const TArray<FString> Extensions = Settings->GetNormalizedSourceFileExtensions();

	TestEqual(TEXT("Only unique non-empty normalized extensions should remain"), Extensions.Num(), 2);
	TestTrue(TEXT("cpp should be normalized"), Extensions.Contains(TEXT("cpp")));
	TestTrue(TEXT("h should be normalized"), Extensions.Contains(TEXT("h")));
	TestTrue(TEXT("IsSourceFileExtension should trim and ignore case"), Settings->IsSourceFileExtension(TEXT(" .CpP ")));
	TestFalse(TEXT("Unknown extension should not match"), Settings->IsSourceFileExtension(TEXT("uasset")));

	return true;
}

#endif
