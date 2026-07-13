// Fill out your copyright notice in the Description page of Project Settings.

#include "Services/NotepadTextActionService.h"

namespace
{
	struct FParsedNotepadText final
	{
		TArray<FString> Lines;
		FString LineEnding = TEXT("\n");
		bool bHasDetectedLineEnding = false;
		bool bEndsWithLineEnding = false;
	};

	bool IsLineEndingAt(const FString& Text, int32 Index, int32& OutLength)
	{
		if (!Text.IsValidIndex(Index))
		{
			OutLength = 0;
			return false;
		}

		if (Text[Index] == TEXT('\r'))
		{
			OutLength = Text.IsValidIndex(Index + 1) && Text[Index + 1] == TEXT('\n') ? 2 : 1;
			return true;
		}

		if (Text[Index] == TEXT('\n'))
		{
			OutLength = 1;
			return true;
		}

		OutLength = 0;
		return false;
	}

	FParsedNotepadText ParseText(const FString& Text)
	{
		FParsedNotepadText Parsed;
		FString CurrentLine;

		if (Text.IsEmpty())
		{
			Parsed.Lines.Add(FString());
			return Parsed;
		}

		for (int32 Index = 0; Index < Text.Len(); ++Index)
		{
			int32 LineEndingLength = 0;
			if (IsLineEndingAt(Text, Index, LineEndingLength))
			{
				if (!Parsed.bHasDetectedLineEnding)
				{
					Parsed.LineEnding = Text.Mid(Index, LineEndingLength);
					Parsed.bHasDetectedLineEnding = true;
				}

				Parsed.Lines.Add(CurrentLine);
				CurrentLine.Reset();
				Parsed.bEndsWithLineEnding = true;
				Index += LineEndingLength - 1;
				continue;
			}

			CurrentLine.AppendChar(Text[Index]);
			Parsed.bEndsWithLineEnding = false;
		}

		Parsed.Lines.Add(CurrentLine);
		return Parsed;
	}

	FString JoinLines(const FParsedNotepadText& Parsed)
	{
		return FString::Join(Parsed.Lines, *Parsed.LineEnding);
	}

	bool HasTrailingEmptyLine(const FParsedNotepadText& Parsed)
	{
		return Parsed.bEndsWithLineEnding && Parsed.Lines.Num() > 1 && Parsed.Lines.Last().IsEmpty();
	}

	int32 GetLastEditableLineIndex(const FParsedNotepadText& Parsed)
	{
		const int32 LastLineIndex = Parsed.Lines.Num() - 1;
		return HasTrailingEmptyLine(Parsed) ? LastLineIndex - 1 : LastLineIndex;
	}

	int32 CountLeadingWhitespace(const FString& Line)
	{
		int32 Index = 0;
		while (Index < Line.Len() && (Line[Index] == TEXT(' ') || Line[Index] == TEXT('\t')))
		{
			++Index;
		}
		return Index;
	}

	bool IsCommentedLine(const FString& Line, const FString& CommentPrefix)
	{
		const int32 PrefixIndex = CountLeadingWhitespace(Line);
		return Line.Mid(PrefixIndex).StartsWith(CommentPrefix);
	}

	bool IsBlankLine(const FString& Line)
	{
		return Line.TrimStartAndEnd().IsEmpty();
	}

	bool RemoveLineComment(FString& Line, const FString& CommentPrefix)
	{
		const int32 PrefixIndex = CountLeadingWhitespace(Line);
		if (!Line.Mid(PrefixIndex).StartsWith(CommentPrefix))
		{
			return false;
		}

		Line.RemoveAt(PrefixIndex, CommentPrefix.Len(), EAllowShrinking::No);
		if (Line.IsValidIndex(PrefixIndex) && Line[PrefixIndex] == TEXT(' '))
		{
			Line.RemoveAt(PrefixIndex, 1, EAllowShrinking::No);
		}
		return true;
	}

	bool AddLineComment(FString& Line, const FString& CommentPrefix)
	{
		if (IsBlankLine(Line))
		{
			return false;
		}

		const int32 PrefixIndex = CountLeadingWhitespace(Line);
		Line.InsertAt(PrefixIndex, CommentPrefix + TEXT(" "));
		return true;
	}

