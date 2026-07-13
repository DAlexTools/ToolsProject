// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerCollisionPresetColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/CollisionProfile.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "OutlinerCollisionPresetColumn"

FOutlinerCollisionPresetColumn::FOutlinerCollisionPresetColumn(ISceneOutliner& InSceneOutliner)
	: FOutlinerToolkitColumnBase(InSceneOutliner)
{
	EnsureCollisionProfileOptions();
}

FName FOutlinerCollisionPresetColumn::GetColumnID()
{
	return GetID();
}

SHeaderRow::FColumn::FArguments FOutlinerCollisionPresetColumn::ConstructHeaderRowColumn()
{
	return SHeaderRow::Column(GetColumnID())
		.FixedWidth(140.0f)
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Fill)
		.VAlignCell(VAlign_Center)
		.DefaultLabel(LOCTEXT("CollisionPresetColumnLabel", "Collision Preset"))
		[
			SNew(STextBlock)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Justification(ETextJustify::Center)
				.Text(LOCTEXT("CollisionPresetColumnHeader", "Collision Preset"))
		];
}

bool FOutlinerCollisionPresetColumn::SupportsSorting() const
{
	return false;
}

const TSharedRef<SWidget> FOutlinerCollisionPresetColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
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

	EnsureCollisionProfileOptions();
	if (CollisionProfileOptions.IsEmpty())
	{
		return SNullWidget::NullWidget;
	}

	TSharedPtr<FString> InitiallySelected;
	const TOptional<FName> UniformProfileName = GetUniformCollisionProfileName(PrimitiveComponents);
	if (UniformProfileName.IsSet())
	{
		InitiallySelected = FindProfileOption(UniformProfileName.GetValue());
	}

	const TWeakObjectPtr<AActor> WeakActor = Actor;

	return SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&CollisionProfileOptions)
		.InitiallySelectedItem(InitiallySelected)
		.OnGenerateWidget(this, &FOutlinerCollisionPresetColumn::MakeCollisionPresetOptionWidget)
		.OnSelectionChanged_Lambda([this, WeakActor](TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
			{
				OnCollisionPresetChanged(NewValue, SelectInfo, WeakActor);
			})
		.ToolTipText(LOCTEXT("CollisionPresetTooltip", "Set collision preset for all primitive components on this actor."))
		.ComboBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>("SimpleComboBox"))
		[
			SNew(STextBlock)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Justification(ETextJustify::Center)
				.Text(TAttribute<FText>::CreateSP(this, &FOutlinerCollisionPresetColumn::GetCollisionPresetText, WeakActor))
		];
}

void FOutlinerCollisionPresetColumn::EnsureCollisionProfileOptions()
{
	if (!CollisionProfileOptions.IsEmpty())
	{
		return;
	}

	TArray<TSharedPtr<FName>> ProfileNames;
	UCollisionProfile::GetProfileNames(ProfileNames);

	TArray<FString> ProfileNameStrings;
	ProfileNameStrings.Reserve(ProfileNames.Num());

	for (const TSharedPtr<FName>& ProfileName : ProfileNames)
	{
		if (!ProfileName.IsValid() || ProfileName->IsNone())
		{
			continue;
		}

		ProfileNameStrings.AddUnique(ProfileName->ToString());
	}

	ProfileNameStrings.Sort();

	CollisionProfileOptions.Reserve(ProfileNameStrings.Num());
	for (const FString& ProfileNameString : ProfileNameStrings)
	{
		CollisionProfileOptions.Add(MakeShared<FString>(ProfileNameString));
	}
}

TSharedPtr<FString> FOutlinerCollisionPresetColumn::FindProfileOption(FName ProfileName) const
{
	const FString ProfileNameString = ProfileName.ToString();
	for (const TSharedPtr<FString>& Option : CollisionProfileOptions)
	{
		if (Option.IsValid() && *Option == ProfileNameString)
		{
			return Option;
		}
	}

	return nullptr;
}

TOptional<FName> FOutlinerCollisionPresetColumn::GetUniformCollisionProfileName(const TArray<UPrimitiveComponent*>& PrimitiveComponents) const
{
	return OutlinerColumnUtils::GetUniformComponentValue<FName>([](const UPrimitiveComponent* Component)
		{
			return Component ? Component->GetCollisionProfileName() : NAME_None;
		},
		PrimitiveComponents);
}

FText FOutlinerCollisionPresetColumn::GetCollisionPresetText(TWeakObjectPtr<AActor> WeakActor) const
{
	if (!WeakActor.IsValid())
	{
		return LOCTEXT("UnavailableCollisionPreset", "N/A");
	}

	const TArray<UPrimitiveComponent*> PrimitiveComponents = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get());
	if (PrimitiveComponents.IsEmpty())
	{
		return LOCTEXT("UnavailableCollisionPreset", "N/A");
	}

	const TOptional<FName> UniformProfileName = GetUniformCollisionProfileName(PrimitiveComponents);
	if (!UniformProfileName.IsSet())
	{
		return LOCTEXT("MixedCollisionPreset", "Mixed");
	}

	return FText::FromName(UniformProfileName.GetValue());
}

TSharedRef<SWidget> FOutlinerCollisionPresetColumn::MakeCollisionPresetOptionWidget(TSharedPtr<FString> Option) const
{
	return SNew(STextBlock)
		.Text(Option.IsValid() ? FText::FromString(*Option) : FText::GetEmpty());
}

void FOutlinerCollisionPresetColumn::OnCollisionPresetChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo, TWeakObjectPtr<AActor> WeakActor)
{
	if (SelectInfo == ESelectInfo::Direct || !NewValue.IsValid() || !WeakActor.IsValid())
	{
		return;
	}

	const FName NewProfileName(**NewValue);
	const TArray<UPrimitiveComponent*> PrimitiveComponents = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(WeakActor.Get());
	if (PrimitiveComponents.IsEmpty())
	{
		return;
	}

	const TOptional<FName> CurrentProfileName = GetUniformCollisionProfileName(PrimitiveComponents);
	if (CurrentProfileName.IsSet() && CurrentProfileName.GetValue() == NewProfileName)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetCollisionPresetTransaction", "Set Collision Preset"));
	OutlinerColumnUtils::ApplyToComponents(PrimitiveComponents,
		[NewProfileName](UPrimitiveComponent* Component)
		{
			Component->SetCollisionProfileName(NewProfileName);
		});

	if (WeakSceneOutliner.IsValid())
	{
		WeakSceneOutliner.Pin()->FullRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
