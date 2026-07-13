// Fill out your copyright notice in the Description page of Project Settings.

#include "Services/NotepadFormatService.h"

#include "Dom/JsonValue.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool FNotepadFormatService::Format(EUNotepadDocumentMode Mode, FString& InOutText, FString& OutError)
{
	switch (Mode)
	{
		case EUNotepadDocumentMode::Json:
		{
			return FormatJson(InOutText, OutError);
		}
		case EUNotepadDocumentMode::Csv:
		{
			return FormatCsv(InOutText, OutError);
		}
		default:
		{
			OutError = TEXT("Formatter is available for JSON and CSV modes");
			return false;
		}
	}
}

ENotepadValidationResult FNotepadFormatService::Validate(EUNotepadDocumentMode Mode, const FString& Text, FString& OutError)
{
	switch (Mode)
	{
		case EUNotepadDocumentMode::Json:
		{
			return ValidateJson(Text, OutError) ? ENotepadValidationResult::Valid : ENotepadValidationResult::Invalid;
		}
		case EUNotepadDocumentMode::Csv:
		{
			return ValidateCsv(Text, OutError) ? ENotepadValidationResult::Valid : ENotepadValidationResult::Invalid;
		}
		default:
		{
			return ENotepadValidationResult::NotSupported;
		}
	}
}

bool FNotepadFormatService::FormatJson(FString& InOutText, FString& OutError)
{
	TSharedPtr<FJsonValue> JsonValue;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InOutText);
	if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
	{
		OutError = FString::Printf(TEXT("Invalid JSON: %s"), *Reader->GetErrorMessage());
		return false;
	}

	FString FormattedText;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&FormattedText);

	if (!FJsonSerializer::Serialize(JsonValue, TEXT(""), Writer))
	{
		OutError = TEXT("Failed to format JSON");
		return false;
	}

	InOutText = FormattedText;
	return true;
}

bool FNotepadFormatService::ValidateJson(const FString& Text, FString& OutError)
{
	TSharedPtr<FJsonValue> JsonValue;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, JsonValue) || !JsonValue.IsValid())
	{
		OutError = FString::Printf(TEXT("Invalid JSON: %s"), *Reader->GetErrorMessage());
		return false;
	}

	return true;
}

bool FNotepadFormatService::FormatCsv(FString& InOutText, FString& OutError)
{
	TArray<TArray<FString>> Rows;
	if (!ParseCsv(InOutText, Rows, OutError))
	{
		return false;
	}

	InOutText = WriteCsv(Rows);
	return true;
}

bool FNotepadFormatService::ValidateCsv(const FString& Text, FString& OutError)
{
	TArray<TArray<FString>> Rows;
	return ParseCsv(Text, Rows, OutError);
}

bool FNotepadFormatService::ParseCsv(const FString& Text, TArray<TArray<FString>>& OutRows, FString& OutError)
{
	OutRows.Reset();

	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, false);

	int32 ExpectedColumnCount = INDEX_NONE;
	for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
	{
		const FString& Line = Lines[LineIndex];
		TArray<FString> Row;
		FString Cell;
		bool bInQuotes = false;

		for (int32 CharIndex = 0; CharIndex < Line.Len(); ++CharIndex)
		{
			const TCHAR Character = Line[CharIndex];
			if (Character == TEXT('"'))
			{
				if (bInQuotes && CharIndex + 1 < Line.Len() && Line[CharIndex + 1] == TEXT('"'))
				{
					Cell.AppendChar(TEXT('"'));
					++CharIndex;
				}
				else
				{
					bInQuotes = !bInQuotes;
				}
			}
			else if (Character == TEXT(',') && !bInQuotes)
			{
				Row.Add(Cell);
				Cell.Reset();
			}
			else
			{
				Cell.AppendChar(Character);
			}
		}

		if (bInQuotes)
		{
			OutError = FString::Printf(TEXT("Invalid CSV: line %d has an unterminated quoted field"), LineIndex + 1);
			return false;
		}

		Row.Add(Cell);
		if (ExpectedColumnCount == INDEX_NONE)
		{
			ExpectedColumnCount = Row.Num();
		}
		else if (Row.Num() != ExpectedColumnCount)
		{
			OutError = FString::Printf(
				TEXT("Invalid CSV: line %d has %d columns, expected %d"),
				LineIndex + 1,
				Row.Num(),
				ExpectedColumnCount);
			return false;
		}

		OutRows.Add(Row);
	}

	return true;
}

FString FNotepadFormatService::WriteCsv(const TArray<TArray<FString>>& Rows)
{
	FString Output;
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<FString>& Row = Rows[RowIndex];
		for (int32 ColumnIndex = 0; ColumnIndex < Row.Num(); ++ColumnIndex)
		{
			if (ColumnIndex > 0)
			{
				Output += TEXT(",");
			}
			Output += EscapeCsvCell(Row[ColumnIndex]);
		}

		if (RowIndex + 1 < Rows.Num())
		{
			Output += LINE_TERMINATOR;
		}
	}

	return Output;
}

FString FNotepadFormatService::EscapeCsvCell(const FString& Cell)
{
	if (!Cell.Contains(TEXT(",")) && !Cell.Contains(TEXT("\"")) && !Cell.Contains(TEXT("\n")) && !Cell.Contains(TEXT("\r")))
	{
		return Cell;
	}

	FString EscapedCell = Cell.Replace(TEXT("\""), TEXT("\"\""));
	return FString::Printf(TEXT("\"%s\""), *EscapedCell);
}
