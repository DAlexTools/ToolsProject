// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/NotepadFormatService.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadFormatServiceJsonTest,
	"UNotepad.FormatService.Json.FormatsAndValidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadFormatServiceJsonTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("{\"enabled\":true,\"values\":[1,2]}");
	FString Error;

	TestTrue(TEXT("Valid JSON should format"), FNotepadFormatService::Format(EUNotepadDocumentMode::Json, Text, Error));
	TestTrue(TEXT("Formatted JSON should contain new lines"), Text.Contains(LINE_TERMINATOR));
	TestTrue(TEXT("Formatted JSON should keep boolean literal"), Text.Contains(TEXT("\"enabled\": true")));
	TestEqual(TEXT("Formatted JSON should validate"), FNotepadFormatService::Validate(EUNotepadDocumentMode::Json, Text, Error), ENotepadValidationResult::Valid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadFormatServiceInvalidJsonTest,
	"UNotepad.FormatService.Json.ReportsInvalidInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadFormatServiceInvalidJsonTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("{\"enabled\":");
	FString Error;

	TestFalse(TEXT("Invalid JSON should not format"), FNotepadFormatService::Format(EUNotepadDocumentMode::Json, Text, Error));
	TestTrue(TEXT("Invalid JSON should explain the error"), Error.StartsWith(TEXT("Invalid JSON:")));
	TestEqual(TEXT("Invalid JSON should validate as invalid"), FNotepadFormatService::Validate(EUNotepadDocumentMode::Json, Text, Error), ENotepadValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadFormatServiceCsvTest,
	"UNotepad.FormatService.Csv.FormatsAndValidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadFormatServiceCsvTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("Name,Note\nAlice,\"Hello, world\"\nBob,\"He said \"\"Hi\"\"\"");
	FString Error;

	TestTrue(TEXT("Valid CSV should format"), FNotepadFormatService::Format(EUNotepadDocumentMode::Csv, Text, Error));
	TestEqual(TEXT("Formatted CSV should preserve escaped cells"), Text, FString::Printf(TEXT("Name,Note%sAlice,\"Hello, world\"%sBob,\"He said \"\"Hi\"\"\""), LINE_TERMINATOR, LINE_TERMINATOR));
	TestEqual(TEXT("Formatted CSV should validate"), FNotepadFormatService::Validate(EUNotepadDocumentMode::Csv, Text, Error), ENotepadValidationResult::Valid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadFormatServiceInvalidCsvTest,
	"UNotepad.FormatService.Csv.ReportsInvalidInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadFormatServiceInvalidCsvTest::RunTest(const FString& Parameters)
{
	FString Error;

	TestEqual(TEXT("Mismatched columns should be invalid"), FNotepadFormatService::Validate(EUNotepadDocumentMode::Csv, TEXT("A,B\nC"), Error), ENotepadValidationResult::Invalid);
	TestTrue(TEXT("Mismatched column error should mention columns"), Error.Contains(TEXT("columns")));

	Error.Reset();
	TestEqual(TEXT("Unterminated quotes should be invalid"), FNotepadFormatService::Validate(EUNotepadDocumentMode::Csv, TEXT("A,\"B"), Error), ENotepadValidationResult::Invalid);
	TestTrue(TEXT("Unterminated quote error should mention quoted field"), Error.Contains(TEXT("quoted field")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadFormatServiceUnsupportedModeTest,
	"UNotepad.FormatService.UnsupportedMode.ReportsNotSupported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadFormatServiceUnsupportedModeTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("Plain text");
	FString Error;

	TestFalse(TEXT("Text mode should not format"), FNotepadFormatService::Format(EUNotepadDocumentMode::Text, Text, Error));
	TestEqual(TEXT("Text mode validation should be not supported"), FNotepadFormatService::Validate(EUNotepadDocumentMode::Text, Text, Error), ENotepadValidationResult::NotSupported);

	return true;
}

#endif
