// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/NotepadDocumentTypes.h"

/**
 * @brief Collection of helper functions for working with notepad documents.
 *
 * Provides utility functions for document mode detection, file naming,
 * display formatting, and text processing.
 */
class UNOTEPAD_API FNotepadDocumentUtils final
{
public:
	/**
	 * @brief Detects the document mode from a file path.
	 *
	 * The document mode is determined from the file extension.
	 *
	 * @param FilePath Path to the document.
	 *
	 * @return Detected document mode.
	 */
	static EUNotepadDocumentMode DetectModeFromPath(const FString& FilePath);

	/**
	 * @brief Returns the default file extension for a document mode.
	 *
	 * @param Mode Document mode.
	 *
	 * @return Default file extension without a leading dot.
	 */
	static FString GetDefaultExtension(EUNotepadDocumentMode Mode);

	/**
	 * @brief Returns a user-friendly label for a document mode.
	 *
	 * @param Mode Document mode.
	 *
	 * @return Display label.
	 */
	static FString GetModeLabel(EUNotepadDocumentMode Mode);

	/**
	 * @brief Generates a default name for a new unsaved document.
	 *
	 * @param Mode Document mode.
	 * @param Index Sequential document index.
	 *
	 * @return Generated file name.
	 */
	static FString GetUntitledName(EUNotepadDocumentMode Mode, int32 Index);

	/**
	 * @brief Creates a display name from a file path.
	 *
	 * Returns the file name if the path is valid; otherwise returns
	 * the default "Untitled" name.
	 *
	 * @param FilePath Path to the document.
	 *
	 * @return Display name.
	 */
	static FString MakeDisplayName(const FString& FilePath);

	/**
	 * @brief Counts the number of lines in a text string.
	 *
	 * An empty string is considered to contain a single line.
	 *
	 * @param Text Input text.
	 *
	 * @return Number of lines.
	 */
	static int32 CountLines(const FString& Text);

	/**
	 * @brief Builds line number text for display alongside the editor.
	 *
	 * Generates a newline-separated list of line numbers from 1 to
	 * the specified line count.
	 *
	 * @param LineCount Number of document lines.
	 *
	 * @return Formatted line number text.
	 */
	static FString BuildLineNumberText(int32 LineCount);
};