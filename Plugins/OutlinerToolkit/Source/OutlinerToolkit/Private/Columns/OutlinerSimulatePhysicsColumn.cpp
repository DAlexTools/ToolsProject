// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerSimulatePhysicsColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(OutlinerSimulatePhysicsColumnLog, All, All);

#define LOCTEXT_NAMESPACE "OutlinerSimulatePhysicsColumn" 

namespace OutlinerSimulatePhysicsColumn
{
	static const FName SimulatePhysicsImageBrushName = FName("PhysicsAssetEditor.Tree.Bone");

	static const FLinearColor DefaultColor = FLinearColor(0.45f, 0.45f, 0.45f, 0.5f);
	static const FLinearColor CheckedColor = FLinearColor(0.2f, 0.85f, 1.0f, 1.0f);
	static const FLinearColor UndeterminedColor = FLinearColor(1.0f, 0.75f, 0.2f, 1.0f);
	static const FLinearColor NoValidColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.35f);
}

FOutlinerSimulatePhysicsColumn::FOutlinerSimulatePhysicsColumn(ISceneOutliner& SceneOutliner)
	: FOutlinerToolkitColumnBase(SceneOutliner)
{
}

FName FOutlinerSimulatePhysicsColumn::GetColumnID()
{
	return GetID();
}

SHeaderRow::FColumn::FArguments FOutlinerSimulatePhysicsColumn::ConstructHeaderRowColumn()
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
						.Image(FAppStyle::Get().GetBrush(OutlinerSimulatePhysicsColumn::SimulatePhysicsImageBrushName))
						.ColorAndOpacity(FSlateColor::UseForeground())
						.ToolTipText(FText::FromString("Physics Simulation"))
				]
		];
}

const TSharedRef<SWidget> FOutlinerSimulatePhysicsColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
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

	const bool bIsStaticMeshActor = Actor->IsA<AStaticMeshActor>();

	bool bIsSkySphere = false;
	if (const UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(PrimitiveComponents[0]))
	{
		const UStaticMesh* Mesh = StaticMeshComp->GetStaticMesh();
		if (Mesh && Mesh->GetName().Contains(TEXT("SM_SkySphere")))
		{
			bIsSkySphere = true;
		}
	}

	const bool bIsEnabled = bIsStaticMeshActor && !bIsSkySphere;
	TWeakObjectPtr<AActor> WeakActor = Actor;

	return	SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(0.0f)
		.IsEnabled(bIsEnabled)
		.OnClicked((FOnClicked::CreateSP(this, &FOutlinerSimulatePhysicsColumn::GetOnButtonClicked, WeakActor, bIsEnabled)))
		.ToolTipText(TAttribute<FText>::CreateSP(this, &FOutlinerSimulatePhysicsColumn::GetButtonToolTipText, bIsStaticMeshActor, bIsSkySphere))
		[
			SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
						.Image(FAppStyle::Get().GetBrush(OutlinerSimulatePhysicsColumn::SimulatePhysicsImageBrushName))
						.ColorAndOpacity(TAttribute<FSlateColor>::CreateSP(this, &FOutlinerSimulatePhysicsColumn::GetButtonColorAndOpacity, WeakActor, bIsEnabled))
				]
		];
}

FText FOutlinerSimulatePhysicsColumn::GetButtonToolTipText(bool bIsStaticMeshActor, bool bIsSkySphereActor) const
{
	if (!bIsStaticMeshActor)
	{
		return LOCTEXT("OnlyStaticMeshActorSupportsPhysics", "Only StaticMeshActor supports physics simulation.");
	}

	if (bIsSkySphereActor)
	{
		return LOCTEXT("SkySphereCannotSimulatePhysics", "SkySphere actors cannot simulate physics.");
	}

	return LOCTEXT("TogglePhysicsSimulationOnComponents", "Toggle physics simulation on all primitive components.");
}

FReply FOutlinerSimulatePhysicsColumn::GetOnButtonClicked(TWeakObjectPtr<AActor> WeakActor, bool bIsEnabled) const
{
	if (!bIsEnabled)
	{
		return FReply::Handled();
	}

	if (!WeakActor.IsValid())
	{
		UE_LOG(OutlinerSimulatePhysicsColumnLog, Warning, TEXT("Actor is invalid in OnClicked"));
		return FReply::Handled();
	}

	AActor* CurrentActor = WeakActor.Get();

	const ECheckBoxState CurrentState = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(CurrentActor),
		[](const UPrimitiveComponent* Component)
		{
			return Component->IsSimulatingPhysics();
		});

	const bool bEnablePhysics = CurrentState != ECheckBoxState::Checked;

	const FScopedTransaction Transaction(LOCTEXT("ToggleSimulatePhysicsTransaction", "Toggle Simulate Physics"));
	if (bEnablePhysics)
	{
		OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<USceneComponent>(CurrentActor),
			[](USceneComponent* Component)
			{
				Component->SetMobility(EComponentMobility::Movable);
			});
	}

	OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(CurrentActor),
		[bEnablePhysics](UPrimitiveComponent* Primitive)
		{
			Primitive->SetSimulatePhysics(bEnablePhysics);
		});

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}

	return FReply::Handled();
}

FSlateColor FOutlinerSimulatePhysicsColumn::GetButtonColorAndOpacity(TWeakObjectPtr<AActor> WeakActor, bool bIsEnabled) const
{
	if (!bIsEnabled || !WeakActor.IsValid())
	{
		return OutlinerSimulatePhysicsColumn::NoValidColor;
	}

	const ECheckBoxState State = OutlinerColumnUtils::GetComponentCheckState(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get()),
		[](const UPrimitiveComponent* Component)
		{
			return Component->IsSimulatingPhysics();
		});

	switch (State)
	{
		case ECheckBoxState::Checked:
		{
			return OutlinerSimulatePhysicsColumn::CheckedColor;
		}

		case ECheckBoxState::Undetermined:
		{
			return OutlinerSimulatePhysicsColumn::UndeterminedColor;
		}

		default:
		{
			return OutlinerSimulatePhysicsColumn::DefaultColor;
		}
	}
}

#undef LOCTEXT_NAMESPACE
