// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/NotepadDocumentHistoryService.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

namespace
{
	TSharedRef<FNotepadDocument> MakeHistoryTestDocument(const FString& Content)
	{
		TSharedRef<FNotepadDocument> Document = MakeShared<FNotepadDocument>();
		Document->Content = Content;
		Document->SavedContent = Content;
		Document->bDirty = false;
		return Document;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDocumentHistorySetContentRecordsUndoTest,
	"UNotepad.DocumentHistory.SetContent.RecordsUndoAndDirtyState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDocumentHistorySetContentRecordsUndoTest::RunTest(const FString& Parameters)
{
	TSharedRef<FNotepadDocument> Document = MakeHistoryTestDocument(TEXT("Saved"));

	TestTrue(TEXT("Changing content should return true"), FNotepadDocumentHistoryService::SetContent(Document, TEXT("Changed"), true));
	TestEqual(TEXT("Content should be replaced"), Document->Content, FString(TEXT("Changed")));
	TestTrue(TEXT("Document should become dirty"), Document->bDirty);
	TestEqual(TEXT("Previous content should be on undo stack"), Document->UndoStack.Num(), 1);
	TestEqual(TEXT("Undo entry should be saved content"), Document->UndoStack[0], FString(TEXT("Saved")));

	TestFalse(TEXT("Setting identical content should no-op"), FNotepadDocumentHistoryService::SetContent(Document, TEXT("Changed"), true));
	TestEqual(TEXT("No duplicate undo entry should be added"), Document->UndoStack.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDocumentHistoryUndoRedoRoundTripTest,
	"UNotepad.DocumentHistory.UndoRedo.RestoresContentAndDirtyState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDocumentHistoryUndoRedoRoundTripTest::RunTest(const FString& Parameters)
{
	TSharedRef<FNotepadDocument> Document = MakeHistoryTestDocument(TEXT("A"));

	FNotepadDocumentHistoryService::SetContent(Document, TEXT("B"), true);
	FNotepadDocumentHistoryService::SetContent(Document, TEXT("C"), true);

	TestTrue(TEXT("Undo should restore B"), FNotepadDocumentHistoryService::Undo(Document));
	TestEqual(TEXT("First undo content"), Document->Content, FString(TEXT("B")));
	TestTrue(TEXT("Document should still be dirty after first undo"), Document->bDirty);
	TestTrue(TEXT("Redo should be available after undo"), FNotepadDocumentHistoryService::CanRedo(Document));

	TestTrue(TEXT("Undo should restore saved A"), FNotepadDocumentHistoryService::Undo(Document));
	TestEqual(TEXT("Second undo content"), Document->Content, FString(TEXT("A")));
	TestFalse(TEXT("Document should not be dirty when content equals saved content"), Document->bDirty);

	TestTrue(TEXT("Redo should restore B"), FNotepadDocumentHistoryService::Redo(Document));
	TestEqual(TEXT("First redo content"), Document->Content, FString(TEXT("B")));
	TestTrue(TEXT("Document should be dirty after redo"), Document->bDirty);

	TestTrue(TEXT("Redo should restore C"), FNotepadDocumentHistoryService::Redo(Document));
	TestEqual(TEXT("Second redo content"), Document->Content, FString(TEXT("C")));
	TestFalse(TEXT("Redo should be exhausted"), FNotepadDocumentHistoryService::CanRedo(Document));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDocumentHistoryNewEditClearsRedoTest,
	"UNotepad.DocumentHistory.SetContent.NewEditClearsRedo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDocumentHistoryNewEditClearsRedoTest::RunTest(const FString& Parameters)
{
	TSharedRef<FNotepadDocument> Document = MakeHistoryTestDocument(TEXT("A"));

	FNotepadDocumentHistoryService::SetContent(Document, TEXT("B"), true);
	FNotepadDocumentHistoryService::SetContent(Document, TEXT("C"), true);
	FNotepadDocumentHistoryService::Undo(Document);

	TestTrue(TEXT("Redo should exist before a new edit"), FNotepadDocumentHistoryService::CanRedo(Document));

	FNotepadDocumentHistoryService::SetContent(Document, TEXT("D"), true);

	TestEqual(TEXT("New edit should set requested content"), Document->Content, FString(TEXT("D")));
	TestFalse(TEXT("New edit should clear redo stack"), FNotepadDocumentHistoryService::CanRedo(Document));
	TestEqual(TEXT("Undo should return to the state before new edit"), Document->UndoStack.Last(), FString(TEXT("B")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDocumentHistoryMarkSavedTest,
	"UNotepad.DocumentHistory.MarkSaved.UpdatesSavedBaseline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDocumentHistoryMarkSavedTest::RunTest(const FString& Parameters)
{
	TSharedRef<FNotepadDocument> Document = MakeHistoryTestDocument(TEXT("A"));
	FNotepadDocumentHistoryService::SetContent(Document, TEXT("B"), true);

	FNotepadDocumentHistoryService::MarkSaved(Document);

	TestEqual(TEXT("Saved content should become current content"), Document->SavedContent, FString(TEXT("B")));
	TestFalse(TEXT("Document should no longer be dirty after save"), Document->bDirty);

	FNotepadDocumentHistoryService::Undo(Document);
	TestEqual(TEXT("Undo should still work after save"), Document->Content, FString(TEXT("A")));
	TestTrue(TEXT("Undoing away from saved baseline should mark dirty"), Document->bDirty);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDocumentHistoryCapsHistoryTest,
	"UNotepad.DocumentHistory.SetContent.CapsHistory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDocumentHistoryCapsHistoryTest::RunTest(const FString& Parameters)
{
	TSharedRef<FNotepadDocument> Document = MakeHistoryTestDocument(TEXT("0"));

	FNotepadDocumentHistoryService::SetContent(Document, TEXT("1"), true, 3);
	FNotepadDocumentHistoryService::SetContent(Document, TEXT("2"), true, 3);
	FNotepadDocumentHistoryService::SetContent(Document, TEXT("3"), true, 3);
	FNotepadDocumentHistoryService::SetContent(Document, TEXT("4"), true, 3);

	TestEqual(TEXT("Undo stack should be capped"), Document->UndoStack.Num(), 3);
	TestEqual(TEXT("Oldest retained undo state should be 1"), Document->UndoStack[0], FString(TEXT("1")));
	TestEqual(TEXT("Newest undo state should be 3"), Document->UndoStack.Last(), FString(TEXT("3")));

	FNotepadDocumentHistoryService::Undo(Document, 3);
	FNotepadDocumentHistoryService::Undo(Document, 3);
	FNotepadDocumentHistoryService::Undo(Document, 3);

	TestEqual(TEXT("Undo should not go past capped history"), Document->Content, FString(TEXT("1")));
	TestFalse(TEXT("Undo should now be exhausted"), FNotepadDocumentHistoryService::CanUndo(Document));

	return true;
}

#endif