	bool ToggleLineCommentInternal(FParsedNotepadText& Parsed, int32 FirstLineIndex, int32 LastLineIndex, const FString& CommentPrefix)
	{
		if (CommentPrefix.IsEmpty() || Parsed.Lines.IsEmpty())
		{
			return false;
		}

		const int32 MaxLineIndex = Parsed.Lines.Num() - 1;
		FirstLineIndex = FMath::Clamp(FirstLineIndex, 0, MaxLineIndex);
		LastLineIndex = FMath::Clamp(LastLineIndex, FirstLineIndex, MaxLineIndex);

		bool bHasCommentableLine = false;
		bool bAllCommentableLinesAreCommented = true;
		for (int32 LineIndex = FirstLineIndex; LineIndex <= LastLineIndex; ++LineIndex)
		{
			const FString& Line = Parsed.Lines[LineIndex];
			if (IsBlankLine(Line))
			{
				continue;
			}

			bHasCommentableLine = true;
			if (!IsCommentedLine(Line, CommentPrefix))
			{
				bAllCommentableLinesAreCommented = false;
				break;
			}
		}

		if (!bHasCommentableLine)
		{
			return false;
		}

		bool bChanged = false;
		for (int32 LineIndex = FirstLineIndex; LineIndex <= LastLineIndex; ++LineIndex)
		{
			if (bAllCommentableLinesAreCommented)
			{
				bChanged |= RemoveLineComment(Parsed.Lines[LineIndex], CommentPrefix);
			}
			else
			{
				bChanged |= AddLineComment(Parsed.Lines[LineIndex], CommentPrefix);
			}
		}

		return bChanged;
	}
}

bool FNotepadTextActionService::ToggleLineComment(FString& InOutText, int32 FirstLineIndex, int32 LastLineIndex, const FString& CommentPrefix)
{
	FParsedNotepadText Parsed = ParseText(InOutText);
	if (!ToggleLineCommentInternal(Parsed, FirstLineIndex, LastLineIndex, CommentPrefix))
	{
		return false;
	}

	const FString NewText = JoinLines(Parsed);
	if (NewText == InOutText)
	{
		return false;
	}

	InOutText = NewText;
	return true;
}

bool FNotepadTextActionService::ToggleLineCommentForSelection(FString& InOutText, const FString& CommentPrefix)
{
	FParsedNotepadText Parsed = ParseText(InOutText);
	int32 LastLineIndex = GetLastEditableLineIndex(Parsed);
	if (LastLineIndex < 0)
	{
		return false;
	}

	if (!ToggleLineCommentInternal(Parsed, 0, LastLineIndex, CommentPrefix))
	{
		return false;
	}

	const FString NewText = JoinLines(Parsed);
	if (NewText == InOutText)
	{
		return false;
	}

	InOutText = NewText;
	return true;
}

bool FNotepadTextActionService::DuplicateLine(FString& InOutText, int32 LineIndex)
{
	FParsedNotepadText Parsed = ParseText(InOutText);
	const int32 LastEditableLineIndex = GetLastEditableLineIndex(Parsed);
	if (LastEditableLineIndex < 0)
	{
		return false;
	}

	LineIndex = FMath::Clamp(LineIndex, 0, LastEditableLineIndex);
	const FString LineToDuplicate = Parsed.Lines[LineIndex];
	Parsed.Lines.Insert(LineToDuplicate, LineIndex + 1);

	const FString NewText = JoinLines(Parsed);
	if (NewText == InOutText)
	{
		return false;
	}

	InOutText = NewText;
	return true;
}

bool FNotepadTextActionService::MoveLine(FString& InOutText, int32 LineIndex, int32 Direction, int32& OutNewLineIndex)
{
	OutNewLineIndex = LineIndex;

	if (Direction == 0)
	{
		return false;
	}

	FParsedNotepadText Parsed = ParseText(InOutText);
	const int32 LastEditableLineIndex = GetLastEditableLineIndex(Parsed);
	if (LastEditableLineIndex <= 0 || LineIndex < 0 || LineIndex > LastEditableLineIndex)
	{
		return false;
	}

	const int32 TargetLineIndex = LineIndex + (Direction < 0 ? -1 : 1);
	if (TargetLineIndex < 0 || TargetLineIndex > LastEditableLineIndex)
	{
		return false;
	}

	Parsed.Lines.Swap(LineIndex, TargetLineIndex);
	OutNewLineIndex = TargetLineIndex;

	const FString NewText = JoinLines(Parsed);
	if (NewText == InOutText)
	{
		return false;
	}

	InOutText = NewText;
	return true;
}

