// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

enum class ENotepadLineEnding : uint8
{
	LF,
	CRLF
};

class UNOTEPAD_API FNotepadTextActionService
{
public:
	static bool ToggleLineComment(FString& InOutText, int32 FirstLineIndex, int32 LastLineIndex, const FString& CommentPrefix);
	static bool ToggleLineCommentForSelection(FString& InOutText, const FString& CommentPrefix);
	static bool DuplicateLine(FString& InOutText, int32 LineIndex);
	static bool MoveLine(FString& InOutText, int32 LineIndex, int32 Direction, int32& OutNewLineIndex);
	static bool TrimTrailingWhitespace(FString& InOutText);
	static bool ConvertTabsToSpaces(FString& InOutText, int32 TabSize);
	static bool ConvertLeadingSpacesToTabs(FString& InOutText, int32 TabSize);
	static bool EnsureFinalNewline(FString& InOutText);
	static bool NormalizeLineEndings(FString& InOutText, ENotepadLineEnding LineEnding);

	static FString GetLineEndingText(ENotepadLineEnding LineEnding);
};
