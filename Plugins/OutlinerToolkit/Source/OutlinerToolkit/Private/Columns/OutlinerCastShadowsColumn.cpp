// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerCastShadowsColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OutlinerCastShadowsColumn"

namespace OutlinerCastShadows
{
	static const FName CastShadowBrushName = FName(TEXT("ClassIcon.LightComponent"));

	static const FLinearColor DefaultColor = FLinearColor(0.45f, 0.45f, 0.45f, 0.5f);
	static const FLinearColor CheckedColor = FLinearColor(1.0f, 0.85f, 0.3f, 1.0f);
	static const FLinearColor UndeterminedColor = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);
	static const FLinearColor NoValidColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.35f);
}

FOutlinerCastShadowsColumn::FOutlinerCastShadowsColumn(ISceneOutliner& SceneOutliner)
	: FOutlinerToolkitColumnBase(SceneOutliner)
{
}

FName FOutlinerCastShadowsColumn::GetColumnID()
{
	return GetID();
}

SHeaderRow::FColumn::FArguments FOutlinerCastShadowsColumn::ConstructHeaderRowColumn()
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
						.Image(FAppStyle::Get().GetBrush(OutlinerCastShadows::CastShadowBrushName))
						.ColorAndOpacity(FSlateColor::UseForeground())
						.ToolTipText(FText::FromString("Cast Shadows"))
				]
		];
}

bool FOutlinerCastShadowsColumn::SupportsSorting() const
{
	return false;
}

const TSharedRef<SWidget> FOutlinerCastShadowsColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
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

	const TWeakObjectPtr<AActor> WeakActor = Actor;

	return	SNew(SButton)
		.ContentPadding(0.0f)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(LOCTEXT("Toogle ToolTip Text", "Toggle whether all primitive components on this actor cast shadows."))
		.OnClicked(FOnClicked::CreateSP(this, &FOutlinerCastShadowsColumn::GetCastShadowColumnButtonClicked, WeakActor))
		[
			SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
						.Image(FAppStyle::Get().GetBrush(OutlinerCastShadows::CastShadowBrushName))
						.ColorAndOpacity(TAttribute<FSlateColor>::CreateSP(this, &FOutlinerCastShadowsColumn::GetColorAndOpacityButtonImage, WeakActor))
				]
		];
}

FReply FOutlinerCastShadowsColumn::GetCastShadowColumnButtonClicked(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return FReply::Handled();
	}

	AActor* CurrentActor = WeakActor.Get();

	const ECheckBoxState CurrentState = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(CurrentActor),
		[](const UPrimitiveComponent* Component)
		{
			return Component->CastShadow;
		});

	const bool bCastShadows = CurrentState != ECheckBoxState::Checked;

	const FScopedTransaction Transaction(LOCTEXT("ToggleCastShadowsTransaction", "Toggle Cast Shadows"));
	OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(CurrentActor),
		[bCastShadows](UPrimitiveComponent* Component)
		{
			Component->Modify();
			Component->SetCastShadow(bCastShadows);
		});

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}

	return FReply::Handled();
}

FSlateColor FOutlinerCastShadowsColumn::GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return OutlinerCastShadows::NoValidColor;
	}

	const ECheckBoxState State = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get()),
		[](const UPrimitiveComponent* Component)
		{
			return Component->CastShadow;
		});

	switch (State)
	{
		case ECheckBoxState::Checked:
		{
			return OutlinerCastShadows::CheckedColor;
		}

		case ECheckBoxState::Undetermined:
		{
			return OutlinerCastShadows::UndeterminedColor;
		}

		default:
		{
			return OutlinerCastShadows::DefaultColor;
		}
	}
}

#undef LOCTEXT_NAMESPACE
