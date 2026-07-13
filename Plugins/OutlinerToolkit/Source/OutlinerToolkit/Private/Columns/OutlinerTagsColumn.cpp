// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerTagsColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "ISceneOutlinerTreeItem.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OutlinerTagsColumn"

SHeaderRow::FColumn::FArguments FOutlinerTagsColumn::ConstructHeaderRowColumn()
{
	return	SHeaderRow::Column(GetColumnID())
			.FillWidth(0.25f)
			.HAlignHeader(HAlign_Center)
			.VAlignHeader(VAlign_Center)
			.HAlignCell(HAlign_Left)
			.VAlignCell(VAlign_Center)
			.OnSort(this, &FOutlinerTagsColumn::OnColumnSortModeChanged)
			.SortMode(TAttribute<EColumnSortMode::Type>::Create(TAttribute<EColumnSortMode::Type>::FGetter::CreateSP(this, &FOutlinerTagsColumn::GetColumnSortMode)))
			.DefaultLabel(LOCTEXT("TagsLabel","Tags"))
			[
				SNew(STextBlock)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Text(LOCTEXT("TagsLog", "Tags"))
			];
}

const TSharedRef<SWidget> FOutlinerTagsColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	AActor* Actor = OutlinerColumnUtils::ResolveActor(TreeItem);
	if(!Actor)
	{
		return SNullWidget::NullWidget;
	}

	TWeakObjectPtr<AActor> WeakActor = Actor;

	return	SNew(SEditableTextBox)
			.SelectAllTextWhenFocused(false)
			.ClearKeyboardFocusOnCommit(true)
			.MinDesiredWidth(160.0f)
			.Text(TAttribute<FText>::CreateSP(this, &FOutlinerTagsColumn::GetTagText, WeakActor))
			.OnTextCommitted(FOnTextCommitted::CreateSP(this, &FOutlinerTagsColumn::OnTextCommited, WeakActor))
			.ToolTipText(LOCTEXT("EditTagsToolTip","Edit actor tags as a comma-separated list."));
}

void FOutlinerTagsColumn::SortItems(TArray<FSceneOutlinerTreeItemPtr>& InOutItems, const EColumnSortMode::Type InSortMode) const
{
	if(InSortMode == EColumnSortMode::None)
	{
		return;
	}

	InOutItems.Sort([this, InSortMode](const FSceneOutlinerTreeItemPtr& A, const FSceneOutlinerTreeItemPtr& B)
		{
			const FString AText = GetTagsString(A.ToSharedRef());
			const FString BText = GetTagsString(B.ToSharedRef());

			return InSortMode == EColumnSortMode::Ascending ? AText < BText : AText > BText;
		});
}

FString FOutlinerTagsColumn::GetTagsString(FSceneOutlinerTreeItemRef TreeItem) const
{
	return OutlinerColumnUtils::JoinActorTags(OutlinerColumnUtils::ResolveActor(TreeItem));
}

EColumnSortMode::Type FOutlinerTagsColumn::GetColumnSortMode() const
{
	return CurrentSortMode;
}

void FOutlinerTagsColumn::OnColumnSortModeChanged(EColumnSortPriority::Type PriorityType, const FName& Name, EColumnSortMode::Type SortMode)
{
	CurrentSortMode = SortMode;

	if(WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->RequestSort();
		WeakSceneOutliner.Pin()->FullRefresh();
	}
}

FText FOutlinerTagsColumn::GetTagText(TWeakObjectPtr<AActor> WeakActor) const
{
	if (WeakActor.IsValid())
	{
		return FText::FromString(OutlinerColumnUtils::JoinActorTags(WeakActor.Get()));
	}

	return FText::GetEmpty();
}

void FOutlinerTagsColumn::OnTextCommited(const FText& NewText, ETextCommit::Type CommitType, TWeakObjectPtr<AActor> WeakActor)
{
	if (CommitType == ETextCommit::Default)
	{
		return;
	}

	if (WeakActor.IsValid())
	{
		AActor* CurrentActor = WeakActor.Get();
		const FScopedTransaction Transaction(LOCTEXT("EditActorTagsTransaction", "Edit Actor Tags"));
		CurrentActor->Modify();
		CurrentActor->Tags = OutlinerColumnUtils::ParseTagsString(NewText.ToString());

		if (WeakSceneOutliner.IsValid())
		{
			WeakSceneOutliner.Pin()->FullRefresh();
		}
	}
}

#undef LOCTEXT_NAMESPACE
