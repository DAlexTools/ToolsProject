// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerActorLockColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "OutlinerActorLockColumn"

namespace OutlinerActorLockColumn
{
	static const FName LockedBrushName = FName(TEXT("PropertyWindow.Locked"));
	static const FName UnlockedBrushName = FName(TEXT("PropertyWindow.Unlocked"));

	static const FLinearColor DefaultColor = FLinearColor(0.45f, 0.45f, 0.45f, 0.5f);
	static const FLinearColor CheckedColor = FLinearColor(1.0f, 0.75f, 0.2f, 1.0f);
	static const FLinearColor NoValidColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.35f);
}

FName FOutlinerActorLockColumn::GetColumnID()
{
	return GetID();
}

SHeaderRow::FColumn::FArguments FOutlinerActorLockColumn::ConstructHeaderRowColumn()
{
	return SHeaderRow::Column(GetColumnID())
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
				.Image(FAppStyle::Get().GetBrush(OutlinerActorLockColumn::LockedBrushName))
				.ColorAndOpacity(FSlateColor::UseForeground())
				.ToolTipText(LOCTEXT("ActorLockHeaderToolTip", "Lock Actor Movement"))
			]
		];
}

const TSharedRef<SWidget> FOutlinerActorLockColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	AActor* Actor = OutlinerColumnUtils::ResolveActor(TreeItem);
	if (!Actor)
	{
		return SNullWidget::NullWidget;
	}

	const TWeakObjectPtr<AActor> WeakActor = Actor;

	return SNew(SButton)
		.ContentPadding(0.0f)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(LOCTEXT("ToggleActorLockToolTip", "Toggle Lock Actor Movement. Locked actors cannot be moved in the editor viewport."))
		.OnClicked(FOnClicked::CreateSP(this, &FOutlinerActorLockColumn::GetLockColumnButtonClicked, WeakActor))
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(TAttribute<const FSlateBrush*>::CreateSP(this, &FOutlinerActorLockColumn::GetIconImageBrush, WeakActor))
				.ColorAndOpacity(TAttribute<FSlateColor>::CreateSP(this, &FOutlinerActorLockColumn::GetColorAndOpacityButtonImage, WeakActor))
			]
		];
}

const FSlateBrush* FOutlinerActorLockColumn::GetIconImageBrush(TWeakObjectPtr<AActor> WeakActor) const
{
	if (WeakActor.IsValid() && WeakActor->IsLockLocation())
	{
		return FAppStyle::Get().GetBrush(OutlinerActorLockColumn::LockedBrushName);
	}

	return FAppStyle::Get().GetBrush(OutlinerActorLockColumn::UnlockedBrushName);
}

FReply FOutlinerActorLockColumn::GetLockColumnButtonClicked(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return FReply::Handled();
	}

	AActor* Actor = WeakActor.Get();
	const bool bLockLocation = !Actor->IsLockLocation();

	const FScopedTransaction Transaction(bLockLocation ? LOCTEXT("LockActorMovementTransaction", "Lock Actor Movement") : LOCTEXT("UnlockActorMovementTransaction", "Unlock Actor Movement"));
	Actor->Modify();
	Actor->SetLockLocation(bLockLocation);

	OutlinerColumnUtils::RefreshEditorMovementState();

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}

	return FReply::Handled();
}

FSlateColor FOutlinerActorLockColumn::GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return OutlinerActorLockColumn::NoValidColor;
	}

	return WeakActor->IsLockLocation() ? OutlinerActorLockColumn::CheckedColor : OutlinerActorLockColumn::DefaultColor;
}

#undef LOCTEXT_NAMESPACE
