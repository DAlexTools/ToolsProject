// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerNaniteColumn.h"

#include "Columns/OutlinerColumnUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "StaticMeshEditorSubsystem.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "OutlinerNaniteColumn"

namespace OutlinerNaniteColumn
{
	static const FName NaniteBrushName = FName(TEXT("Icons.NaniteBrowseContent"));

	static const FLinearColor DefaultColor = FLinearColor(0.45f, 0.45f, 0.45f, 0.5f);
	static const FLinearColor CheckedColor = FLinearColor(0.1f, 0.85f, 0.65f, 1.0f);
	static const FLinearColor UndeterminedColor = FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);
	static const FLinearColor NoValidColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.35f);

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

	static void SetNaniteEnabled(UStaticMesh* StaticMesh, bool bEnabled)
	{
		if (!StaticMesh || StaticMesh->NaniteSettings.bEnabled == bEnabled)
		{
			return;
		}

		FMeshNaniteSettings NaniteSettings = StaticMesh->NaniteSettings;
		NaniteSettings.bEnabled = bEnabled;

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

	static ECheckBoxState GetNaniteCheckState(const TArray<UStaticMesh*>& StaticMeshes)
	{
		return OutlinerColumnUtils::GetComponentCheckState(
			StaticMeshes,
			[](const UStaticMesh* StaticMesh)
			{
				return StaticMesh->NaniteSettings.bEnabled;
			});
	}
} // namespace OutlinerNaniteColumn

SHeaderRow::FColumn::FArguments FOutlinerNaniteColumn::ConstructHeaderRowColumn()
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
				.Image(FAppStyle::Get().GetBrush(OutlinerNaniteColumn::NaniteBrushName))
				.ColorAndOpacity(FSlateColor::UseForeground())
				.ToolTipText(LOCTEXT("NaniteHeaderToolTip", "Nanite"))
			]
		];
}

const TSharedRef<SWidget> FOutlinerNaniteColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	AActor* Actor = OutlinerColumnUtils::ResolveActor(TreeItem);
	if (!Actor)
	{
		return SNullWidget::NullWidget;
	}

	const TArray<UStaticMesh*> StaticMeshes = OutlinerNaniteColumn::GetStaticMeshes(Actor);
	if (StaticMeshes.IsEmpty())
	{
		return SNullWidget::NullWidget;
	}

	const TWeakObjectPtr<AActor> WeakActor = Actor;

	return SNew(SButton)
		.ContentPadding(0.0f)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(LOCTEXT("ToggleNaniteToolTip", "Toggle Nanite on static mesh assets used by this actor. This affects every actor that uses the same mesh asset."))
		.OnClicked(FOnClicked::CreateSP(this, &FOutlinerNaniteColumn::GetNaniteColumnButtonClicked, WeakActor))
		[
			SNew(SBox)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush(OutlinerNaniteColumn::NaniteBrushName))
				.ColorAndOpacity(TAttribute<FSlateColor>::CreateSP(this, &FOutlinerNaniteColumn::GetColorAndOpacityButtonImage, WeakActor))
			]
		];
}

FReply FOutlinerNaniteColumn::GetNaniteColumnButtonClicked(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return FReply::Handled();
	}

	const TArray<UStaticMesh*> StaticMeshes = OutlinerNaniteColumn::GetStaticMeshes(WeakActor.Get());
	if (StaticMeshes.IsEmpty())
	{
		return FReply::Handled();
	}

	const bool bEnableNanite = OutlinerNaniteColumn::GetNaniteCheckState(StaticMeshes) != ECheckBoxState::Checked;

	const FScopedTransaction Transaction(LOCTEXT("ToggleNaniteTransaction", "Toggle Nanite"));
	for (UStaticMesh* StaticMesh : StaticMeshes)
	{
		OutlinerNaniteColumn::SetNaniteEnabled(StaticMesh, bEnableNanite);
	}

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}

	return FReply::Handled();
}

FSlateColor FOutlinerNaniteColumn::GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return OutlinerNaniteColumn::NoValidColor;
	}

	const TArray<UStaticMesh*> StaticMeshes = OutlinerNaniteColumn::GetStaticMeshes(WeakActor.Get());
	if (StaticMeshes.IsEmpty())
	{
		return OutlinerNaniteColumn::NoValidColor;
	}

	switch (OutlinerNaniteColumn::GetNaniteCheckState(StaticMeshes))
	{
		case ECheckBoxState::Checked:
		{
			return OutlinerNaniteColumn::CheckedColor;
		}

		case ECheckBoxState::Undetermined:
		{
			return OutlinerNaniteColumn::UndeterminedColor;
		}

		default:
		{
			return OutlinerNaniteColumn::DefaultColor;
		}
	}
}

#undef LOCTEXT_NAMESPACE
