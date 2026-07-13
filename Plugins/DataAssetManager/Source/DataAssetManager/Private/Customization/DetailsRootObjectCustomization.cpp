// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Customization/DetailsRootObjectCustomization.h"

#include "AssetRegistry/AssetData.h"
#include "DetailLayoutBuilder.h"
#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "JsonObjectConverter.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDesktopPlatform.h"
#include "ScopedTransaction.h"
#include "Services/DataAssetManagerAssetService.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Engine/DataAsset.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"

namespace
{
	const FString JsonFileDialogFilter = TEXT("JSON files (*.json)|*.json");

	const void* GetParentWindowHandle()
	{
		return FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	}

	bool PickJsonSaveFile(const UDataAsset* DataAsset, FString& OutFilePath)
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (!DesktopPlatform)
		{
			return false;
		}

		TArray<FString> SelectedFiles;
		const FString DefaultFileName = IsValid(DataAsset)
			? FString::Printf(TEXT("%s.json"), *DataAsset->GetName())
			: FString(TEXT("DataAsset.json"));

		if (!DesktopPlatform->SaveFileDialog(GetParentWindowHandle(),
			TEXT("Export Data Asset to JSON"),
			FPaths::ProjectSavedDir(),
			DefaultFileName,
			JsonFileDialogFilter,
			EFileDialogFlags::None,
			SelectedFiles)
			|| SelectedFiles.Num() == 0)
		{
			return false;
		}

		OutFilePath = SelectedFiles[0];
		if (!FPaths::GetExtension(OutFilePath).Equals(TEXT("json"), ESearchCase::IgnoreCase))
		{
			OutFilePath = FPaths::ChangeExtension(OutFilePath, TEXT("json"));
		}

		return true;
	}

	bool PickJsonOpenFile(FString& OutFilePath)
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (!DesktopPlatform)
		{
			return false;
		}

		TArray<FString> SelectedFiles;
		if (!DesktopPlatform->OpenFileDialog(GetParentWindowHandle(),
			TEXT("Import Data Asset from JSON"),
			FPaths::ProjectSavedDir(),
			TEXT(""),
			JsonFileDialogFilter,
			EFileDialogFlags::None,
			SelectedFiles)
			|| SelectedFiles.Num() == 0)
		{
			return false;
		}

		OutFilePath = SelectedFiles[0];
		return true;
	}

	bool SaveDataAssetToJsonFile(const UDataAsset* DataAsset, const FString& FilePath)
	{
		if (!DataAsset)
		{
			return false;
		}

		FString JsonString;
		if (!FJsonObjectConverter::UStructToJsonObjectString(DataAsset->GetClass(), DataAsset, JsonString, 0, 0))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to serialize Data Asset to JSON"));
			return false;
		}

		if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to save JSON file: %s"), *FilePath);
			return false;
		}

		UE_LOG(LogTemp, Log, TEXT("Data Asset saved to JSON: %s"), *FilePath);
		return true;
	}

	bool LoadDataAssetFromJsonFile(UDataAsset* DataAsset, const FString& FilePath)
	{
		if (!IsValid(DataAsset))
		{
			return false;
		}

		FString JsonString;
		if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load JSON file: %s"), *FilePath);
			return false;
		}

		TSharedPtr<FJsonObject> JsonObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to parse JSON from file: %s"), *FilePath);
			return false;
		}

		FScopedTransaction Transaction(FText::FromString(TEXT("Import Data Asset from JSON")));
		DataAsset->Modify();

		if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), DataAsset->GetClass(), DataAsset, 0, 0))
		{
			Transaction.Cancel();
			UE_LOG(LogTemp, Warning, TEXT("Failed to deserialize JSON to Data Asset"));
			return false;
		}

		DataAsset->PostEditChange();
		DataAsset->MarkPackageDirty();

		UE_LOG(LogTemp, Log, TEXT("Data Asset loaded from JSON: %s"), *FilePath);
		return true;
	}
}

TSharedPtr<SWidget> FDetailsRootObjectCustomization::CustomizeObjectHeader( const FDetailsObjectSet& InRootObjectSet, const TSharedPtr<ITableRow>& InTableRow)
{
	CachedRootObjectSet = InRootObjectSet;
	const UObject* MainObject = InRootObjectSet.RootObjects.Num() > 0 ? InRootObjectSet.RootObjects[0] : nullptr;

	FSlateFontInfo BoldFont = IDetailLayoutBuilder::GetDetailFontBold();
	BoldFont.Size = 16;

	UDataAssetManagerSettings* Settings = GetMutableDefault<UDataAssetManagerSettings>();
	check(Settings);
	

	return SNew(SBorder)
		.Padding(4)
		.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
		.BorderBackgroundColor(Settings->RootCustomColor)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(FMargin(4.0f, 1.0f))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked_Lambda([InTableRow]()
				{
					if (InTableRow.IsValid())
					{
						InTableRow->ToggleExpansion();
					}
					return FReply::Handled();
				})
				.Cursor(EMouseCursor::Hand)
				.ToolTipText(FText::FromString("Expand / Collapse"))
				[
					SNew(SImage)
					.Image_Lambda([InTableRow]()
					{
						if (!InTableRow.IsValid())
						{
							return FSlateIconFinder::FindIcon("Icons.Plus").GetIcon();
						}

						return InTableRow->IsItemExpanded()
							? FSlateIconFinder::FindIcon("Icons.Minus").GetIcon()
							: FSlateIconFinder::FindIcon("Icons.Plus").GetIcon();
					})
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(IsValid(MainObject)
					? FText::FromString(MainObject->GetName())
					: FText::FromString(TEXT("Invalid Object")))
				.Font(BoldFont)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.HasDownArrow(true)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString(""))
				]
				.MenuContent()
				[
					SNew(SBox)
					.MinDesiredWidth(200.0f)
					[
						BuildHeaderMenu(InRootObjectSet)
					]
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(FMargin(4.0f, 2.0f))
				.ToolTipText(FText::FromString("Find in Content Browser"))
				.OnClicked_Lambda([MainObject]()
				{
					if (MainObject)
					{
						TArray<UObject*> Objects;
						Objects.Add(const_cast<UObject*>(MainObject));
						GEditor->SyncBrowserToObjects(Objects);
					}
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Search"))
				]
			]
		];
}

