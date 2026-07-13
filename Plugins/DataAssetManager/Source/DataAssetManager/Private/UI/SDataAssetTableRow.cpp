// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "UI/SDataAssetTableRow.h"
#include "Styling/SlateIconFinder.h"
#include "Styling/SlateBrush.h"
#include "UObject/Package.h"
#include "Widgets/Images/SLayeredImage.h"

void SDataAssetTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Item = InArgs._Item;
	if (Item.IsValid())
	{
		RefreshDirtyState();
		AddDirtyEventHandler(Item->PackageName.ToString());
	}

	OnAssetRenamed = InArgs._OnAssetRenamed;
	OnCreateContextMenu = InArgs._OnCreateContextMenu;
	OnAssetDoubleClicked = InArgs._OnAssetDoubleClicked;
	OnRegisterEditableText = InArgs._OnRegisterEditableText;
	OnGetValidationState = InArgs._OnGetValidationState;
	MouseButtonDown = InArgs._OnMouseButtonDown;

	ISourceControlModule::Get().RegisterProviderChanged(
		FSourceControlProviderChanged::FDelegate::CreateSP(this, &SDataAssetTableRow::HandleSourceControlProviderChanged));
	SourceControlStateChangedDelegateHandle = ISourceControlModule::Get().GetProvider().RegisterSourceControlStateChanged_Handle(
		FSourceControlStateChanged::FDelegate::CreateSP(this, &SDataAssetTableRow::HandleSourceControlStateChanged));

	SMultiColumnTableRow::Construct(FSuperRowType::FArguments().Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow"), InOwnerTable);
}

SDataAssetTableRow::~SDataAssetTableRow()
{
	if (OnPackageDirtyStateChangedHandle.IsValid())
	{
		UPackage::PackageDirtyStateChangedEvent.Remove(OnPackageDirtyStateChangedHandle);
	}

	if (ISourceControlModule::Get().IsEnabled() && SourceControlStateChangedDelegateHandle.IsValid())
	{
		ISourceControlModule::Get().GetProvider().UnregisterSourceControlStateChanged_Handle(SourceControlStateChangedDelegateHandle);
	}
}
/* clang-format off */
TSharedRef<SWidget> SDataAssetTableRow::GenerateWidgetForColumn(const FName& ColumnId)
{
	if (ColumnId == DataAssetListColumns::ColumnID_Name)
	{
		return BuildNameColumnWidget();
	}
	else if (ColumnId == DataAssetListColumns::ColumnID_Type)
	{
		return SNew(STextBlock) // bug fix in 5.5 version GetClass() returned nullptr on some asset classes
			.Text(FText::FromName(Item.IsValid() ? Item->AssetClassPath.GetAssetName() : NAME_None));
	}
	else if (ColumnId == DataAssetListColumns::ColumnID_DiskSize)
	{
		return SNew(STextBlock).Text(FText::FromString(DataAssetManager::GetAssetDiskSize(Item.ToSharedRef().Get())));
	}
	else if (ColumnId == DataAssetListColumns::ColumnID_Path)
	{
		return SNew(STextBlock).Text(FText::FromString(Item->PackagePath.ToString()));
	}
	else if (ColumnId == DataAssetListColumns::ColumnID_RC)
	{
		return SNew(SBox)
			.WidthOverride(16.0f)
			.HeightOverride(16.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				GenerateSourceControlIconWidget()
			];

		HandleSourceControlStateChanged();
	}
	else if (ColumnId == DataAssetListColumns::ColumnID_Validation)
	{
		return BuildValidationColumnWidget();
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SDataAssetTableRow::GenerateSourceControlIconWidget()
{
	TSharedRef<SLayeredImage> Image = SNew(SLayeredImage).Image(FStyleDefaults::GetNoBrush());
	SCCStateWidget = Image;

	return Image;
}

void SDataAssetTableRow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (DirtyBrushWidget.IsValid())
	{
		RefreshDirtyState();
		DirtyBrushWidget->SetVisibility(bIsDirty ? EVisibility::Visible : EVisibility::Collapsed);
	}
}

TSharedRef<SWidget> SDataAssetTableRow::BuildNameColumnWidget()
{
	TSharedRef<SEditableText> EditableText = SNew(SEditableText)
		.Cursor(EMouseCursor::Hand)
		.HintText(FText::FromName(Item->PackagePath))
		.Text(this, &SDataAssetTableRow::GetAssetNameText)
		.SelectAllTextWhenFocused(true)
		.OnTextCommitted(this, &SDataAssetTableRow::OnAssetNameCommitted);

	if (OnRegisterEditableText.IsBound())
	{
		OnRegisterEditableText.Execute(Item, EditableText);
	}

	TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

	HorizontalBox->AddSlot()
		.HAlign(HAlign_Left)
		.AutoWidth()
		[
			SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
						.Image(FAppStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon"))
						.ColorAndOpacity(FColor::FromHex("616161FF"))
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Bottom)
				[
					SAssignNew(DirtyBrushWidget, SImage)
						.Image(FAppStyle::GetBrush("Icons.DirtyBadge"))
						.Visibility(EVisibility::Collapsed)
				]
		];

	HorizontalBox->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SOverlay)
				+ SOverlay::Slot()
				.VAlign(VAlign_Center)
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					EditableText
				]

				+ SOverlay::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					SNew(SBox)
						.Visibility(EVisibility::Visible)
				]

				+ SOverlay::Slot()
				[
					SNew(SBorder)
						.Cursor(EMouseCursor::Hand)
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						.ColorAndOpacity(FColor::Transparent)
						.BorderBackgroundColor(FColor::Transparent)
						.OnMouseButtonDown(this, &SDataAssetTableRow::HandleMouseButtonDown)
						.OnMouseDoubleClick(this, &SDataAssetTableRow::HandleMouseDoubleClick)
				]
		];

	return HorizontalBox;
}
/* clang-format on */

