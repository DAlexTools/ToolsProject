// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/NotepadDocumentTypes.h"

class UNOTEPAD_API FNotepadDocumentUtils
{
public:
	static EUNotepadDocumentMode DetectModeFromPath(const FString& FilePath);
	static FString GetDefaultExtension(EUNotepadDocumentMode Mode);
	static FString GetModeLabel(EUNotepadDocumentMode Mode);
	static FString GetUntitledName(EUNotepadDocumentMode Mode, int32 Index);
	static FString MakeDisplayName(const FString& FilePath);
	static int32 CountLines(const FString& Text);
	static FString BuildLineNumberText(int32 LineCount);
};
