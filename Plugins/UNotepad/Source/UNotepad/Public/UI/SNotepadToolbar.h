// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Types/NotepadDocumentTypes.h"
#include "Widgets/SCompoundWidget.h"

template <typename OptionType>
class SComboBox;

DECLARE_DELEGATE(FNotepadToolbarAction);
DECLARE_DELEGATE_OneParam(FNotepadToolbarModeChanged, EUNotepadDocumentMode);

class UNOTEPAD_API SNotepadToolbar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNotepadToolbar) {}
		SLATE_EVENT(FNotepadToolbarAction, OnNew)
		SLATE_EVENT(FNotepadToolbarAction, OnOpen)
		SLATE_EVENT(FNotepadToolbarAction, OnSave)
		SLATE_EVENT(FNotepadToolbarAction, OnSaveAs)
		SLATE_EVENT(FNotepadToolbarAction, OnValidate)
		SLATE_EVENT(FNotepadToolbarAction, OnFormat)
		SLATE_EVENT(FNotepadToolbarAction, OnCompile)
		SLATE_EVENT(FNotepadToolbarAction, OnToggleLineNumbers)
		SLATE_EVENT(FNotepadToolbarAction, OnToggleWhitespace)
		SLATE_EVENT(FNotepadToolbarAction, OnClose)
		SLATE_EVENT(FNotepadToolbarModeChanged, OnModeChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetMode(EUNotepadDocumentMode NewMode);
	void SetLineNumbersEnabled(bool bEnabled);
	void SetWhitespaceEnabled(bool bEnabled);

private:
	FReply ExecuteAction(FNotepadToolbarAction Action);
	TSharedRef<SWidget> BuildToolbarButton(TAttribute<FText> Label, FName IconName, FNotepadToolbarAction Action);
	TSharedRef<SWidget> GenerateModeComboItem(TSharedPtr<FString> Item) const;
	void OnModeSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	FText GetActiveModeText() const;
	FText GetLineNumbersButtonText() const;
	FText GetWhitespaceButtonText() const;
	TSharedPtr<FString> FindModeOption(EUNotepadDocumentMode Mode) const;
	EUNotepadDocumentMode GetModeFromOption(const TSharedPtr<FString>& Option) const;

	FNotepadToolbarAction OnNew;
	FNotepadToolbarAction OnOpen;
	FNotepadToolbarAction OnSave;
	FNotepadToolbarAction OnSaveAs;
	FNotepadToolbarAction OnValidate;
	FNotepadToolbarAction OnFormat;
	FNotepadToolbarAction OnCompile;
	FNotepadToolbarAction OnToggleLineNumbers;
	FNotepadToolbarAction OnToggleWhitespace;
	FNotepadToolbarAction OnClose;
	FNotepadToolbarModeChanged OnModeChanged;

	TArray<TSharedPtr<FString>> ModeOptions;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ModeComboBox;
	EUNotepadDocumentMode ActiveMode = EUNotepadDocumentMode::Text;
	bool bLineNumbersEnabled = true;
	bool bWhitespaceEnabled = true;
	bool bIsChangingModeSelection = false;
};
