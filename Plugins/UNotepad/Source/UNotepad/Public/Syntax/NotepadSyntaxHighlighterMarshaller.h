// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/PlainTextLayoutMarshaller.h"
#include "Styling/SlateTypes.h"
#include "Types/NotepadDocumentTypes.h"

class UNOTEPAD_API FNotepadSyntaxHighlighterMarshaller : public FPlainTextLayoutMarshaller
{
public:
	static TSharedRef<FNotepadSyntaxHighlighterMarshaller> Create(EUNotepadDocumentMode InMode, const FSlateFontInfo& InFont);

	void SetMode(EUNotepadDocumentMode InMode);
	void SetShowWhitespace(bool bInShowWhitespace);
	void SetTabSize(int32 InTabSize);

	virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
	virtual bool RequiresLiveUpdate() const override;

private:
	explicit FNotepadSyntaxHighlighterMarshaller(EUNotepadDocumentMode InMode, const FSlateFontInfo& InFont);

	void BuildStyles(const FSlateFontInfo& InFont);
	void ProcessLine(const FString& Line, TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString, bool& bInBlockComment) const;
	void ProcessCodeLine(const FString& Line, TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString, bool& bInBlockComment) const;
	void ProcessJsonLine(const FString& Line, TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString) const;
	void ProcessCsvLine(const FString& Line, TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString) const;

	void AddRun(TArray<TSharedRef<IRun>>& Runs, const TSharedRef<FString>& ModelString, int32 BeginIndex, int32 EndIndex, const FTextBlockStyle& Style, const TCHAR* RunName) const;
	const FTextBlockStyle& GetCodeIdentifierStyle(const FString& Token, const FString& Line, int32 EndIndex) const;
	const TCHAR* GetCodeIdentifierRunName(const FString& Token, const FString& Line, int32 EndIndex) const;

	static bool IsIdentifierStart(TCHAR Character);
	static bool IsIdentifierPart(TCHAR Character);
	static bool IsPunctuation(TCHAR Character);
	static bool IsJsonLiteral(const FString& Token);
	static bool IsCodeKeyword(const FString& Token);
	static bool IsDelegateIdentifier(const FString& Token);
	static bool IsFunctionIdentifier(const FString& Line, int32 EndIndex);
	int32 CalculateVisualColumnBeforeIndex(const FString& Line, int32 EndIndex) const;
	static int32 ConsumeQuotedString(const FString& Line, int32 StartIndex, TCHAR QuoteChar);
	static int32 ConsumeNumber(const FString& Line, int32 StartIndex);

	EUNotepadDocumentMode Mode = EUNotepadDocumentMode::Text;

	FTextBlockStyle NormalStyle;
	FTextBlockStyle KeywordStyle;
	FTextBlockStyle StringStyle;
	FTextBlockStyle NumberStyle;
	FTextBlockStyle CommentStyle;
	FTextBlockStyle PunctuationStyle;
	FTextBlockStyle PreprocessorStyle;
	FTextBlockStyle FunctionStyle;
	FTextBlockStyle DelegateStyle;
	FTextBlockStyle WhitespaceStyle;
	int32 TabSize = 4;
	bool bShowWhitespace = true;
};
