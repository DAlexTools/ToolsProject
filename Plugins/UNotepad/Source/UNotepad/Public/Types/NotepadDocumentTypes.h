// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Defines the available document modes.
 *
 * Determines how the document is interpreted and displayed
 * within the editor (e.g. plain text, source code, or structured data).
 */
enum class EUNotepadDocumentMode : uint8
{
	/** Plain text document. */
	Text,

	/** Source code document. */
	Code,

	/** JSON document. */
	Json,

	/** CSV document. */
	Csv
};

/**
 * @brief Result of a document validation operation.
 */
enum class ENotepadValidationResult : uint8
{
	/** The document type or operation is not supported. */
	NotSupported,

	/** Validation completed successfully. */
	Valid,

	/** Validation failed. */
	Invalid
};

/**
 * @brief Represents an opened document in the Notepad editor.
 *
 * Stores the document state, including its file information, current content,
 * undo/redo history, editing mode, and modification status.
 */
struct FNotepadDocument final
{
	/** Full path to the document on disk. */
	FString FilePath;

	/** Display name shown in the editor UI. */
	FString DisplayName;

	/** Current document content. */
	FString Content;

	/** Last saved version of the document content. */
	FString SavedContent;

	/** Stack of previous document states for undo operations. */
	TArray<FString> UndoStack;

	/** Stack of reverted document states for redo operations. */
	TArray<FString> RedoStack;

	/** Current document editing mode. */
	EUNotepadDocumentMode Mode = EUNotepadDocumentMode::Text;

	/** Indicates whether the document has unsaved changes. */
	bool bDirty = false;
};

