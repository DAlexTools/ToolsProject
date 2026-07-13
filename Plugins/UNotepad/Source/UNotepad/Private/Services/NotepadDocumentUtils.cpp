// Fill out your copyright notice in the Description page of Project Settings.

#include "Services/NotepadDocumentUtils.h"
#include "Misc/Paths.h"

EUNotepadDocumentMode FNotepadDocumentUtils::DetectModeFromPath(const FString& FilePath)
{
	const FString Extension = FPaths::GetExtension(FilePath).ToLower();
	if (Extension == TEXT("json") || Extension == TEXT("uplugin") || Extension == TEXT("uproject"))
	{
		return EUNotepadDocumentMode::Json;
	}

	if (Extension == TEXT("csv") || Extension == TEXT("tsv"))
	{
		return EUNotepadDocumentMode::Csv;
	}

	if (Extension == TEXT("txt") || Extension == TEXT("md") || Extension == TEXT("log"))
	{
		return EUNotepadDocumentMode::Text;
	}

	return EUNotepadDocumentMode::Code;
}

FString FNotepadDocumentUtils::GetDefaultExtension(EUNotepadDocumentMode Mode)
{
	switch (Mode)
	{
		case EUNotepadDocumentMode::Code:
		{
			return TEXT("cpp");
		}
		case EUNotepadDocumentMode::Json:
		{
			return TEXT("json");
		}
		case EUNotepadDocumentMode::Csv:
		{
			return TEXT("csv");
		}
		case EUNotepadDocumentMode::Text:
		default:
		{
			return TEXT("txt");
		}
	}
}

FString FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode Mode)
{
	switch (Mode)
	{
		case EUNotepadDocumentMode::Code:
		{
			return TEXT("Code");
		}
		case EUNotepadDocumentMode::Json:
		{
			return TEXT("JSON");
		}
		case EUNotepadDocumentMode::Csv:
		{
			return TEXT("CSV");
		}
		case EUNotepadDocumentMode::Text:
		default:
		{
			return TEXT("Text");
		}
	}
}

FString FNotepadDocumentUtils::GetUntitledName(EUNotepadDocumentMode Mode, int32 Index)
{
	return FString::Printf(TEXT("Untitled-%d.%s"), Index, *GetDefaultExtension(Mode));
}

FString FNotepadDocumentUtils::MakeDisplayName(const FString& FilePath)
{
	const FString CleanName = FPaths::GetCleanFilename(FilePath);
	return CleanName.IsEmpty() ? TEXT("Untitled") : CleanName;
}

int32 FNotepadDocumentUtils::CountLines(const FString& Text)
{
	if (Text.IsEmpty())
	{
		return 1;
	}

	int32 LineCount = 1;
	for (const TCHAR Character : Text)
	{
		if (Character == TEXT('\n'))
		{
			++LineCount;
		}
	}

	return LineCount;
}

FString FNotepadDocumentUtils::BuildLineNumberText(int32 LineCount)
{
	const int32 SafeLineCount = FMath::Max(LineCount, 1);
	FString Result;
	for (int32 LineNumber = 1; LineNumber <= SafeLineCount; ++LineNumber)
	{
		Result += FString::FromInt(LineNumber);
		if (LineNumber < SafeLineCount)
		{
			Result += LINE_TERMINATOR;
		}
	}

	return Result;
}