bool FNotepadTextActionService::TrimTrailingWhitespace(FString& InOutText)
{
	FParsedNotepadText Parsed = ParseText(InOutText);
	bool bChanged = false;

	for (FString& Line : Parsed.Lines)
	{
		const int32 OriginalLength = Line.Len();
		while (!Line.IsEmpty())
		{
			const TCHAR LastCharacter = Line[Line.Len() - 1];
			if (LastCharacter != TEXT(' ') && LastCharacter != TEXT('\t'))
			{
				break;
			}

			Line.RemoveAt(Line.Len() - 1, 1, EAllowShrinking::No);
		}
		bChanged |= Line.Len() != OriginalLength;
	}

	if (!bChanged)
	{
		return false;
	}

	InOutText = JoinLines(Parsed);
	return true;
}

bool FNotepadTextActionService::ConvertTabsToSpaces(FString& InOutText, int32 TabSize)
{
	TabSize = FMath::Clamp(TabSize, 1, 16);
	FParsedNotepadText Parsed = ParseText(InOutText);
	bool bChanged = false;

	for (FString& Line : Parsed.Lines)
	{
		FString ConvertedLine;
		int32 Column = 0;
		bool bLineChanged = false;

		for (const TCHAR Character : Line)
		{
			if (Character == TEXT('\t'))
			{
				const int32 SpacesToNextTab = TabSize - (Column % TabSize);
				ConvertedLine += FString::ChrN(SpacesToNextTab, TEXT(' '));
				Column += SpacesToNextTab;
				bLineChanged = true;
			}
			else
			{
				ConvertedLine.AppendChar(Character);
				++Column;
			}
		}

		if (bLineChanged)
		{
			Line = MoveTemp(ConvertedLine);
			bChanged = true;
		}
	}

	if (!bChanged)
	{
		return false;
	}

	InOutText = JoinLines(Parsed);
	return true;
}

bool FNotepadTextActionService::ConvertLeadingSpacesToTabs(FString& InOutText, int32 TabSize)
{
	TabSize = FMath::Clamp(TabSize, 1, 16);
	FParsedNotepadText Parsed = ParseText(InOutText);
	bool bChanged = false;

	for (FString& Line : Parsed.Lines)
	{
		int32 SpaceCount = 0;
		while (Line.IsValidIndex(SpaceCount) && Line[SpaceCount] == TEXT(' '))
		{
			++SpaceCount;
		}

		const int32 TabCount = SpaceCount / TabSize;
		if (TabCount <= 0)
		{
			continue;
		}

		const int32 SpacesToReplace = TabCount * TabSize;
		FString ConvertedLine;
		ConvertedLine += FString::ChrN(TabCount, TEXT('\t'));
		ConvertedLine += Line.Mid(SpacesToReplace);
		Line = MoveTemp(ConvertedLine);
		bChanged = true;
	}

	if (!bChanged)
	{
		return false;
	}

	InOutText = JoinLines(Parsed);
	return true;
}

bool FNotepadTextActionService::EnsureFinalNewline(FString& InOutText)
{
	if (InOutText.IsEmpty())
	{
		return false;
	}

	int32 LineEndingLength = 0;
	if (IsLineEndingAt(InOutText, InOutText.Len() - 1, LineEndingLength))
	{
		return false;
	}

	const FParsedNotepadText Parsed = ParseText(InOutText);
	InOutText += Parsed.LineEnding;
	return true;
}

bool FNotepadTextActionService::NormalizeLineEndings(FString& InOutText, ENotepadLineEnding LineEnding)
{
	const FString TargetLineEnding = GetLineEndingText(LineEnding);
	FString NormalizedText;
	bool bChanged = false;

	for (int32 Index = 0; Index < InOutText.Len(); ++Index)
	{
		int32 LineEndingLength = 0;
		if (IsLineEndingAt(InOutText, Index, LineEndingLength))
		{
			const FString ExistingLineEnding = InOutText.Mid(Index, LineEndingLength);
			NormalizedText += TargetLineEnding;
			bChanged |= ExistingLineEnding != TargetLineEnding;
			Index += LineEndingLength - 1;
			continue;
		}

		NormalizedText.AppendChar(InOutText[Index]);
	}

	if (!bChanged)
	{
		return false;
	}

	InOutText = MoveTemp(NormalizedText);
	return true;
}

FString FNotepadTextActionService::GetLineEndingText(ENotepadLineEnding LineEnding)
{
	return LineEnding == ENotepadLineEnding::CRLF ? TEXT("\r\n") : TEXT("\n");
}
