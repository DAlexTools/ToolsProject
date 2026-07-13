// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "Styling/SlateTypes.h"
#include "Types/NotepadDocumentTypes.h"
#include "Widgets/SCompoundWidget.h"

class FNotepadSyntaxHighlighterMarshaller;
class SMultiLineEditableTextBox;
class SNotepadSyncedText;
class SScrollBar;
class SWidget;

enum class ENotepadLineEnding : uint8;

class UNOTEPAD_API SNotepadEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNotepadEditor) {}
		SLATE_EVENT(FOnTextChanged, OnTextChanged)
		SLATE_EVENT(FOnKeyDown, OnShortcutKeyDown)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	void SetText(const FString& NewText);
	FString GetText() const;
	void SetMode(EUNotepadDocumentMode NewMode);
	void FocusEditor();
	void SetShowLineNumbers(bool bShow);
	void SetShowWhitespace(bool bShow);
	void SetTabSize(int32 InTabSize);
	bool IsShowingLineNumbers() const;
	bool IsShowingWhitespace() const;
	int32 GetLineCount() const;
	bool GoToLine(int32 LineNumber);
	void BeginSearch(const FString& SearchText, ESearchCase::Type SearchCase, bool bReverse);
	void AdvanceSearch(bool bReverse);
	void ClearSearch();
	FString GetSelectedText() const;
	bool IsSelectedTextMatching(const FString& SearchText, ESearchCase::Type SearchCase) const;
	bool ReplaceSelectedText(const FString& SearchText, const FString& ReplacementText, ESearchCase::Type SearchCase);
	int32 ReplaceAllText(const FString& SearchText, const FString& ReplacementText, ESearchCase::Type SearchCase);
	bool ToggleLineComment(const FString& CommentPrefix);
	bool DuplicateLineOrSelection();
	bool MoveCurrentLine(int32 Direction);
	bool TrimTrailingWhitespace();
	bool ConvertTabsToSpaces();
	bool ConvertSpacesToTabs();
	bool EnsureFinalNewline();
	bool NormalizeLineEndings(ENotepadLineEnding LineEnding);

private:
	FReply HandleEditorKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	FReply ForwardMouseWheelToEditor(const FPointerEvent& MouseEvent);
	void HandleTextChanged(const FText& NewText);
	void HandleEditorHorizontalScroll(float ScrollOffsetFraction);
	void HandleEditorVerticalScroll(float ScrollOffsetFraction);
	void UpdateDecorations(const FString& Text);
	void SyncDecorationScroll();
	EVisibility GetLineNumberVisibility() const;

	FOnTextChanged OnTextChanged;
	FOnKeyDown OnShortcutKeyDown;

	TSharedPtr<SMultiLineEditableTextBox> EditorTextBox;
	TSharedPtr<SNotepadSyncedText> LineNumberTextBox;
	TSharedPtr<SScrollBar> VerticalScrollBar;
	TSharedPtr<SScrollBar> HorizontalScrollBar;
	TSharedPtr<FNotepadSyntaxHighlighterMarshaller> SyntaxMarshaller;

	FSlateFontInfo EditorFontInfo;
	FTextBlockStyle LineNumberTextStyle;
	EUNotepadDocumentMode Mode = EUNotepadDocumentMode::Text;
	int32 TabSize = 4;
	bool bShowLineNumbers = true;
	bool bShowWhitespace = true;
	bool bIsSettingText = false;
	bool bIsSyncingDecorationScroll = false;
	float LastHorizontalScrollFraction = 0.0f;
	float LastVerticalScrollFraction = 0.0f;
};
