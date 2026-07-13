// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SNotepadTabStrip.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace UNotepad::Tabs
{
	const FLinearColor PanelColor(0.055f, 0.060f, 0.070f, 1.0f);
	const FLinearColor RaisedPanelColor(0.075f, 0.083f, 0.095f, 1.0f);
	const FLinearColor ActiveTabColor(0.120f, 0.135f, 0.160f, 1.0f);
	const FLinearColor TextColor(0.830f, 0.860f, 0.900f, 1.0f);
	const FLinearColor MutedTextColor(0.520f, 0.570f, 0.650f, 1.0f);
}

void SNotepadTabStrip::Construct(const FArguments& InArgs)
{
	OnDocumentSelected = InArgs._OnDocumentSelected;
	OnDocumentClosed = InArgs._OnDocumentClosed;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::Tabs::RaisedPanelColor)
		.Padding(FMargin(4.0f, 4.0f, 4.0f, 0.0f))
		[
			SAssignNew(TabStrip, SScrollBox)
			.Orientation(Orient_Horizontal)
		]
	];
}

void SNotepadTabStrip::SetDocuments(const TArray<TSharedPtr<FNotepadDocument>>& InDocuments, TSharedPtr<FNotepadDocument> InActiveDocument)
{
	Documents = InDocuments;
	ActiveDocument = InActiveDocument;
	Rebuild();
}

void SNotepadTabStrip::Rebuild()
{
	if (!TabStrip.IsValid())
	{
		return;
	}

	TabStrip->ClearChildren();

	for (const TSharedPtr<FNotepadDocument>& Document : Documents)
	{
		if (!Document.IsValid())
		{
			continue;
		}

		const bool bIsActive = Document == ActiveDocument;
		const FLinearColor TabColor = bIsActive ? UNotepad::Tabs::ActiveTabColor : UNotepad::Tabs::PanelColor;

		TabStrip->AddSlot()
		.Padding(FMargin(0.0f, 0.0f, 2.0f, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(TabColor)
			.Padding(FMargin(7.0f, 3.0f))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ContentPadding(FMargin(4.0f, 1.0f))
					.ToolTipText(FText::FromString(Document->FilePath.IsEmpty() ? Document->DisplayName : Document->FilePath))
					.OnClicked(this, &SNotepadTabStrip::SelectDocument, Document)
					[
						SNew(STextBlock)
						.Text_Lambda([Document]()
						{
							return FText::FromString((Document->bDirty ? TEXT("*") : TEXT("")) + Document->DisplayName);
						})
						.ColorAndOpacity(UNotepad::Tabs::TextColor)
					]
				]

				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ContentPadding(FMargin(4.0f, 1.0f))
					.OnClicked(this, &SNotepadTabStrip::CloseDocument, Document)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("x")))
						.ColorAndOpacity(UNotepad::Tabs::MutedTextColor)
					]
				]
			]
		];
	}
}

FReply SNotepadTabStrip::SelectDocument(TSharedPtr<FNotepadDocument> Document)
{
	OnDocumentSelected.ExecuteIfBound(Document);
	return FReply::Handled();
}

FReply SNotepadTabStrip::CloseDocument(TSharedPtr<FNotepadDocument> Document)
{
	OnDocumentClosed.ExecuteIfBound(Document);
	return FReply::Handled();
}
