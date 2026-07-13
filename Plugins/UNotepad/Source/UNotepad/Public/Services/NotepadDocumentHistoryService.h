// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/NotepadDocumentTypes.h"

class UNOTEPAD_API FNotepadDocumentHistoryService
{
public:
	static constexpr int32 DefaultMaxHistoryEntries = 200;

	static bool SetContent(const TSharedPtr<FNotepadDocument>& Document, const FString& NewContent, bool bRecordUndo, int32 MaxHistoryEntries = DefaultMaxHistoryEntries);
	static bool Undo(const TSharedPtr<FNotepadDocument>& Document, int32 MaxHistoryEntries = DefaultMaxHistoryEntries);
	static bool Redo(const TSharedPtr<FNotepadDocument>& Document, int32 MaxHistoryEntries = DefaultMaxHistoryEntries);
	static bool CanUndo(const TSharedPtr<FNotepadDocument>& Document);
	static bool CanRedo(const TSharedPtr<FNotepadDocument>& Document);
	static void MarkSaved(const TSharedPtr<FNotepadDocument>& Document);
	static void UpdateDirtyState(const TSharedPtr<FNotepadDocument>& Document);

private:
	static void PushUndoState(const TSharedPtr<FNotepadDocument>& Document, const FString& PreviousContent, int32 MaxHistoryEntries);
	static void PushRedoState(const TSharedPtr<FNotepadDocument>& Document, const FString& PreviousContent, int32 MaxHistoryEntries);
	static void PushUndoStatePreservingRedo(const TSharedPtr<FNotepadDocument>& Document, const FString& PreviousContent, int32 MaxHistoryEntries);
	static int32 NormalizeMaxHistoryEntries(int32 MaxHistoryEntries);
};
