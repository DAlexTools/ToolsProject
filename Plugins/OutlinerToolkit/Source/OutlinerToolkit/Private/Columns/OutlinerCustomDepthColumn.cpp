// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerCustomDepthColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateIconFinder.h"

namespace OutlinerCustomDepthColumn
{
	static const FName CustomDepthIconName = FName(TEXT("ClassIcon.PostProcessVolume"));
	static const FLinearColor DefaultColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.5f);
	static const FLinearColor CheckedColor = FLinearColor(1.0f, 0.75f, 0.2f, 1.0f);
	static const FLinearColor UndeterminedColor = FLinearColor(0.3f, 0.8f, 1.0f, 1.0f);
	static const FLinearColor NoValidColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.35f);
}

FName FOutlinerCustomDepthColumn::GetColumnID()
{
	return GetID();
}

FOutlinerCustomDepthColumn::FOutlinerCustomDepthColumn(ISceneOutliner& SceneOutliner)
	: FOutlinerToolkitColumnBase(SceneOutliner)
{
}

SHeaderRow::FColumn::FArguments FOutlinerCustomDepthColumn::ConstructHeaderRowColumn()
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
					.ToolTipText(FText::FromString("Render CustomDepth"))
					.Image(FAppStyle::Get().GetBrush(OutlinerCustomDepthColumn::CustomDepthIconName))
				]
			];
}

bool FOutlinerCustomDepthColumn::SupportsSorting() const
{
	return false;
}

const TSharedRef<SWidget> FOutlinerCustomDepthColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
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
			.ToolTipText(FText::FromString("Toggle whether all primitive components on this actor render to CustomDepth."))
			.OnClicked(FOnClicked::CreateSP(this, &FOutlinerCustomDepthColumn::GetOnClicked, WeakActor))
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush(OutlinerCustomDepthColumn::CustomDepthIconName))
					.ColorAndOpacity(TAttribute<FSlateColor>::CreateSP(this, &FOutlinerCustomDepthColumn::GetColorAndOpacity, WeakActor))
				]
			];
}

FReply FOutlinerCustomDepthColumn::GetOnClicked(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return FReply::Handled();
	}

	AActor* CurrentActor = WeakActor.Get();

	const ECheckBoxState CurrentState = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(CurrentActor),
			[](const UPrimitiveComponent* Component)
			{
				return Component->bRenderCustomDepth;
			});

	const bool bEnableCustomDepth = CurrentState != ECheckBoxState::Checked;
	const FScopedTransaction Transaction(NSLOCTEXT("OutlinerCustomDepthColumn", "ToggleCustomDepthTransaction", "Toggle Render CustomDepth"));
	OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(CurrentActor),
		[bEnableCustomDepth](UPrimitiveComponent* Component)
		{
			Component->Modify();
			Component->SetRenderCustomDepth(bEnableCustomDepth);
		});

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}

	return FReply::Handled();

}

FSlateColor FOutlinerCustomDepthColumn::GetColorAndOpacity(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return OutlinerCustomDepthColumn::NoValidColor;
	}

	const ECheckBoxState State = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get()),
			[](const UPrimitiveComponent* Component)
			{
				return Component->bRenderCustomDepth;
			});

	switch (State)
	{
		case ECheckBoxState::Checked:
		{
			return OutlinerCustomDepthColumn::CheckedColor;
		}

		case ECheckBoxState::Undetermined:
		{
			return OutlinerCustomDepthColumn::UndeterminedColor;
		}

		default:
		{
			return OutlinerCustomDepthColumn::DefaultColor;
		}
	}
}
