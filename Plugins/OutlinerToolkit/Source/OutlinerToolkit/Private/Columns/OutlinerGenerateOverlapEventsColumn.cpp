// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerGenerateOverlapEventsColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"


namespace OutlinerGenerateOverlapEventsColumn
{
	static const FName GenerateOverlapIconName = FName(TEXT("CollisionAnalyzer.TabIcon"));

	static const FLinearColor DefaultColor = FLinearColor(0.45f, 0.45f, 0.45f, 0.5f);
	static const FLinearColor CheckedColor = FLinearColor(0.3f, 0.9f, 0.4f, 1.0f);
	static const FLinearColor UndeterminedColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
	static const FLinearColor NoValidColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.35f);

}

FOutlinerGenerateOverlapEventsColumn::FOutlinerGenerateOverlapEventsColumn(ISceneOutliner& SceneOutliner)
	: FOutlinerToolkitColumnBase(SceneOutliner)
{
}

FName FOutlinerGenerateOverlapEventsColumn::GetColumnID()
{
	return GetID();
}

SHeaderRow::FColumn::FArguments FOutlinerGenerateOverlapEventsColumn::ConstructHeaderRowColumn()
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
						.ColorAndOpacity(FSlateColor::UseForeground())
						.ToolTipText(FText::FromString("Generate Overlap Events"))
						.Image(FAppStyle::Get().GetBrush(OutlinerGenerateOverlapEventsColumn::GenerateOverlapIconName))
				]
		];
}

bool FOutlinerGenerateOverlapEventsColumn::SupportsSorting() const
{
	return false;
}

const TSharedRef<SWidget> FOutlinerGenerateOverlapEventsColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	AActor* Actor = OutlinerColumnUtils::ResolveActor(TreeItem);
	if (!Actor)
	{
		return SNullWidget::NullWidget;
	}

	const TArray<UPrimitiveComponent*> PrimitiveComponents = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor);
	if (PrimitiveComponents.IsEmpty())
	{
		return SNullWidget::NullWidget;
	}

	TWeakObjectPtr<AActor> WeakActor = Actor;

	return	SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(0.0f)
		.ToolTipText(FText::FromString("Toggle whether all primitive components on this actor generate overlap events."))
		.OnClicked(FOnClicked::CreateSP(this, &FOutlinerGenerateOverlapEventsColumn::GetGenerateOverlapButtonClicked, WeakActor))
		[
			SNew(SImage)
				.Image(FAppStyle::Get().GetBrush(OutlinerGenerateOverlapEventsColumn::GenerateOverlapIconName))
				.ColorAndOpacity(TAttribute<FSlateColor>::CreateSP(this, &FOutlinerGenerateOverlapEventsColumn::GetColorAndOpacityButtonImage, WeakActor))
		];
}

FReply FOutlinerGenerateOverlapEventsColumn::GetGenerateOverlapButtonClicked(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return FReply::Handled();
	}

	AActor* CurrentActor = WeakActor.Get();
	const ECheckBoxState CurrentState = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(CurrentActor),
		[](const UPrimitiveComponent* Component)
		{
			return Component->GetGenerateOverlapEvents();
		});

	const bool bEnableOverlap = CurrentState != ECheckBoxState::Checked;

	const FScopedTransaction Transaction(NSLOCTEXT("OutlinerGenerateOverlapEventsColumn", "ToggleGenerateOverlapEventsTransaction", "Toggle Generate Overlap Events"));
	OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(CurrentActor),
		[bEnableOverlap](UPrimitiveComponent* Component)
		{
			Component->Modify();
			Component->SetGenerateOverlapEvents(bEnableOverlap);
		});

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}

	return FReply::Handled();
}

FSlateColor FOutlinerGenerateOverlapEventsColumn::GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return OutlinerGenerateOverlapEventsColumn::NoValidColor;
	}

	const ECheckBoxState State = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get()),
		[](const UPrimitiveComponent* Component)
		{
			return Component->GetGenerateOverlapEvents();
		});

	switch (State)
	{
		case ECheckBoxState::Checked:
		{
			return OutlinerGenerateOverlapEventsColumn::CheckedColor;
		}

		case ECheckBoxState::Undetermined:
		{
			return OutlinerGenerateOverlapEventsColumn::UndeterminedColor;
		}

		default:
		{
			return OutlinerGenerateOverlapEventsColumn::DefaultColor;
		}
	}
}
