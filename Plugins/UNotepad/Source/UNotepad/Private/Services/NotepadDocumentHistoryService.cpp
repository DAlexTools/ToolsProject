// Fill out your copyright notice in the Description page of Project Settings.

#include "Services/NotepadDocumentHistoryService.h"

bool FNotepadDocumentHistoryService::SetContent(const TSharedPtr<FNotepadDocument>& Document, const FString& NewContent, bool bRecordUndo, int32 MaxHistoryEntries)
{
	if (!Document.IsValid() || Document->Content == NewContent)
	{
		return false;
	}

	if (bRecordUndo)
	{
		PushUndoState(Document, Document->Content, MaxHistoryEntries);
	}

	Document->Content = NewContent;
	UpdateDirtyState(Document);
	return true;
}

bool FNotepadDocumentHistoryService::Undo(const TSharedPtr<FNotepadDocument>& Document, int32 MaxHistoryEntries)
{
	if (!CanUndo(Document))
	{
		return false;
	}

	const FString CurrentContent = Document->Content;
	const FString PreviousContent = Document->UndoStack.Pop(EAllowShrinking::No);
	PushRedoState(Document, CurrentContent, MaxHistoryEntries);

	Document->Content = PreviousContent;
	UpdateDirtyState(Document);
	return true;
}

bool FNotepadDocumentHistoryService::Redo(const TSharedPtr<FNotepadDocument>& Document, int32 MaxHistoryEntries)
{
	if (!CanRedo(Document))
	{
		return false;
	}

	const FString CurrentContent = Document->Content;
	const FString NextContent = Document->RedoStack.Pop(EAllowShrinking::No);
	PushUndoStatePreservingRedo(Document, CurrentContent, MaxHistoryEntries);

	Document->Content = NextContent;
	UpdateDirtyState(Document);
	return true;
}

bool FNotepadDocumentHistoryService::CanUndo(const TSharedPtr<FNotepadDocument>& Document)
{
	return Document.IsValid() && !Document->UndoStack.IsEmpty();
}

bool FNotepadDocumentHistoryService::CanRedo(const TSharedPtr<FNotepadDocument>& Document)
{
	return Document.IsValid() && !Document->RedoStack.IsEmpty();
}

void FNotepadDocumentHistoryService::MarkSaved(const TSharedPtr<FNotepadDocument>& Document)
{
	if (!Document.IsValid())
	{
		return;
	}

	Document->SavedContent = Document->Content;
	Document->bDirty = false;
}

void FNotepadDocumentHistoryService::UpdateDirtyState(const TSharedPtr<FNotepadDocument>& Document)
{
	if (Document.IsValid())
	{
		Document->bDirty = Document->Content != Document->SavedContent;
	}
}

void FNotepadDocumentHistoryService::PushUndoState(const TSharedPtr<FNotepadDocument>& Document, const FString& PreviousContent, int32 MaxHistoryEntries)
{
	PushUndoStatePreservingRedo(Document, PreviousContent, MaxHistoryEntries);
	if (Document.IsValid())
	{
		Document->RedoStack.Reset();
	}
}

void FNotepadDocumentHistoryService::PushRedoState(const TSharedPtr<FNotepadDocument>& Document, const FString& PreviousContent, int32 MaxHistoryEntries)
{
	if (!Document.IsValid())
	{
		return;
	}

	Document->RedoStack.Add(PreviousContent);
	const int32 SafeMaxHistoryEntries = NormalizeMaxHistoryEntries(MaxHistoryEntries);
	if (Document->RedoStack.Num() > SafeMaxHistoryEntries)
	{
		Document->RedoStack.RemoveAt(0, Document->RedoStack.Num() - SafeMaxHistoryEntries, EAllowShrinking::No);
	}
}

void FNotepadDocumentHistoryService::PushUndoStatePreservingRedo(const TSharedPtr<FNotepadDocument>& Document, const FString& PreviousContent, int32 MaxHistoryEntries)
{
	if (!Document.IsValid())
	{
		return;
	}

	if (Document->UndoStack.Num() == 0 || Document->UndoStack.Last() != PreviousContent)
	{
		Document->UndoStack.Add(PreviousContent);
		const int32 SafeMaxHistoryEntries = NormalizeMaxHistoryEntries(MaxHistoryEntries);
		if (Document->UndoStack.Num() > SafeMaxHistoryEntries)
		{
			Document->UndoStack.RemoveAt(0, Document->UndoStack.Num() - SafeMaxHistoryEntries, EAllowShrinking::No);
		}
	}
}

int32 FNotepadDocumentHistoryService::NormalizeMaxHistoryEntries(int32 MaxHistoryEntries)
{
	return FMath::Max(MaxHistoryEntries, 1);
}
