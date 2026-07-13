// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SNotepadToolbar.h"

#include "Services/NotepadDocumentUtils.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace UNotepad::Toolbar
{
	const FLinearColor PanelColor(0.055f, 0.060f, 0.070f, 1.0f);
	const FLinearColor MutedTextColor(0.520f, 0.570f, 0.650f, 1.0f);
	constexpr float ButtonPaddingX = 8.0f;
	constexpr float ButtonPaddingY = 3.0f;
}

void SNotepadToolbar::Construct(const FArguments& InArgs)
{
	OnNew = InArgs._OnNew;
	OnOpen = InArgs._OnOpen;
	OnSave = InArgs._OnSave;
	OnSaveAs = InArgs._OnSaveAs;
	OnValidate = InArgs._OnValidate;
	OnFormat = InArgs._OnFormat;
	OnCompile = InArgs._OnCompile;
	OnToggleLineNumbers = InArgs._OnToggleLineNumbers;
	OnToggleWhitespace = InArgs._OnToggleWhitespace;
	OnClose = InArgs._OnClose;
	OnModeChanged = InArgs._OnModeChanged;

	ModeOptions.Add(MakeShared<FString>(FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Text)));
	ModeOptions.Add(MakeShared<FString>(FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Code)));
	ModeOptions.Add(MakeShared<FString>(FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Json)));
	ModeOptions.Add(MakeShared<FString>(FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Csv)));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::Toolbar::PanelColor)
		.Padding(FMargin(8.0f, 5.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildToolbarButton(NSLOCTEXT("UNotepadToolbar", "NewButton", "New"), TEXT("Icons.Plus"), OnNew)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildToolbarButton(NSLOCTEXT("UNotepadToolbar", "OpenButton", "Open"), TEXT("Icons.FolderOpen"), OnOpen)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildToolbarButton(NSLOCTEXT("UNotepadToolbar", "SaveButton", "Save"), TEXT("Icons.Save"), OnSave)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				BuildToolbarButton(NSLOCTEXT("UNotepadToolbar", "SaveAsButton", "Save As"), TEXT("Icons.SaveModified"), OnSaveAs)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("UNotepadToolbar", "ModeLabel", "Mode"))
				.ColorAndOpacity(UNotepad::Toolbar::MutedTextColor)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(110.0f)
				[
					SAssignNew(ModeComboBox, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&ModeOptions)
					.OnGenerateWidget(this, &SNotepadToolbar::GenerateModeComboItem)
					.OnSelectionChanged(this, &SNotepadToolbar::OnModeSelectionChanged)
					[
						SNew(STextBlock).Text(this, &SNotepadToolbar::GetActiveModeText)
					]
				]
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildToolbarButton(NSLOCTEXT("UNotepadToolbar", "ValidateButton", "Validate"), TEXT("Icons.Check"), OnValidate)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildToolbarButton(NSLOCTEXT("UNotepadToolbar", "FormatButton", "Format"), TEXT("Icons.Edit"), OnFormat)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				BuildToolbarButton(NSLOCTEXT("UNotepadToolbar", "CompileButton", "Compile"), TEXT("Icons.Check"), OnCompile)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildToolbarButton(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SNotepadToolbar::GetLineNumbersButtonText)), TEXT("Icons.Edit"), OnToggleLineNumbers)
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				BuildToolbarButton(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SNotepadToolbar::GetWhitespaceButtonText)), TEXT("Icons.EyeDropper"), OnToggleWhitespace)
			]

			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]

			+ SHorizontalBox::Slot().AutoWidth()
			[
				BuildToolbarButton(NSLOCTEXT("UNotepadToolbar", "CloseTabButton", "Close"), TEXT("Icons.X"), OnClose)
			]
		]
	];
}

void SNotepadToolbar::SetMode(EUNotepadDocumentMode NewMode)
{
	ActiveMode = NewMode;
	if (ModeComboBox.IsValid())
	{
		bIsChangingModeSelection = true;
		ModeComboBox->SetSelectedItem(FindModeOption(ActiveMode));
		bIsChangingModeSelection = false;
	}
}

void SNotepadToolbar::SetLineNumbersEnabled(bool bEnabled)
{
	bLineNumbersEnabled = bEnabled;
}

void SNotepadToolbar::SetWhitespaceEnabled(bool bEnabled)
{
	bWhitespaceEnabled = bEnabled;
}

FReply SNotepadToolbar::ExecuteAction(FNotepadToolbarAction Action)
{
	Action.ExecuteIfBound();
	return FReply::Handled();
}

TSharedRef<SWidget> SNotepadToolbar::BuildToolbarButton(TAttribute<FText> Label, FName IconName, FNotepadToolbarAction Action)
{
	return SNew(SButton)
		.ContentPadding(FMargin(UNotepad::Toolbar::ButtonPaddingX, UNotepad::Toolbar::ButtonPaddingY))
		.OnClicked(this, &SNotepadToolbar::ExecuteAction, Action)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(IconName))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Label)
			]
		];
}

TSharedRef<SWidget> SNotepadToolbar::GenerateModeComboItem(TSharedPtr<FString> Item) const
{
	return SNew(STextBlock)
		.Text(FText::FromString(Item.IsValid() ? *Item : TEXT("")));
}

void SNotepadToolbar::OnModeSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (bIsChangingModeSelection || !NewSelection.IsValid())
	{
		return;
	}

	ActiveMode = GetModeFromOption(NewSelection);
	OnModeChanged.ExecuteIfBound(ActiveMode);
}

FText SNotepadToolbar::GetActiveModeText() const
{
	return FText::FromString(FNotepadDocumentUtils::GetModeLabel(ActiveMode));
}

FText SNotepadToolbar::GetLineNumbersButtonText() const
{
	return bLineNumbersEnabled ? NSLOCTEXT("UNotepadToolbar", "LinesOnButton", "Lines On") : NSLOCTEXT("UNotepadToolbar", "LinesOffButton", "Lines Off");
}

FText SNotepadToolbar::GetWhitespaceButtonText() const
{
	return bWhitespaceEnabled ? NSLOCTEXT("UNotepadToolbar", "WhitespaceOnButton", "Spaces On") : NSLOCTEXT("UNotepadToolbar", "WhitespaceOffButton", "Spaces Off");
}

TSharedPtr<FString> SNotepadToolbar::FindModeOption(EUNotepadDocumentMode Mode) const
{
	const FString ModeLabel = FNotepadDocumentUtils::GetModeLabel(Mode);
	for (const TSharedPtr<FString>& Option : ModeOptions)
	{
		if (Option.IsValid() && *Option == ModeLabel)
		{
			return Option;
		}
	}

	return nullptr;
}

EUNotepadDocumentMode SNotepadToolbar::GetModeFromOption(const TSharedPtr<FString>& Option) const
{
	if (!Option.IsValid())
	{
		return EUNotepadDocumentMode::Text;
	}

	if (*Option == FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Code))
	{
		return EUNotepadDocumentMode::Code;
	}

	if (*Option == FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Json))
	{
		return EUNotepadDocumentMode::Json;
	}

	if (*Option == FNotepadDocumentUtils::GetModeLabel(EUNotepadDocumentMode::Csv))
	{
		return EUNotepadDocumentMode::Csv;
	}

	return EUNotepadDocumentMode::Text;
}
