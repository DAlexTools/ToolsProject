// Fill out your copyright notice in the Description page of Project Settings.

#include "Syntax/NotepadSyntaxHighlighterMarshaller.h"

#include "Framework/Text/IRun.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/TextLayout.h"
#include "Framework/Text/TextHitPoint.h"
#include "Framework/Text/TextLineHighlight.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"

namespace
{
	class FNotepadWhitespaceTextRun final : public FSlateTextRun
	{
	public:
		static TSharedRef<FNotepadWhitespaceTextRun> Create(
			const FRunInfo& InRunInfo,
			const TSharedRef<const FString>& InText,
			const FTextBlockStyle& InStyle,
			const FTextRange& InRange,
			int32 InVisualWidthInSpaces,
			FString InMarker,
			bool bInCenterMarker)
		{
			return MakeShareable(new FNotepadWhitespaceTextRun(InRunInfo, InText, InStyle, InRange, InVisualWidthInSpaces, MoveTemp(InMarker), bInCenterMarker));
		}

		virtual FVector2D Measure(int32 BeginIndex, int32 EndIndex, float Scale, const FRunTextContext& TextContext) const override
		{
			(void)TextContext;

			if (EndIndex <= BeginIndex)
			{
				return FVector2D(0.0f, GetMaxHeight(Scale));
			}

			const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const float SpaceWidth = FontMeasure->Measure(TEXT(" "), Style.Font, Scale).X;
			return FVector2D(SpaceWidth * static_cast<float>(VisualWidthInSpaces), GetMaxHeight(Scale));
		}

		virtual int8 GetKerning(int32 CurrentIndex, float Scale, const FRunTextContext& TextContext) const override
		{
			(void)CurrentIndex;
			(void)Scale;
			(void)TextContext;
			return 0;
		}

		virtual int32 OnPaint(
			const FPaintArgs& PaintArgs,
			const FTextArgs& TextArgs,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled) const override
		{
			(void)PaintArgs;
			(void)MyCullingRect;

			const ESlateDrawEffect DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			const TSharedRef<ILayoutBlock>& Block = TextArgs.Block;
			const float InverseScale = Inverse(AllottedGeometry.Scale);

			if (Marker.IsEmpty())
			{
				return LayerId;
			}

			FVector2D MarkerOffset = Block->GetLocationOffset();
			if (bCenterMarker)
			{
				const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
				const float MarkerWidth = FontMeasure->Measure(Marker, Style.Font, AllottedGeometry.Scale).X;
				MarkerOffset.X += FMath::Max(0.0f, (Block->GetSize().X - MarkerWidth) * 0.5f);
			}

			FSlateDrawElement::MakeText(
				OutDrawElements,
				++LayerId,
				AllottedGeometry.ToPaintGeometry(
					TransformVector(InverseScale, Block->GetSize()),
					FSlateLayoutTransform(TransformPoint(InverseScale, MarkerOffset))),
				Marker,
				Style.Font,
				DrawEffects,
				InWidgetStyle.GetColorAndOpacityTint() * Style.ColorAndOpacity.GetColor(InWidgetStyle));

			return LayerId;
		}

		virtual int32 GetTextIndexAt(const TSharedRef<ILayoutBlock>& Block, const FVector2D& Location, float Scale, ETextHitPoint* const OutHitPoint = nullptr) const override
		{
			(void)Scale;

			const FVector2D& BlockOffset = Block->GetLocationOffset();
			const FVector2D& BlockSize = Block->GetSize();
			const bool bContainsPoint =
				Location.X >= BlockOffset.X &&
				Location.X < BlockOffset.X + BlockSize.X &&
				Location.Y >= BlockOffset.Y &&
				Location.Y < BlockOffset.Y + BlockSize.Y;

			if (!bContainsPoint)
			{
				return INDEX_NONE;
			}

			if (OutHitPoint)
			{
				*OutHitPoint = ETextHitPoint::WithinText;
			}

			const FTextRange BlockRange = Block->GetTextRange();
			return (Location.X - BlockOffset.X) < (BlockSize.X * 0.5f) ? BlockRange.BeginIndex : BlockRange.EndIndex;
		}

