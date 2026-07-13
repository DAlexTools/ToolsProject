// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/NotepadDocumentUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDocumentUtilsDetectModeTest,
	"UNotepad.DocumentUtils.DetectModeFromPath.MapsSupportedExtensions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDocumentUtilsDetectModeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("json should map to JSON mode"), FNotepadDocumentUtils::DetectModeFromPath(TEXT("Data.JSON")), EUNotepadDocumentMode::Json);
	TestEqual(TEXT("uplugin should map to JSON mode"), FNotepadDocumentUtils::DetectModeFromPath(TEXT("Plugin.uplugin")), EUNotepadDocumentMode::Json);
	TestEqual(TEXT("csv should map to CSV mode"), FNotepadDocumentUtils::DetectModeFromPath(TEXT("Table.csv")), EUNotepadDocumentMode::Csv);
	TestEqual(TEXT("tsv should map to CSV mode"), FNotepadDocumentUtils::DetectModeFromPath(TEXT("Table.tsv")), EUNotepadDocumentMode::Csv);
	TestEqual(TEXT("md should map to Text mode"), FNotepadDocumentUtils::DetectModeFromPath(TEXT("Readme.md")), EUNotepadDocumentMode::Text);
	TestEqual(TEXT("unknown extensions should map to Code mode"), FNotepadDocumentUtils::DetectModeFromPath(TEXT("Config.ini")), EUNotepadDocumentMode::Code);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDocumentUtilsLabelsAndExtensionsTest,
	"UNotepad.DocumentUtils.LabelsAndExtensions.ReturnExpectedValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDocumentUtilsLabelsAndExtensionsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Text default extension"), FNotepadDocumentUtils::GetDefaultExtension(EUNotepadDocumentMode::Text), FString(TEXT("txt")));
	TestEqual(TEXT("Code default extension"), FNotepadDocumentUtils::GetDefaultExtension(EUNotepadDocumentMode::Code), FString(TEXT("cpp")));
	TestEqual(TEXT("JSON default extension"), FNotepadDocumentUtils::GetDefaultExtension(EUNotepadDocumentMode::Json), FString(TEXT("json")));
	TestEqual(TEXT("CSV default extension"), FNotepadDocumentUtils::GetDefaultExtension(EUNotepadDocumentMode::Csv), FString(TEXT("csv")));

	TestEqual(TEXT("Text mode label"), FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Text), FString(TEXT("Text")));
	TestEqual(TEXT("Code mode label"), FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Code), FString(TEXT("Code")));
	TestEqual(TEXT("JSON mode label"), FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Json), FString(TEXT("JSON")));
	TestEqual(TEXT("CSV mode label"), FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Csv), FString(TEXT("CSV")));

	TestEqual(TEXT("Untitled names should include index and mode extension"), FNotepadDocumentUtils::GetUntitledName(EUNotepadDocumentMode::Json, 7), FString(TEXT("Untitled-7.json")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDocumentUtilsDisplayAndLineNumbersTest,
	"UNotepad.DocumentUtils.DisplayAndLineNumbers.HandleEdgeCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDocumentUtilsDisplayAndLineNumbersTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Display name should use clean filename"), FNotepadDocumentUtils::MakeDisplayName(TEXT("C:/Temp/Test.cpp")), FString(TEXT("Test.cpp")));
	TestEqual(TEXT("Empty display name should fall back to Untitled"), FNotepadDocumentUtils::MakeDisplayName(TEXT("")), FString(TEXT("Untitled")));

	TestEqual(TEXT("Empty text has one line"), FNotepadDocumentUtils::CountLines(TEXT("")), 1);
	TestEqual(TEXT("Single line text has one line"), FNotepadDocumentUtils::CountLines(TEXT("A")), 1);
	TestEqual(TEXT("LF should increment line count"), FNotepadDocumentUtils::CountLines(TEXT("A\nB\n")), 3);
	TestEqual(TEXT("CRLF should increment line count once per LF"), FNotepadDocumentUtils::CountLines(TEXT("A\r\nB")), 2);

	TestEqual(TEXT("Line number text should clamp to at least one line"), FNotepadDocumentUtils::BuildLineNumberText(0), FString(TEXT("1")));
	TestEqual(TEXT("Line number text should list all lines"), FNotepadDocumentUtils::BuildLineNumberText(3), FString::Printf(TEXT("1%s2%s3"), LINE_TERMINATOR, LINE_TERMINATOR));

	return true;
}

#endif
