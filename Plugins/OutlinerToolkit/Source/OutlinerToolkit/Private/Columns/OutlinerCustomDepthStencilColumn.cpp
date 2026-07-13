// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerCustomDepthStencilColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/Optional.h"

FOutlinerCustomDepthStencilColumn::FOutlinerCustomDepthStencilColumn(ISceneOutliner& SceneOutliner)
	: FOutlinerToolkitColumnBase(SceneOutliner)
{

}

FName FOutlinerCustomDepthStencilColumn::GetColumnID()
{
	return GetID();
}

SHeaderRow::FColumn::FArguments FOutlinerCustomDepthStencilColumn::ConstructHeaderRowColumn()
{
	return	SHeaderRow::Column(GetColumnID())
		.FixedWidth(70.0f)
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.DefaultLabel(FText::FromString("Stencil"))
		[
			SNew(STextBlock)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Text(FText::FromString("St"))
		];
}

bool FOutlinerCustomDepthStencilColumn::SupportsSorting() const
{
	return false;
}

const TSharedRef<SWidget> FOutlinerCustomDepthStencilColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
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

	const TWeakObjectPtr<AActor> WeakActor(Actor);

	return	SNew(SNumericEntryBox<int32>)
		.AllowSpin(true)
		.MinValue(0)
		.MaxValue(255)
		.MinSliderValue(0)
		.MaxSliderValue(255)
		.MinDesiredValueWidth(50.0f)
		.IsEnabled(TAttribute<bool>::CreateSP(this, &FOutlinerCustomDepthStencilColumn::IsCustomDepthEnable, WeakActor))
		.Value(TAttribute<TOptional<int32>>::CreateSP(this, &FOutlinerCustomDepthStencilColumn::GetCustomDepthStencilValue, WeakActor))
		.OnValueCommitted(SNumericEntryBox<int32>::FOnValueCommitted::CreateSP(this, &FOutlinerCustomDepthStencilColumn::OnCustomDepthStencilCommited, WeakActor))
		.ToolTipText(FText::FromString("Set CustomDepth stencil value for all primitive components on this actor. Blank means mixed values."));
}


TOptional<int32> FOutlinerCustomDepthStencilColumn::GetCustomDepthStencilValue(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return TOptional<int32>();
	}

	const TArray<UPrimitiveComponent*> Components = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get());
	if (Components.IsEmpty())
	{
		return TOptional<int32>();
	}

	const int32 FirstValue = Components[0]->CustomDepthStencilValue;
	for (int32 Index = 1; Index < Components.Num(); ++Index)
	{
		if (Components[Index]->CustomDepthStencilValue != FirstValue)
		{
			return TOptional<int32>();
		}
	}

	return FirstValue;
}

bool FOutlinerCustomDepthStencilColumn::IsCustomDepthEnable(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return false;
	}

	const ECheckBoxState State = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get()),
		[](const UPrimitiveComponent* Component)
		{
			return Component->bRenderCustomDepth;
		});

	return State == ECheckBoxState::Checked;
}

void FOutlinerCustomDepthStencilColumn::OnCustomDepthStencilCommited(int32 NewValue, ETextCommit::Type CommitType, TWeakObjectPtr<AActor> WeakActor)
{
	if (!WeakActor.IsValid())
	{
		return;
	}

	const int32 ClampedValue = FMath::Clamp(NewValue, 0, 255);

	const FScopedTransaction Transaction(NSLOCTEXT("OutlinerCustomDepthStencilColumn", "SetCustomDepthStencilTransaction", "Set CustomDepth Stencil"));
	OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get()),
		[ClampedValue](UPrimitiveComponent* Component)
		{
			Component->SetCustomDepthStencilValue(ClampedValue);
		});

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}
}