		virtual FVector2D GetLocationAt(const TSharedRef<ILayoutBlock>& Block, int32 Offset, float Scale) const override
		{
			(void)Scale;

			const FTextRange BlockRange = Block->GetTextRange();
			return Block->GetLocationOffset() + FVector2D(Offset <= BlockRange.BeginIndex ? 0.0f : Block->GetSize().X, 0.0f);
		}

		virtual TSharedRef<IRun> Clone() const override
		{
			return Create(RunInfo, Text, Style, Range, VisualWidthInSpaces, Marker, bCenterMarker);
		}

	private:
		FNotepadWhitespaceTextRun(
			const FRunInfo& InRunInfo,
			const TSharedRef<const FString>& InText,
			const FTextBlockStyle& InStyle,
			const FTextRange& InRange,
			int32 InVisualWidthInSpaces,
			FString InMarker,
			bool bInCenterMarker)
			: FSlateTextRun(InRunInfo, InText, InStyle, InRange)
			, VisualWidthInSpaces(FMath::Max(InVisualWidthInSpaces, 1))
			, Marker(MoveTemp(InMarker))
			, bCenterMarker(bInCenterMarker)
		{
		}

		int32 VisualWidthInSpaces = 1;
		FString Marker;
		bool bCenterMarker = false;
	};
}

TSharedRef<FNotepadSyntaxHighlighterMarshaller> FNotepadSyntaxHighlighterMarshaller::Create(EUNotepadDocumentMode InMode, const FSlateFontInfo& InFont)
{
	return MakeShareable(new FNotepadSyntaxHighlighterMarshaller(InMode, InFont));
}

FNotepadSyntaxHighlighterMarshaller::FNotepadSyntaxHighlighterMarshaller(EUNotepadDocumentMode InMode, const FSlateFontInfo& InFont)
	: Mode(InMode)
{
	BuildStyles(InFont);
}

void FNotepadSyntaxHighlighterMarshaller::SetMode(EUNotepadDocumentMode InMode)
{
	Mode = InMode;
}

void FNotepadSyntaxHighlighterMarshaller::SetShowWhitespace(bool bInShowWhitespace)
{
	bShowWhitespace = bInShowWhitespace;
}

void FNotepadSyntaxHighlighterMarshaller::SetTabSize(int32 InTabSize)
{
	TabSize = FMath::Clamp(InTabSize, 1, 16);
}

bool FNotepadSyntaxHighlighterMarshaller::RequiresLiveUpdate() const
{
	return true;
}

void FNotepadSyntaxHighlighterMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
{
	TArray<FTextRange> LineRanges;
	FTextRange::CalculateLineRangesFromString(SourceString, LineRanges);

	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(LineRanges.Num());

	bool bInBlockComment = false;
	for (const FTextRange& LineRange : LineRanges)
	{
		TSharedRef<FString> LineText = MakeShareable(new FString(SourceString.Mid(LineRange.BeginIndex, LineRange.Len())));
		TArray<TSharedRef<IRun>> Runs;

		ProcessLine(*LineText, Runs, LineText, bInBlockComment);

		if (Runs.Num() == 0)
		{
			Runs.Add(FSlateTextRun::Create(FRunInfo(TEXT("UNotepad.Normal")), LineText, NormalStyle));
		}

		LinesToAdd.Emplace(MoveTemp(LineText), MoveTemp(Runs));
	}

	TargetTextLayout.AddLines(LinesToAdd);
	TargetTextLayout.SetLineHighlights(TArray<FTextLineHighlight>());
}

void FNotepadSyntaxHighlighterMarshaller::BuildStyles(const FSlateFontInfo& InFont)
{
	NormalStyle = FTextBlockStyle()
		.SetFont(InFont)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.87f, 0.91f, 1.0f)));

	KeywordStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.50f, 0.72f, 1.00f, 1.0f)));

	StringStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.88f, 0.56f, 1.0f)));

	NumberStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.72f, 0.45f, 1.0f)));

	CommentStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.50f, 0.42f, 1.0f)));

	PunctuationStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.78f, 0.86f, 1.0f)));

	PreprocessorStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.58f, 0.95f, 1.0f)));

	FunctionStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.86f, 0.94f, 1.0f)));

	DelegateStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.62f, 0.86f, 1.0f)));

	WhitespaceStyle = FTextBlockStyle(NormalStyle)
		.SetColorAndOpacity(FSlateColor(FLinearColor(0.260f, 0.310f, 0.380f, 0.70f)));
}