IDetailRootObjectCustomization::EExpansionArrowUsage FDetailsRootObjectCustomization::GetExpansionArrowUsage() const
{
	return EExpansionArrowUsage::Custom;
}

TSharedRef<SWidget> FDetailsRootObjectCustomization::BuildHeaderMenu(const FDetailsObjectSet& InRootObjectSet)
{
	CachedRootObjectSet = InRootObjectSet;
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection("Actions", FText::FromString("Actions"));
	MenuBuilder.AddMenuEntry(
		FText::FromString("Reset To Default (CDO)"),
		FText::FromString("Reset all property values to their default state"),
		FSlateIcon(FSlateIconFinder::FindIcon("ContentBrowser.ResetPrimitiveToDefault")), 
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			DataAssetManager::ResetToCDO(CachedRootObjectSet);
		})));

	MenuBuilder.AddMenuEntry(
		FText::FromString("Validate"),
		FText::FromString("Validate this Data Asset and show the result in Message Log"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			ValidateDataAsset();
		})));

	MenuBuilder.AddMenuEntry(
		FText::FromString("Copy Reference"),
		FText::FromString("Copy the Data Asset reference to clipboard"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			CopyReferenceToClipboard();
		})));

	MenuBuilder.AddMenuEntry(
		FText::FromString("Copy Path"),
		FText::FromString("Copy the Data Asset package filename to clipboard"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			CopyPathToClipboard();
		})));

	MenuBuilder.AddMenuEntry(
		FText::FromString("Export to JSON..."),
		FText::FromString("Save current Data Asset properties to JSON file"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "FontEditor.ExportPage"), // FSlateIcon(FName("EditorStyle"), "FontEditor.ExportPage")
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			ExportToJson();
		})));

	MenuBuilder.AddMenuEntry(
		FText::FromString("Import from JSON..."),
		FText::FromString("Load Data Asset properties from JSON file"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Persona.ReimportAsset"), // FSlateIcon(FName("EditorStyle"), "Persona.ReimportAsset")
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			ImportFromJson();
		})));

	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

void FDetailsRootObjectCustomization::ExportToJson()
{
	const UDataAsset* DataAsset = GetRootDataAsset();
	if (!IsValid(DataAsset))
	{
		return;
	}

	FString FilePath;
	if (!PickJsonSaveFile(DataAsset, FilePath))
	{
		return;
	}

	if (SaveDataAssetToJsonFile(DataAsset, FilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("Export successful: %s"), *FilePath);
	}
}

void FDetailsRootObjectCustomization::ImportFromJson()
{
	UDataAsset* DataAsset = GetRootDataAsset();
	if (!IsValid(DataAsset))
	{
		return;
	}

	FString FilePath;
	if (!PickJsonOpenFile(FilePath))
	{
		return;
	}

	if (LoadDataAssetFromJsonFile(DataAsset, FilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("Import successful: %s"), *FilePath);
	}
}

void FDetailsRootObjectCustomization::CopyReferenceToClipboard() const
{
	const UDataAsset* DataAsset = GetRootDataAsset();
	if (!IsValid(DataAsset))
	{
		return;
	}

	const FAssetData AssetData(DataAsset);
	const FString ClipboardText = DataAssetManager::BuildClipboardEntry(AssetData, false);
	FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);
}

void FDetailsRootObjectCustomization::CopyPathToClipboard() const
{
	const UDataAsset* DataAsset = GetRootDataAsset();
	if (!IsValid(DataAsset))
	{
		return;
	}

	const FAssetData AssetData(DataAsset);
	const FString ClipboardText = DataAssetManager::BuildClipboardEntry(AssetData, true);
	FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);
}

void FDetailsRootObjectCustomization::ValidateDataAsset() const
{
	const UDataAsset* DataAsset = GetRootDataAsset();
	if (!IsValid(DataAsset))
	{
		return;
	}

	const TArray<TSharedPtr<FAssetData>> AssetsToValidate = { MakeShared<FAssetData>(DataAsset) };
	FDataAssetManagerAssetService::ValidateAssets(AssetsToValidate, true);
}

UDataAsset* FDetailsRootObjectCustomization::GetRootDataAsset() const
{
	if (CachedRootObjectSet.RootObjects.Num() == 0)
	{
		return nullptr;
	}

	const UObject* ConstObject = CachedRootObjectSet.RootObjects[0];
	UDataAsset* DataAsset = Cast<UDataAsset>(const_cast<UObject*>(ConstObject));
	if (!DataAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Root object is not a UDataAsset"));
	}

	return DataAsset;
}
