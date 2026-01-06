// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SFolderItemTree.h"


void SFolderItemTree::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
	Item = InArgs._Item;
	HighlightText = InArgs._HighlightText;


	SMultiColumnTableRow::Construct(
		SMultiColumnTableRow::FArguments()
		.Padding(FMargin{ 0.0f, 2.0f, 0.0f, 0.0f }),
		InTable
	);
}

TSharedRef<SWidget> SFolderItemTree::GenerateWidgetForColumn(const FName& InColumnName)
{
	TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox).ToolTipText(FText::FromString(Item->FolderPath));
	HorizontalBox->AddSlot().AutoWidth().Padding(2.0f)
	[
		SNew(SExpanderArrow, SharedThis(this))
		.IndentAmount(10.0f)
		.ShouldDrawWires(false)
	];

	HorizontalBox->AddSlot().AutoWidth().Padding(0, 0, 2, 0).VAlign(VAlign_Center)
	[
		SNew(SImage)
		.Image(GetFolderIcon())
		.ColorAndOpacity(FColor::White)
	];

	HorizontalBox->AddSlot().AutoWidth().Padding(2.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Item->FolderName))
		.HighlightText(HighlightText)
	];

	return HorizontalBox;
		/*SNew(SHorizontalBox)
			.ToolTipText(FText::FromString(Item->FolderPath))

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin{ 2.0f })
		[
			SNew(SExpanderArrow, SharedThis(this))
				.IndentAmount(10)
				.ShouldDrawWires(false)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 2, 0)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
				.Image(GetFolderIcon())
				.ColorAndOpacity(FColor::White)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin{ 2.0f })
		[
			SNew(STextBlock)
				.Text(FText::FromString(Item->FolderName))
				.HighlightText(HighlightText)
		];*/
}

const FSlateBrush* SFolderItemTree::GetFolderIcon() const
{
	return FAppStyle::GetBrush(Item->bIsExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
}

FSlateColor SFolderItemTree::GetFolderColor() const
{
	return FSlateColor(FColor::White);
}