void FNotepadSyntaxHighlighterMarshaller::ProcessLine(const FString& Line, TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString, bool& bInBlockComment) const
{
	switch (Mode)
	{
		case EUNotepadDocumentMode::Code:
		{
			ProcessCodeLine(Line, Runs, ModelString, bInBlockComment);
			break;
		}

		case EUNotepadDocumentMode::Json:
		{
			ProcessJsonLine(Line, Runs, ModelString);
			break;
		}
		case EUNotepadDocumentMode::Csv:
		{
			ProcessCsvLine(Line, Runs, ModelString);
			break;
		}
		case EUNotepadDocumentMode::Text:
		default:
		{
			AddRun(Runs, ModelString, 0, Line.Len(), NormalStyle, TEXT("UNotepad.Normal"));
			break;
		}
	}
}

void FNotepadSyntaxHighlighterMarshaller::ProcessCodeLine(const FString& Line, TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString, bool& bInBlockComment) const
{
	const FString TrimmedLine = Line.TrimStart();
	if (TrimmedLine.StartsWith(TEXT("#")))
	{
		AddRun(Runs, ModelString, 0, Line.Len(), PreprocessorStyle, TEXT("UNotepad.Preprocessor"));
		return;
	}

	int32 Index = 0;
	while (Index < Line.Len())
	{
		if (bInBlockComment)
		{
			int32 CloseIndex = INDEX_NONE;
			if (Line.FindChar(TEXT('*'), CloseIndex))
			{
				while (CloseIndex != INDEX_NONE && CloseIndex + 1 < Line.Len() && Line[CloseIndex + 1] != TEXT('/'))
				{
					CloseIndex = Line.Find(TEXT("*"), ESearchCase::CaseSensitive, ESearchDir::FromStart, CloseIndex + 1);
				}
			}

			if (CloseIndex != INDEX_NONE && CloseIndex + 1 < Line.Len())
			{
				AddRun(Runs, ModelString, Index, CloseIndex + 2, CommentStyle, TEXT("UNotepad.Comment"));
				Index = CloseIndex + 2;
				bInBlockComment = false;
			}
			else
			{
				AddRun(Runs, ModelString, Index, Line.Len(), CommentStyle, TEXT("UNotepad.Comment"));
				return;
			}
		}
		else if (Index + 1 < Line.Len() && Line[Index] == TEXT('/') && Line[Index + 1] == TEXT('/'))
		{
			AddRun(Runs, ModelString, Index, Line.Len(), CommentStyle, TEXT("UNotepad.Comment"));
			return;
		}
		else if (Index + 1 < Line.Len() && Line[Index] == TEXT('/') && Line[Index + 1] == TEXT('*'))
		{
			int32 CloseIndex = Line.Find(TEXT("*/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index + 2);
			if (CloseIndex == INDEX_NONE)
			{
				AddRun(Runs, ModelString, Index, Line.Len(), CommentStyle, TEXT("UNotepad.Comment"));
				bInBlockComment = true;
				return;
			}

			AddRun(Runs, ModelString, Index, CloseIndex + 2, CommentStyle, TEXT("UNotepad.Comment"));
			Index = CloseIndex + 2;
		}
		else if (Line[Index] == TEXT('"') || Line[Index] == TEXT('\''))
		{
			const int32 EndIndex = ConsumeQuotedString(Line, Index, Line[Index]);
			AddRun(Runs, ModelString, Index, EndIndex, StringStyle, TEXT("UNotepad.String"));
			Index = EndIndex;
		}
		else if (FChar::IsDigit(Line[Index]))
		{
			const int32 EndIndex = ConsumeNumber(Line, Index);
			AddRun(Runs, ModelString, Index, EndIndex, NumberStyle, TEXT("UNotepad.Number"));
			Index = EndIndex;
		}
		else if (IsIdentifierStart(Line[Index]))
		{
			int32 EndIndex = Index + 1;
			while (EndIndex < Line.Len() && IsIdentifierPart(Line[EndIndex]))
			{
				++EndIndex;
			}

			const FString Token = Line.Mid(Index, EndIndex - Index);
			AddRun(Runs, ModelString, Index, EndIndex, GetCodeIdentifierStyle(Token, Line, EndIndex), GetCodeIdentifierRunName(Token, Line, EndIndex));
			Index = EndIndex;
		}
		else if (IsPunctuation(Line[Index]))
		{
			AddRun(Runs, ModelString, Index, Index + 1, PunctuationStyle, TEXT("UNotepad.Punctuation"));
			++Index;
		}
		else
		{
			const int32 StartIndex = Index;
			while (Index < Line.Len() && !IsIdentifierStart(Line[Index]) && !FChar::IsDigit(Line[Index]) && !IsPunctuation(Line[Index]) && Line[Index] != TEXT('"') && Line[Index] != TEXT('\''))
			{
				if (Index + 1 < Line.Len() && Line[Index] == TEXT('/') && (Line[Index + 1] == TEXT('/') || Line[Index + 1] == TEXT('*')))
				{
					break;
				}
				++Index;
			}
			AddRun(Runs, ModelString, StartIndex, Index, NormalStyle, TEXT("UNotepad.Normal"));
		}
	}
}

void FNotepadSyntaxHighlighterMarshaller::ProcessJsonLine(const FString& Line, TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString) const
{
	int32 Index = 0;
	while (Index < Line.Len())
	{
		if (Line[Index] == TEXT('"'))
		{
			const int32 EndIndex = ConsumeQuotedString(Line, Index, TEXT('"'));
			AddRun(Runs, ModelString, Index, EndIndex, StringStyle, TEXT("UNotepad.String"));
			Index = EndIndex;
		}
		else if (FChar::IsDigit(Line[Index]) || Line[Index] == TEXT('-'))
		{
			const int32 EndIndex = ConsumeNumber(Line, Index);
			AddRun(Runs, ModelString, Index, EndIndex, NumberStyle, TEXT("UNotepad.Number"));
			Index = EndIndex;
		}
		else if (IsIdentifierStart(Line[Index]))
		{
			int32 EndIndex = Index + 1;
			while (EndIndex < Line.Len() && IsIdentifierPart(Line[EndIndex]))
			{
				++EndIndex;
			}

			const FString Token = Line.Mid(Index, EndIndex - Index);
			AddRun(Runs, ModelString, Index, EndIndex, IsJsonLiteral(Token) ? KeywordStyle : NormalStyle, IsJsonLiteral(Token) ? TEXT("UNotepad.Keyword") : TEXT("UNotepad.Normal"));
			Index = EndIndex;
		}
		else if (IsPunctuation(Line[Index]))
		{
			AddRun(Runs, ModelString, Index, Index + 1, PunctuationStyle, TEXT("UNotepad.Punctuation"));
			++Index;
		}
		else
		{
			const int32 StartIndex = Index++;
			while (Index < Line.Len() && !IsIdentifierStart(Line[Index]) && !FChar::IsDigit(Line[Index]) && Line[Index] != TEXT('-') && !IsPunctuation(Line[Index]) && Line[Index] != TEXT('"'))
			{
				++Index;
			}
			AddRun(Runs, ModelString, StartIndex, Index, NormalStyle, TEXT("UNotepad.Normal"));
		}
	}
}

void FNotepadSyntaxHighlighterMarshaller::ProcessCsvLine(const FString& Line, TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString) const
{
	int32 Index = 0;
	while (Index < Line.Len())
	{
		if (Line[Index] == TEXT('"'))
		{
			const int32 EndIndex = ConsumeQuotedString(Line, Index, TEXT('"'));
			AddRun(Runs, ModelString, Index, EndIndex, StringStyle, TEXT("UNotepad.String"));
			Index = EndIndex;
		}
		else if (Line[Index] == TEXT(','))
		{
			AddRun(Runs, ModelString, Index, Index + 1, PunctuationStyle, TEXT("UNotepad.Punctuation"));
			++Index;
		}
		else
		{
			const int32 StartIndex = Index++;
			while (Index < Line.Len() && Line[Index] != TEXT(',') && Line[Index] != TEXT('"'))
			{
				++Index;
			}
			AddRun(Runs, ModelString, StartIndex, Index, NormalStyle, TEXT("UNotepad.Normal"));
		}
	}
}

void FNotepadSyntaxHighlighterMarshaller::AddRun(TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString, int32 BeginIndex, int32 EndIndex, const FTextBlockStyle& Style, const TCHAR* RunName) const
{
	if (EndIndex <= BeginIndex)
	{
		return;
	}

	int32 SegmentBeginIndex = BeginIndex;
	for (int32 Index = BeginIndex; Index < EndIndex; ++Index)
	{
		const TCHAR Character = (*ModelString)[Index];
		const bool bIsVisibleSpace = bShowWhitespace && Character == TEXT(' ');
		const bool bIsTab = Character == TEXT('\t');

		if (!bIsVisibleSpace && !bIsTab)
		{
			continue;
		}

		if (SegmentBeginIndex < Index)
		{
			Runs.Add(FSlateTextRun::Create(FRunInfo(RunName), ModelString, Style, FTextRange(SegmentBeginIndex, Index)));
		}

		if (bIsTab)
		{
			const int32 Column = CalculateVisualColumnBeforeIndex(*ModelString, Index);
			const int32 SpacesToNextTab = TabSize - (Column % TabSize);
			Runs.Add(FNotepadWhitespaceTextRun::Create(
				FRunInfo(TEXT("UNotepad.Tab")),
				ModelString,
				WhitespaceStyle,
				FTextRange(Index, Index + 1),
				SpacesToNextTab,
				bShowWhitespace ? FString(TEXT("\u2192")) : FString(),
				false));
		}
		else
		{
			Runs.Add(FNotepadWhitespaceTextRun::Create(
				FRunInfo(TEXT("UNotepad.Space")),
				ModelString,
				WhitespaceStyle,
				FTextRange(Index, Index + 1),
				1,
				FString(TEXT("\u00B7")),
				true));
		}

		SegmentBeginIndex = Index + 1;
	}

	if (SegmentBeginIndex < EndIndex)
	{
		Runs.Add(FSlateTextRun::Create(FRunInfo(RunName), ModelString, Style, FTextRange(SegmentBeginIndex, EndIndex)));
	}
}

const FTextBlockStyle& FNotepadSyntaxHighlighterMarshaller::GetCodeIdentifierStyle(const FString& Token, const FString& Line, int32 EndIndex) const
{
	if (IsDelegateIdentifier(Token))
	{
		return DelegateStyle;
	}

	if (!IsCodeKeyword(Token) && IsFunctionIdentifier(Line, EndIndex))
	{
		return FunctionStyle;
	}

	return IsCodeKeyword(Token) ? KeywordStyle : NormalStyle;
}

const TCHAR* FNotepadSyntaxHighlighterMarshaller::GetCodeIdentifierRunName(const FString& Token, const FString& Line, int32 EndIndex) const
{
	if (IsDelegateIdentifier(Token))
	{
		return TEXT("UNotepad.Delegate");
	}

	if (!IsCodeKeyword(Token) && IsFunctionIdentifier(Line, EndIndex))
	{
		return TEXT("UNotepad.Function");
	}

	return IsCodeKeyword(Token) ? TEXT("UNotepad.Keyword") : TEXT("UNotepad.Normal");
}

bool FNotepadSyntaxHighlighterMarshaller::IsIdentifierStart(TCHAR Character)
{
	return FChar::IsAlpha(Character) || Character == TEXT('_');
}

bool FNotepadSyntaxHighlighterMarshaller::IsIdentifierPart(TCHAR Character)
{
	return FChar::IsAlnum(Character) || Character == TEXT('_');
}

bool FNotepadSyntaxHighlighterMarshaller::IsPunctuation(TCHAR Character)
{
	static const FString Punctuation = TEXT("{}[]():;,.+-*/%=!<>|&?");
	return Punctuation.Contains(FString::Chr(Character));
}

bool FNotepadSyntaxHighlighterMarshaller::IsJsonLiteral(const FString& Token)
{
	return Token == TEXT("true") || Token == TEXT("false") || Token == TEXT("null");
}

bool FNotepadSyntaxHighlighterMarshaller::IsCodeKeyword(const FString& Token)
{
	static const TSet<FString> Keywords = {
		TEXT("alignas"), TEXT("alignof"), TEXT("auto"), TEXT("bool"), TEXT("break"), TEXT("case"), TEXT("catch"), TEXT("char"),
		TEXT("class"), TEXT("const"), TEXT("constexpr"), TEXT("continue"), TEXT("decltype"), TEXT("default"), TEXT("delete"),
		TEXT("do"), TEXT("double"), TEXT("else"), TEXT("enum"), TEXT("explicit"), TEXT("extern"), TEXT("false"), TEXT("float"),
		TEXT("for"), TEXT("friend"), TEXT("if"), TEXT("inline"), TEXT("int"), TEXT("long"), TEXT("mutable"), TEXT("namespace"),
		TEXT("new"), TEXT("nullptr"), TEXT("private"), TEXT("protected"), TEXT("public"), TEXT("return"), TEXT("short"),
		TEXT("signed"), TEXT("sizeof"), TEXT("static"), TEXT("struct"), TEXT("switch"), TEXT("template"), TEXT("this"),
		TEXT("throw"), TEXT("true"), TEXT("try"), TEXT("typedef"), TEXT("typename"), TEXT("uint8"), TEXT("uint16"),
		TEXT("uint32"), TEXT("uint64"), TEXT("int8"), TEXT("int16"), TEXT("int32"), TEXT("int64"), TEXT("using"),
		TEXT("virtual"), TEXT("void"), TEXT("while"), TEXT("UCLASS"), TEXT("USTRUCT"), TEXT("UENUM"), TEXT("UFUNCTION"),
		TEXT("UPROPERTY"), TEXT("GENERATED_BODY"), TEXT("TEXT"), TEXT("LOCTEXT"), TEXT("override"), TEXT("final")
	};

	return Keywords.Contains(Token);
}

bool FNotepadSyntaxHighlighterMarshaller::IsDelegateIdentifier(const FString& Token)
{
	return (Token.StartsWith(TEXT("DECLARE_")) && Token.Contains(TEXT("DELEGATE"))) || Token.Contains(TEXT("Delegate")) || Token.Contains(TEXT("DELEGATE"));
}

bool FNotepadSyntaxHighlighterMarshaller::IsFunctionIdentifier(const FString& Line, int32 EndIndex)
{
	int32 Index = EndIndex;
	while (Index < Line.Len() && FChar::IsWhitespace(Line[Index]))
	{
		++Index;
	}

	return Index < Line.Len() && Line[Index] == TEXT('(');
}

int32 FNotepadSyntaxHighlighterMarshaller::CalculateVisualColumnBeforeIndex(const FString& Line, int32 EndIndex) const
{
	int32 Column = 0;
	const int32 SafeEndIndex = FMath::Clamp(EndIndex, 0, Line.Len());

	for (int32 Index = 0; Index < SafeEndIndex; ++Index)
	{
		if (Line[Index] == TEXT('\t'))
		{
			Column += TabSize - (Column % TabSize);
		}
		else
		{
			++Column;
		}
	}

	return Column;
}

int32 FNotepadSyntaxHighlighterMarshaller::ConsumeQuotedString(const FString& Line, int32 StartIndex, TCHAR QuoteChar)
{
	int32 Index = StartIndex + 1;
	while (Index < Line.Len())
	{
		if (Line[Index] == TEXT('\\'))
		{
			Index += 2;
			continue;
		}

		if (Line[Index] == QuoteChar)
		{
			return Index + 1;
		}

		++Index;
	}

	return Line.Len();
}

int32 FNotepadSyntaxHighlighterMarshaller::ConsumeNumber(const FString& Line, int32 StartIndex)
{
	int32 Index = StartIndex;
	if (Index < Line.Len() && Line[Index] == TEXT('-'))
	{
		++Index;
	}

	while (Index < Line.Len() && (FChar::IsDigit(Line[Index]) || Line[Index] == TEXT('.') || Line[Index] == TEXT('x') || Line[Index] == TEXT('X') || Line[Index] == TEXT('e') || Line[Index] == TEXT('E') || Line[Index] == TEXT('+') || Line[Index] == TEXT('-')))
	{
		++Index;
	}

	return FMath::Max(Index, StartIndex + 1);
}
