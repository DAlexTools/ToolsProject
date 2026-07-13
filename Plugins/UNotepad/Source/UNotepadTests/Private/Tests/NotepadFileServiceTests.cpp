// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/NotepadFileService.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadFileServiceSaveLoadTest,
	"UNotepad.FileService.SaveLoad.RoundTripsUtf8Text",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadFileServiceSaveLoadTest::RunTest(const FString& Parameters)
{
	const FString TestRoot = FPaths::ProjectSavedDir() / TEXT("UNotepadTests") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString TestFile = TestRoot / TEXT("RoundTrip.txt");
	IFileManager::Get().MakeDirectory(*TestRoot, true);

	FString Error;
	const FString OriginalText = TEXT("Alpha\nBeta\n");
	TestTrue(TEXT("SaveStringToFile should write the file"), FNotepadFileService::SaveStringToFile(TestFile, OriginalText, Error));

	FString LoadedText;
	TestTrue(TEXT("LoadFileToString should read the saved file"), FNotepadFileService::LoadFileToString(TestFile, LoadedText, Error));
	TestEqual(TEXT("Loaded text should match saved text"), LoadedText, OriginalText);

	IFileManager::Get().DeleteDirectory(*TestRoot, false, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadFileServiceMissingFileTest,
	"UNotepad.FileService.LoadFileToString.ReportsMissingFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadFileServiceMissingFileTest::RunTest(const FString& Parameters)
{
	const FString MissingFile = FPaths::ProjectSavedDir() / TEXT("UNotepadTests") / FGuid::NewGuid().ToString(EGuidFormats::Digits) / TEXT("Missing.txt");
	FString LoadedText;
	FString Error;

	TestFalse(TEXT("Missing file should fail to load"), FNotepadFileService::LoadFileToString(MissingFile, LoadedText, Error));
	TestTrue(TEXT("Missing file error should include read failure"), Error.StartsWith(TEXT("Failed to read")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadFileServiceNormalizePathTest,
	"UNotepad.FileService.NormalizeFilePath.ReturnsFullNormalizedPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadFileServiceNormalizePathTest::RunTest(const FString& Parameters)
{
	const FString NormalizedPath = FNotepadFileService::NormalizeFilePath(TEXT("Saved\\UNotepadTests\\..\\UNotepadTests\\File.txt"));

	TestTrue(TEXT("Normalized path should be absolute"), FPaths::IsRelative(NormalizedPath) == false);
	TestFalse(TEXT("Normalized path should not contain backslashes"), NormalizedPath.Contains(TEXT("\\")));
	TestTrue(TEXT("Normalized path should end with requested file"), NormalizedPath.EndsWith(TEXT("Saved/UNotepadTests/File.txt")));

	return true;
}

#endif
