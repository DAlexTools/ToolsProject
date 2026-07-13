// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Columns/OutlinerMobilityColumn.h"
#include "ComponentTreeItem.h"
#include "ActorTreeItem.h"
#include "SceneOutliner.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/STextComboBox.h"
#include "Styling/CoreStyle.h"


FOutlinerMobilityColumn::FOutlinerMobilityColumn(ISceneOutliner& SceneOutliner)
	: FOutlinerToolkitColumnBase(SceneOutliner)
{
}

FName FOutlinerMobilityColumn::GetColumnID()
{
	return GetID();
}

SHeaderRow::FColumn::FArguments FOutlinerMobilityColumn::ConstructHeaderRowColumn()
{
	return	SHeaderRow::Column(GetColumnID())
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.DefaultLabel(FText::FromString("Set Mobility"))
		.HeaderContent()
		[
			SNew(STextBlock)
				.Text(FText::FromString("Set Mobility"))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FSlateColor::UseForeground())
		];
}

const TSharedRef<SWidget> FOutlinerMobilityColumn::ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row)
{
	const FActorTreeItem* ActorTreeItem = TreeItem->CastTo<FActorTreeItem>();
	if (!ActorTreeItem || !ActorTreeItem->IsValid())
	{
		return SNullWidget::NullWidget;
	}

	AActor* Actor = ActorTreeItem->Actor.Get();
	if (!Actor)
	{
		return SNullWidget::NullWidget;
	}

	USceneComponent* RootComponent = Actor->GetRootComponent();
	if (!RootComponent)
	{
		return SNullWidget::NullWidget;
	}

	const TWeakObjectPtr<USceneComponent> WeakComponent = RootComponent;

	auto GetMobilityAsText = [WeakComponent]() -> FText
		{
			if (!WeakComponent.IsValid())
			{
				return FText::FromString("N/A");
			}

			switch (WeakComponent->Mobility)
			{
				case EComponentMobility::Static:
				{
					return FText::FromString("Static");
				}
				case EComponentMobility::Stationary:
				{
					return FText::FromString("Stationary");
				}
				case EComponentMobility::Movable:
				{
					return FText::FromString("Movable");
				}
				default:
				{
					return FText::FromString("Unknown");
				}
			}
		};

	auto OnMobilityChanged = [WeakComponent, this](TSharedPtr<FString> NewValue, ESelectInfo::Type)
		{
			if (!WeakComponent.IsValid() || !NewValue.IsValid())
			{
				return;
			}

			USceneComponent* Component = WeakComponent.Get();
			if (!Component)
			{
				return;
			}

			const FScopedTransaction Transaction(NSLOCTEXT("OutlinerMobilityColumn", "SetComponentMobilityTransaction", "Set Component Mobility"));
			Component->Modify();

			if (*NewValue == "Static")
			{
				Component->SetMobility(EComponentMobility::Static);
			}
			else if (*NewValue == "Stationary")
			{
				Component->SetMobility(EComponentMobility::Stationary);
			}
			else if (*NewValue == "Movable")
			{
				Component->SetMobility(EComponentMobility::Movable);
			}

			if (WeakSceneOutliner.IsValid())
			{
				WeakSceneOutliner.Pin()->FullRefresh();
			}
		};


	if (MobilityOptions.Num() == 0)
	{
		MobilityOptions.Add(MakeShared<FString>("Static"));
		MobilityOptions.Add(MakeShared<FString>("Stationary"));
		MobilityOptions.Add(MakeShared<FString>("Movable"));
	}

	TSharedPtr<FString> InitiallySelected;
	const FString CurrentMobility = GetMobilityAsText().ToString();

	for (const TSharedPtr<FString>& Option : MobilityOptions)
	{
		if (*Option == CurrentMobility)
		{
			InitiallySelected = Option;
			break;
		}
	}

	if (!InitiallySelected.IsValid() && MobilityOptions.Num() > 0)
	{
		InitiallySelected = MobilityOptions[0];
	}

	return	SNew(STextComboBox)
		.OptionsSource(&MobilityOptions)
		.InitiallySelectedItem(InitiallySelected)
		.OnSelectionChanged_Lambda(OnMobilityChanged)
		.ToolTipText(FText::FromString("Change component mobility"))
		.ComboBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>("SimpleComboBox"));
}
