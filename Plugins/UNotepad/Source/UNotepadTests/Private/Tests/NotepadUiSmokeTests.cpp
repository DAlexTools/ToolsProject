// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SNotepadEditor.h"
#include "UI/SNotepadTabStrip.h"
#include "UI/SNotepadToolbar.h"
#include "UI/SNotepadWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/Application/SlateApplication.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadEditorSmokeTest,
	"UNotepad.UI.Editor.Smoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadEditorSmokeTest::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate is not initialized; skipping SNotepadEditor smoke test."));
		return true;
	}

	TSharedRef<SNotepadEditor> Editor = SNew(SNotepadEditor);
	Editor->SetText(TEXT("A\nB"));
	Editor->SetMode(EUNotepadDocumentMode::Json);
	Editor->SetShowLineNumbers(false);
	Editor->SetShowWhitespace(false);
	Editor->SetTabSize(100);

	TestEqual(TEXT("Editor should return the text it was given"), Editor->GetText(), FString(TEXT("A\nB")));
	TestEqual(TEXT("Editor should count document lines"), Editor->GetLineCount(), 2);
	TestFalse(TEXT("Line numbers should be disabled"), Editor->IsShowingLineNumbers());
	TestFalse(TEXT("Whitespace markers should be disabled"), Editor->IsShowingWhitespace());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadToolbarAndTabStripSmokeTest,
	"UNotepad.UI.ToolbarAndTabStrip.Smoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadToolbarAndTabStripSmokeTest::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate is not initialized; skipping SNotepadToolbar/SNotepadTabStrip smoke test."));
		return true;
	}

	TSharedRef<SNotepadToolbar> Toolbar = SNew(SNotepadToolbar);
	Toolbar->SetMode(EUNotepadDocumentMode::Csv);
	Toolbar->SetLineNumbersEnabled(false);
	Toolbar->SetWhitespaceEnabled(false);

	TSharedRef<FNotepadDocument> FirstDocument = MakeShared<FNotepadDocument>();
	FirstDocument->DisplayName = TEXT("First.txt");

	TSharedRef<FNotepadDocument> SecondDocument = MakeShared<FNotepadDocument>();
	SecondDocument->DisplayName = TEXT("Second.cpp");
	SecondDocument->bDirty = true;

	TArray<TSharedPtr<FNotepadDocument>> Documents;
	Documents.Add(FirstDocument);
	Documents.Add(SecondDocument);

	TSharedRef<SNotepadTabStrip> TabStrip = SNew(SNotepadTabStrip);
	TabStrip->SetDocuments(Documents, SecondDocument);

	TestTrue(TEXT("Toolbar should remain valid after public state changes"), StaticCastSharedRef<SWidget>(Toolbar).Get().GetVisibility().IsVisible());
	TestTrue(TEXT("Tab strip should remain valid after document rebuild"), StaticCastSharedRef<SWidget>(TabStrip).Get().GetVisibility().IsVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadWidgetSmokeTest,
	"UNotepad.UI.Widget.Smoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadWidgetSmokeTest::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized())
	{
		AddWarning(TEXT("Slate is not initialized; skipping SNotepadWidget smoke test."));
		return true;
	}

	TSharedRef<SNotepadWidget> Widget = SNew(SNotepadWidget);
	TestTrue(TEXT("Main widget should support keyboard focus"), Widget->SupportsKeyboardFocus());

	return true;
}

#endif
