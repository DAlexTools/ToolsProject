// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/NotepadDocumentTypes.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBox;

/** Delegate invoked for document-related actions. */
DECLARE_DELEGATE_OneParam(FNotepadDocumentAction, TSharedPtr<FNotepadDocument>);

/**
 * @brief Tab strip widget for managing open documents.
 *
 * Displays open documents as tabs and allows selecting or closing them.
 */
class UNOTEPAD_API SNotepadTabStrip : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNotepadTabStrip) {}
		
		/** Called when a document tab is selected. */
		SLATE_EVENT(FNotepadDocumentAction, OnDocumentSelected)
		
		/** Called when a document tab is closed. */
		SLATE_EVENT(FNotepadDocumentAction, OnDocumentClosed)
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the tab strip widget.
	 *
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * @brief Updates the displayed document tabs.
	 *
	 * @param InDocuments Collection of open documents.
	 * @param InActiveDocument Currently active document.
	 */
	void SetDocuments(const TArray<TSharedPtr<FNotepadDocument>>& InDocuments, TSharedPtr<FNotepadDocument> InActiveDocument);

private:
	/**
	 * @brief Rebuilds the tab strip.
	 */
	void Rebuild();

	/**
	 * @brief Selects the specified document.
	 *
	 * @param Document Document to activate.
	 *
	 * @return Slate reply indicating whether the event was handled.
	 */
	FReply SelectDocument(TSharedPtr<FNotepadDocument> Document);

	/**
	 * @brief Closes the specified document.
	 *
	 * @param Document Document to close.
	 *
	 * @return Slate reply indicating whether the event was handled.
	 */
	FReply CloseDocument(TSharedPtr<FNotepadDocument> Document);

	/** Collection of open documents. */
	TArray<TSharedPtr<FNotepadDocument>> Documents;

	/** Currently active document. */
	TSharedPtr<FNotepadDocument> ActiveDocument;

	/** Scroll box containing the document tabs. */
	TSharedPtr<SScrollBox> TabStrip;

	/** Delegate fired when a document is selected. */
	FNotepadDocumentAction OnDocumentSelected;

	/** Delegate fired when a document is closed. */
	FNotepadDocumentAction OnDocumentClosed;
};
