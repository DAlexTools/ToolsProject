// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "UI/SFolderItemTree.h"

#define LOCTEXT_NAMESPACE "SFolderItemTree"

void SFolderItemTree::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable)
{
        Item = InArgs._Item;
        HighlightText = InArgs._HightlightText;

        check(Item.IsValid());

        SMultiColumnTableRow::Construct(SMultiColumnTableRow::FArguments()
                .Padding(FMargin{ 0.0f, 2.0f, 0.0f, 0.0f }), InTable);
}

/* clang-format off */
TSharedRef<SWidget> SFolderItemTree::GenerateWidgetForColumn(const FName& InColumnName)
{
        if (!Item.IsValid())
        {
                return SNullWidget::NullWidget;
        }

        TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox).ToolTipText(FText::FromString(Item->FolderPath))
                
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
                ];

        return HorizontalBox;
                
}
/* clang-format on */

const FSlateBrush* SFolderItemTree::GetFolderIcon() const
{
        if (!Item.IsValid())
        {
                return nullptr;
        }

        return FAppStyle::GetBrush(Item->bIsExpanded ? TEXT("ContentBrowser.AssetTreeFolderOpen") : TEXT("ContentBrowser.AssetTreeFolderClosed"));
}

FSlateColor SFolderItemTree::GetFolderColor() const
{
        if (!Item.IsValid())
        {
                return FSlateColor(FColor::White);
        }

        /*
         * TODO: Add visual state for folders where asset creation is disallowed
         * or where the current project workflow should warn the user.
         */
        return FSlateColor(FColor::White); 
}

#undef LOCTEXT_NAMESPACE
