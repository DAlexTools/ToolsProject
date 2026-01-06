// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SDataAssetTableRow.h"
#include "Styling/SlateIconFinder.h"

void SDataAssetTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Item = InArgs._Item;
	AddDirtyEventHandler(Item->PackageName.ToString());

	OnAssetRenamed = InArgs._OnAssetRenamed;
	OnCreateContextMenu = InArgs._OnCreateContextMenu;
	OnAssetDoubleClicked = InArgs._OnAssetDoubleClicked;
	OnRegisterEditableText = InArgs._OnRegisterEditableText;
	MouseButtonDown = InArgs._OnMouseButtonDown;

	SMultiColumnTableRow::Construct(FSuperRowType::FArguments()
		.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow"), InOwnerTable);
}


SDataAssetTableRow::~SDataAssetTableRow()
{
	if (OnPackageDirtyStateChangedHandle.IsValid())
	{
		UPackage::PackageDirtyStateChangedEvent.Remove(OnPackageDirtyStateChangedHandle);
	}
}

void SDataAssetTableRow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (DirtyBrushWidget.IsValid())
	{
		/**  Show dirty (unsaved) badge if the asset's package is marked dirty */
		DirtyBrushWidget->SetVisibility(bIsDirty ? EVisibility::Visible : EVisibility::Collapsed);
	}
}

TSharedRef<SWidget> SDataAssetTableRow::GenerateWidgetForColumn(const FName& ColumnId) 
{
	if (ColumnId == DataAssetListColumns::ColumnID_Name)
	{
		TSharedRef<SEditableText> EditableText =
			SNew(SEditableText)
			.Cursor(EMouseCursor::Hand)
			.HintText(FText::FromName(Item->PackagePath))
			.Text_Lambda([this]() { return FText::FromName(Item->AssetName); })
			.SelectAllTextWhenFocused(true)
			.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type Type)
				{
					if (OnAssetRenamed.IsBound() && Type == ETextCommit::OnEnter)
					{
						OnAssetRenamed.Execute(Item, Text, Type);
					}
				});

		if (OnRegisterEditableText.IsBound())
		{
			OnRegisterEditableText.Execute(Item, EditableText);
		}

		TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);
		HorizontalBox->AddSlot().HAlign(HAlign_Left).AutoWidth()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("ContentBrowser.ColumnViewAssetIcon"))
					.ColorAndOpacity(FColor::FromHex("616161FF"))
				]
				
				+SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Bottom)
				[
					SAssignNew(DirtyBrushWidget, SImage)
					.Image(FAppStyle::GetBrush("Icons.DirtyBadge"))
					.Visibility_Lambda([this] {return bIsDirty ? EVisibility::Visible : EVisibility::Collapsed; })
				]
			];

		HorizontalBox->AddSlot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
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
					.OnMouseButtonDown(SharedThis(this), &SDataAssetTableRow::OnMouseButtonClickedHandler)
					.OnMouseDoubleClick(SharedThis(this), &SDataAssetTableRow::OnMouseDoubleButtonClickedHandler)
				]
			];

		return HorizontalBox;
	}
	else if (ColumnId == DataAssetListColumns::ColumnID_Type)
	{
		// bug fix in 5.5 version GetClass() returned nullptr on some asset classes
		return SNew(STextBlock).Text(FText::FromName(Item.IsValid() ? Item->AssetClassPath.GetAssetName() : NAME_None));
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
		const FString AssetPath = FPackageName::LongPackageNameToFilename(Item->PackageName.ToString(), FPackageName::GetAssetPackageExtension());
		FSourceControlStatePtr SourceControlState = ISourceControlModule::Get().GetProvider().GetState(AssetPath, EStateCacheUsage::Use);
		const FSlateBrush* IconBrush = FSlateIconFinder::FindIcon("SourceControl.Settings.StatusBorder").GetOptionalIcon();

		if (SourceControlState.IsValid())
		{
			if (SourceControlState->IsCheckedOut())
			{
				IconBrush = FSlateIconFinder::FindIcon("SourceControl.StatusIcon.On").GetOptionalIcon();
			}
			else if (SourceControlState->IsModified())
			{
				IconBrush = FAppStyle::GetBrush("SourceControl.Modified");
			}
			else if (SourceControlState->IsSourceControlled())
			{
				IconBrush = FAppStyle::GetBrush("SourceControl.CheckedIn");
			}
			else
			{
				IconBrush = FAppStyle::GetBrush("SourceControl.NotUnderSourceControl");
			}
		}

		return SNew(SImage).Image(IconBrush).ColorAndOpacity(FColor::Transparent);
	}

	return SNullWidget::NullWidget;
}

void SDataAssetTableRow::AddDirtyEventHandler(const FString& PackageName)
{
	//CurrentPackageName = PackageName;

	//if (Item.IsValid())
	//{
	//	OnPackageDirtyStateChangedHandle = UPackage::PackageMarkedDirtyEvent.AddRaw(this, &SDataAssetTableRow::PackageDirtyDesc);
	//	OnPackageSavedHandle = UPackage::PackageSavedEvent.AddRaw(this, &SDataAssetTableRow::PackageSavedDesc);
	//}

	if (Item.IsValid())
	{
		OnPackageDirtyStateChangedHandle =
			UPackage::PackageDirtyStateChangedEvent.AddLambda(
				[this, PackageName](UPackage* DirtyPackage)
				{
					if (DirtyPackage && DirtyPackage->GetName() == PackageName)
					{
						bIsDirty = DirtyPackage->IsDirty();
					}
				});
	}
}

FReply SDataAssetTableRow::OnMouseButtonClickedHandler(const FGeometry& InGeometry, const FPointerEvent& InPointerEvent)
{
	if (OnCreateContextMenu.IsBound() && InPointerEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		OnCreateContextMenu.Execute(InGeometry, InPointerEvent);
		return FReply::Handled();
	}

	if (MouseButtonDown.IsBound())
	{
		return MouseButtonDown.Execute(InGeometry, InPointerEvent);
	}

	return FReply::Unhandled();
}

FReply SDataAssetTableRow::OnMouseDoubleButtonClickedHandler(const FGeometry& InGeometry, const FPointerEvent& PointerEvent)
{
	if (OnAssetDoubleClicked.IsBound())
	{
		OnAssetDoubleClicked.Execute(InGeometry, PointerEvent);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

//void SDataAssetTableRow::PackageSavedDesc(const FString& SavedPackageFileName, UObject* PackageObj)
//{
//	if (PackageObj)
//	{
//		UPackage* SavedPackage = Cast<UPackage>(PackageObj);
//		if (SavedPackage && SavedPackage->FileName == CurrentPackageName)
//		{
//			if (DirtyBrushWidget.IsValid())
//			{
//				bIsDirty = false;
//			}
//		}
//	}
//}
//
//void SDataAssetTableRow::PackageDirtyDesc(UPackage* DirtyPackage, bool InDirty)
//{
//	if (DirtyPackage && DirtyPackage->FileName == CurrentPackageName)
//	{
//		if (DirtyBrushWidget.IsValid())
//		{
//			if (!OwnerTablePtr.IsValid() || !IsSelected()) return;
//
//			bIsDirty = true;
//		}
//	}
//}
