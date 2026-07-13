// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerHiddenInGameColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OutlinerHiddenInGameColumn"

namespace OutlinerHiddenInGameColumn
{
	static const FSlateIcon HiddenInGameIcon = FSlateIcon(FName("AutomationWindowStyle"), "Automation.InProcess");
	static const FLinearColor DefaultColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
	static const FLinearColor CheckedColor = FLinearColor(1.0f, 0.3f, 0.3f, 1.0f);
	static const FLinearColor UndeterminedColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
	static const FLinearColor NoValidColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.35f);
}


FOutlinerHiddenInGameColumn::FOutlinerHiddenInGameColumn(ISceneOutliner& SceneOutliner)
	: FOutlinerToolkitColumnBase(SceneOutliner)
{
}

FName FOutlinerHiddenInGameColumn::GetColumnID()
{
	return GetID();
}

SHeaderRow::FColumn::FArguments FOutlinerHiddenInGameColumn::ConstructHeaderRowColumn()
{
	return	SHeaderRow::Column(GetColumnID())
		.FixedWidth(30.0f)
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		[
			SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
						.Image(OutlinerHiddenInGameColumn::HiddenInGameIcon.GetIcon())
						.ColorAndOpacity(FSlateColor::UseForeground())
						.ToolTipText(LOCTEXT("HiddenInGameToolTip", "Hidden In Game"))
				]
		];
}

bool FOutlinerHiddenInGameColumn::SupportsSorting() const
{
	return false;
}

const TSharedRef<SWidget> FOutlinerHiddenInGameColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	AActor* Actor = OutlinerColumnUtils::ResolveActor(TreeItem);
	if (!Actor)
	{
		return SNullWidget::NullWidget;
	}

	const TArray<USceneComponent*> SceneComponents = OutlinerColumnUtils::GetActorComponents<USceneComponent>(Actor);
	if (SceneComponents.IsEmpty())
	{
		return SNullWidget::NullWidget;
	}

	TWeakObjectPtr<AActor> WeakActor = Actor;

	return	SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(0.0f)
		.ToolTipText(LOCTEXT("ToggleToolTipText", "Toggle whether all scene components on this actor are hidden during gameplay."))
		.OnClicked(FOnClicked::CreateSP(this, &FOutlinerHiddenInGameColumn::GetHiddenInGameColumnButtonClicked, WeakActor))
		[
			SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
						.Image(TAttribute<const FSlateBrush*>::CreateSP(this, &FOutlinerHiddenInGameColumn::GetIconImageBrush, WeakActor))
						.ColorAndOpacity(TAttribute<FSlateColor>::CreateSP(this, &FOutlinerHiddenInGameColumn::GetColorAndOpacityButtonImage, WeakActor))
				]
		];
}

const FSlateBrush* FOutlinerHiddenInGameColumn::GetIconImageBrush(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return OutlinerHiddenInGameColumn::HiddenInGameIcon.GetIcon();
	}

	const ECheckBoxState State = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<USceneComponent>(WeakActor.Get()),
		[](const USceneComponent* Component)
		{
			return !Component->bHiddenInGame;
		});

	const bool bVisible = (State != ECheckBoxState::Checked);

	if (bVisible)
	{
		return OutlinerHiddenInGameColumn::HiddenInGameIcon.GetIcon();
	}

	return OutlinerHiddenInGameColumn::HiddenInGameIcon.GetIcon();
}

FSlateColor FOutlinerHiddenInGameColumn::GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return OutlinerHiddenInGameColumn::NoValidColor;
	}

	const ECheckBoxState State = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<USceneComponent>(WeakActor.Get()),
		[](const USceneComponent* Component)
		{
			return Component->bHiddenInGame;
		});

	switch (State)
	{
		case ECheckBoxState::Checked:
		{
			return OutlinerHiddenInGameColumn::CheckedColor;
		}

		case ECheckBoxState::Undetermined:
		{
			return OutlinerHiddenInGameColumn::UndeterminedColor;
		}

		default:
		{
			return OutlinerHiddenInGameColumn::DefaultColor;
		}
	}
}

FReply FOutlinerHiddenInGameColumn::GetHiddenInGameColumnButtonClicked(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return FReply::Handled();
	}

	AActor* CurrentActor = WeakActor.Get();

	const ECheckBoxState CurrentState = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<USceneComponent>(CurrentActor),
		[](const USceneComponent* Component)
		{
			return Component->bHiddenInGame;
		});

	const bool bNewHiddenState = CurrentState != ECheckBoxState::Checked;
	const FScopedTransaction Transaction(LOCTEXT("ToggleHiddenInGameTransaction", "Toggle Hidden In Game"));
	OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<USceneComponent>(CurrentActor),
		[bNewHiddenState](USceneComponent* Component)
		{
			Component->Modify();
			Component->SetHiddenInGame(bNewHiddenState);
		});

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
