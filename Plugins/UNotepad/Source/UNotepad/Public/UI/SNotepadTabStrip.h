// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/NotepadDocumentTypes.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;

DECLARE_DELEGATE_OneParam(FNotepadDocumentAction, TSharedPtr<FNotepadDocument>);

class UNOTEPAD_API SNotepadTabStrip : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNotepadTabStrip) {}
		SLATE_EVENT(FNotepadDocumentAction, OnDocumentSelected)
		SLATE_EVENT(FNotepadDocumentAction, OnDocumentClosed)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetDocuments(const TArray<TSharedPtr<FNotepadDocument>>& InDocuments, TSharedPtr<FNotepadDocument> InActiveDocument);

private:
	void Rebuild();
	FReply SelectDocument(TSharedPtr<FNotepadDocument> Document);
	FReply CloseDocument(TSharedPtr<FNotepadDocument> Document);

	TArray<TSharedPtr<FNotepadDocument>> Documents;
	TSharedPtr<FNotepadDocument> ActiveDocument;
	TSharedPtr<SScrollBox> TabStrip;
	FNotepadDocumentAction OnDocumentSelected;
	FNotepadDocumentAction OnDocumentClosed;
};
