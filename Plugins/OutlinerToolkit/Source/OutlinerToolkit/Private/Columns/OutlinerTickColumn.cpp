// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerTickColumn.h"
#include "ActorTreeItem.h"
#include "GameFramework/Actor.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OutlinerTickColumn"

namespace OutlinerTickColumn
{
	static const FName TickIconBrush(TEXT("GraphEditor.Event_16x"));
	static const FLinearColor TickEnableColor = FLinearColor(0.3f, 1.0f, 0.3f, 1.0f);
	static const FLinearColor TickDisableColor = FLinearColor(0.4f, 0.4f, 0.4f, 0.45f);
	static const FLinearColor NoValidWeakColor = FLinearColor(0.15f, 0.15f, 0.15f, 0.35f);
}

SHeaderRow::FColumn::FArguments FOutlinerTickColumn::ConstructHeaderRowColumn()
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
					.Image(FAppStyle::Get().GetBrush(OutlinerTickColumn::TickIconBrush))
					.ToolTipText(LOCTEXT("ActorTickHeaderTooltip", "Actor Tick"))
				]
			];
}

const TSharedRef<SWidget> FOutlinerTickColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	const FActorTreeItem* const ActorItem = TreeItem->CastTo<FActorTreeItem>();
	AActor* Actor = ActorItem ? ActorItem->Actor.Get() : nullptr;
	if(!Actor)
	{
		return SNullWidget::NullWidget;
	}

	const TWeakObjectPtr<AActor> WeakActor(Actor);
	const bool bCanEverTick = Actor->PrimaryActorTick.bCanEverTick;

	return	SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(0.0f)
			.ToolTipText(TAttribute<FText>::CreateSP(this, &FOutlinerTickColumn::GetTickTooltipText, bCanEverTick))
			.IsEnabled(bCanEverTick)
			.OnClicked(FOnClicked::CreateSP(this, &FOutlinerTickColumn::OnTickColumnClicked, WeakActor, bCanEverTick))
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush(OutlinerTickColumn::TickIconBrush))
					.ColorAndOpacity(TAttribute<FSlateColor>::CreateSP(this, &FOutlinerTickColumn::GetColorAndOpacity, WeakActor, bCanEverTick))
				]
			];
}

FText FOutlinerTickColumn::GetTickTooltipText(bool bCanEverTick) const
{
	return bCanEverTick
		? LOCTEXT("TickTooltip", "Toggle whether this actor receives Tick.")
		: LOCTEXT("TickUnsupportedTooltip", "This actor class does not support Tick.");
}

FSlateColor FOutlinerTickColumn::GetColorAndOpacity(TWeakObjectPtr<AActor> WeakActor, bool bCanEverTick) const
{
	if (!WeakActor.IsValid() || !bCanEverTick)
	{
		return OutlinerTickColumn::NoValidWeakColor;
	}

	return WeakActor->IsActorTickEnabled()
		? OutlinerTickColumn::TickEnableColor
		: OutlinerTickColumn::TickDisableColor;
}

FReply FOutlinerTickColumn::OnTickColumnClicked(TWeakObjectPtr<AActor> WeakActor, bool bCanEverTick) const
{
	if (!bCanEverTick || !WeakActor.IsValid())
	{
		return FReply::Handled();
	}

	AActor* CurrentActor = WeakActor.Get();
	const FScopedTransaction Transaction(LOCTEXT("ToggleActorTickTransaction", "Toggle Actor Tick"));
	CurrentActor->Modify();

	const bool bNewTickState = !CurrentActor->IsActorTickEnabled();
	CurrentActor->SetActorTickEnabled(bNewTickState);
	
	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
