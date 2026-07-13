// Copyright Epic Games, Inc. All Rights Reserved.

#include "UNotepad.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadModuleSmokeTest,
	"UNotepad.Module.Smoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadModuleSmokeTest::RunTest(const FString& Parameters)
{
	IUNotepadModule* Module = FModuleManager::LoadModulePtr<IUNotepadModule>(TEXT("UNotepad"));

	TestNotNull(TEXT("UNotepad module should be loadable"), Module);
	TestEqual(TEXT("Nomad tab name should stay stable"), FUNotepadModule::UNotepadTabName, FName(TEXT("UNotepad")));

	return true;
}

#endif
