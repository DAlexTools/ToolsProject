// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

enum class EUNotepadDocumentMode : uint8
{
	Text,
	Code,
	Json,
	Csv
};

enum class ENotepadValidationResult : uint8
{
	NotSupported,
	Valid,
	Invalid
};

struct FNotepadDocument final 
{
	FString FilePath;
	FString DisplayName;
	FString Content;
	FString SavedContent;
	TArray<FString> UndoStack;
	TArray<FString> RedoStack;
	EUNotepadDocumentMode Mode = EUNotepadDocumentMode::Text;
	bool bDirty = false;
};

