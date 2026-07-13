// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerNaniteKeepTrianglePercentColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "StaticMeshEditorSubsystem.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OutlinerNaniteKeepTrianglePercentColumn"

namespace OutlinerNaniteKeepTrianglePercentColumn
{
static TArray<UStaticMesh*> GetStaticMeshes(AActor* Actor)
{
	TArray<UStaticMesh*> StaticMeshes;
	for (const UStaticMeshComponent* Component : OutlinerColumnUtils::GetActorComponents<UStaticMeshComponent>(Actor))
	{
		if (!Component)
		{
			continue;
		}

		if (UStaticMesh* StaticMesh = Component->GetStaticMesh())
		{
			StaticMeshes.AddUnique(StaticMesh);
		}
	}

	return StaticMeshes;
}

static void SetKeepTrianglePercent(UStaticMesh* StaticMesh, float KeepTrianglePercent)
{
	if (!StaticMesh)
	{
		return;
	}

	const float ClampedPercent = FMath::Clamp(KeepTrianglePercent, 0.0f, 100.0f);
	const float NewKeepPercentTriangles = ClampedPercent * 0.01f;
	if (FMath::IsNearlyEqual(StaticMesh->NaniteSettings.KeepPercentTriangles, NewKeepPercentTriangles))
	{
		return;
	}

	FMeshNaniteSettings NaniteSettings = StaticMesh->NaniteSettings;
	NaniteSettings.KeepPercentTriangles = NewKeepPercentTriangles;

	if (GEditor)
	{
		if (UStaticMeshEditorSubsystem* StaticMeshEditorSubsystem = GEditor->GetEditorSubsystem<UStaticMeshEditorSubsystem>())
		{
			StaticMeshEditorSubsystem->SetNaniteSettings(StaticMesh, NaniteSettings, true);
			return;
		}
	}

	StaticMesh->Modify();
	StaticMesh->NaniteSettings = NaniteSettings;
	StaticMesh->PostEditChange();
	StaticMesh->MarkPackageDirty();
}
} // namespace OutlinerNaniteKeepTrianglePercentColumn

SHeaderRow::FColumn::FArguments FOutlinerNaniteKeepTrianglePercentColumn::ConstructHeaderRowColumn()
{
	return SHeaderRow::Column(GetColumnID())
		.FixedWidth(76.0f)
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.DefaultLabel(LOCTEXT("KeepTrianglePercentColumnLabel", "Keep Triangle %"))
			[SNew(STextBlock)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Text(LOCTEXT("KeepTrianglePercentColumnShortLabel", "Keep %"))
					.ToolTipText(LOCTEXT("KeepTrianglePercentHeaderToolTip", "Nanite Keep Triangle Percent"))];
}

const TSharedRef<SWidget> FOutlinerNaniteKeepTrianglePercentColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	AActor* Actor = OutlinerColumnUtils::ResolveActor(TreeItem);
	if (!Actor)
	{
		return SNullWidget::NullWidget;
	}

	const TArray<UStaticMesh*> StaticMeshes = OutlinerNaniteKeepTrianglePercentColumn::GetStaticMeshes(Actor);
	if (StaticMeshes.IsEmpty())
	{
		return SNullWidget::NullWidget;
	}

	const TWeakObjectPtr<AActor> WeakActor = Actor;

	return SNew(SNumericEntryBox<float>)
		.AllowSpin(true)
		.MinValue(0.0f)
		.MaxValue(100.0f)
		.MinSliderValue(0.0f)
		.MaxSliderValue(100.0f)
		.MinDesiredValueWidth(56.0f)
		.Value(TAttribute<TOptional<float>>::CreateSP(this, &FOutlinerNaniteKeepTrianglePercentColumn::GetKeepTrianglePercentValue, WeakActor))
		.OnValueCommitted(SNumericEntryBox<float>::FOnValueCommitted::CreateSP(this, &FOutlinerNaniteKeepTrianglePercentColumn::OnKeepTrianglePercentCommitted, WeakActor))
		.ToolTipText(LOCTEXT("KeepTrianglePercentToolTip", "Set Nanite Keep Triangle Percent on static mesh assets used by this actor. Blank means mixed values."));
}

TOptional<float> FOutlinerNaniteKeepTrianglePercentColumn::GetKeepTrianglePercentValue(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return TOptional<float>();
	}

	const TArray<UStaticMesh*> StaticMeshes = OutlinerNaniteKeepTrianglePercentColumn::GetStaticMeshes(WeakActor.Get());
	if (StaticMeshes.IsEmpty())
	{
		return TOptional<float>();
	}

	const float FirstValue = StaticMeshes[0]->NaniteSettings.KeepPercentTriangles * 100.0f;
	for (int32 Index = 1; Index < StaticMeshes.Num(); ++Index)
	{
		const float CurrentValue = StaticMeshes[Index]->NaniteSettings.KeepPercentTriangles * 100.0f;
		if (!FMath::IsNearlyEqual(CurrentValue, FirstValue))
		{
			return TOptional<float>();
		}
	}

	return FirstValue;
}

void FOutlinerNaniteKeepTrianglePercentColumn::OnKeepTrianglePercentCommitted(float NewValue, ETextCommit::Type CommitType, TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return;
	}

	const TArray<UStaticMesh*> StaticMeshes = OutlinerNaniteKeepTrianglePercentColumn::GetStaticMeshes(WeakActor.Get());
	if (StaticMeshes.IsEmpty())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetKeepTrianglePercentTransaction", "Set Nanite Keep Triangle Percent"));
	for (UStaticMesh* StaticMesh : StaticMeshes)
	{
		OutlinerNaniteKeepTrianglePercentColumn::SetKeepTrianglePercent(StaticMesh, NewValue);
	}

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
