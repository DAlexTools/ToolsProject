// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/NotepadDocumentTypes.h"

class UNOTEPAD_API FNotepadFormatService
{
public:
	static bool Format(EUNotepadDocumentMode Mode, FString& InOutText, FString& OutError);
	static ENotepadValidationResult Validate(EUNotepadDocumentMode Mode, const FString& Text, FString& OutError);

private:
	static bool FormatJson(FString& InOutText, FString& OutError);
	static bool ValidateJson(const FString& Text, FString& OutError);
	static bool FormatCsv(FString& InOutText, FString& OutError);
	static bool ValidateCsv(const FString& Text, FString& OutError);
	static bool ParseCsv(const FString& Text, TArray<TArray<FString>>& OutRows, FString& OutError);
	static FString WriteCsv(const TArray<TArray<FString>>& Rows);
	static FString EscapeCsvCell(const FString& Cell);
};