TSharedRef<SWidget> SDataAssetTableRow::BuildValidationColumnWidget()
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
				{
					return GetValidationLabelText();
				})
			.ToolTipText_Lambda([this]()
				{
					return GetValidationTooltipText();
				})
			.ColorAndOpacity_Lambda([this]()
				{
					return GetValidationColor();
				})
		];
}

const FDataAssetValidationState* SDataAssetTableRow::GetValidationState() const
{
	return OnGetValidationState.IsBound()
		? OnGetValidationState.Execute(Item)
		: nullptr;
}

FText SDataAssetTableRow::GetValidationLabelText() const
{
	if (const FDataAssetValidationState* State = GetValidationState())
	{
		switch (State->Status)
		{
		case EDataAssetValidationStatus::Valid:
			return FText::FromString(TEXT("OK"));
		case EDataAssetValidationStatus::Warning:
			return FText::FromString(TEXT("Warn"));
		case EDataAssetValidationStatus::Invalid:
			return FText::FromString(TEXT("Error"));
		default:
			break;
		}
	}

	return FText::FromString(TEXT("-"));
}

FText SDataAssetTableRow::GetValidationTooltipText() const
{
	if (const FDataAssetValidationState* State = GetValidationState())
	{
		return State->Summary;
	}

	return FText::FromString(TEXT("Not validated."));
}

FSlateColor SDataAssetTableRow::GetValidationColor() const
{
	if (const FDataAssetValidationState* State = GetValidationState())
	{
		switch (State->Status)
		{
		case EDataAssetValidationStatus::Valid:
			return FSlateColor(FLinearColor(0.15f, 0.75f, 0.35f));
		case EDataAssetValidationStatus::Warning:
			return FSlateColor(FLinearColor(0.95f, 0.65f, 0.10f));
		case EDataAssetValidationStatus::Invalid:
			return FSlateColor(FLinearColor(0.95f, 0.20f, 0.18f));
		default:
			break;
		}
	}

	return FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f));
}

void SDataAssetTableRow::AddDirtyEventHandler(const FString& PackageName)
{
	if (Item.IsValid())
	{
		OnPackageDirtyStateChangedHandle = UPackage::PackageDirtyStateChangedEvent.AddLambda(
			[this, PackageName](UPackage* DirtyPackage)
			{
				if (DirtyPackage && DirtyPackage->GetName() == PackageName)
				{
					AssetPackage = DirtyPackage;
					bIsDirty = DirtyPackage->IsDirty();
				}
			});
	}
}

void SDataAssetTableRow::RefreshDirtyState()
{
	if (!Item.IsValid())
	{
		AssetPackage = nullptr;
		bIsDirty = false;
		return;
	}

	const FString PackageName = Item->PackageName.ToString();
	if (!AssetPackage || AssetPackage->GetName() != PackageName)
	{
		AssetPackage = FindPackage(nullptr, *PackageName);
	}

	bIsDirty = AssetPackage && AssetPackage->IsDirty();
}

void SDataAssetTableRow::HandleSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider)
{
	OldProvider.UnregisterSourceControlStateChanged_Handle(SourceControlStateChangedDelegateHandle);
	SourceControlStateChangedDelegateHandle = NewProvider.RegisterSourceControlStateChanged_Handle(
		FSourceControlStateChanged::FDelegate::CreateSP(this, &SDataAssetTableRow::HandleSourceControlStateChanged));

	// Reset this so the state will be queried from the new provider on the next Tick
	// SourceControlStateDelay = 0.0f;
	// bSourceControlStateRequested = false;
	HandleSourceControlStateChanged();
}

void SDataAssetTableRow::HandleSourceControlStateChanged()
{
	if (Item.IsValid() && ISourceControlModule::Get().IsEnabled())
	{
		FString AssetPath = FPackageName::LongPackageNameToFilename(Item->PackageName.ToString(), FPackageName::GetAssetPackageExtension());
		FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(AssetPath, EStateCacheUsage::Use);
		if (SourceControlState)
		{
			if (SCCStateWidget.IsValid())
			{
				FSlateIcon SCCIcon = SourceControlState->GetIcon();
				bHasCCStateBrush = SCCIcon.GetIcon() != FStyleDefaults::GetNoBrush();
				SCCStateWidget->SetFromSlateIcon(SCCIcon);
			}
		}
	}
}

FReply SDataAssetTableRow::HandleMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& MouseEvent)
{
	if (OnCreateContextMenu.IsBound() && MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		OnCreateContextMenu.Execute(InGeometry, MouseEvent);
		return FReply::Handled();
	}

	if (MouseButtonDown.IsBound())
	{
		return MouseButtonDown.Execute(InGeometry, MouseEvent);
	}

	return FReply::Unhandled();
}

FReply SDataAssetTableRow::HandleMouseDoubleClick(const FGeometry& InGeometry, const FPointerEvent& MouseEvent)
{
	if (OnAssetDoubleClicked.IsBound())
	{
		OnAssetDoubleClicked.Execute(InGeometry, MouseEvent);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FText SDataAssetTableRow::GetAssetNameText() const
{
	return FText::FromName(Item->AssetName);
}

void SDataAssetTableRow::OnAssetNameCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (OnAssetRenamed.IsBound() && CommitType == ETextCommit::OnEnter)
	{
		OnAssetRenamed.Execute(Item, Text, CommitType);
	}
}
