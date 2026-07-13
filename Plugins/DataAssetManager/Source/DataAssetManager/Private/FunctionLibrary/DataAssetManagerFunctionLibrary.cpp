// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"

#include "AssetManagerEditorModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "DataAssetManagerTypes.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "IDetailRootObjectCustomization.h"
#include "JsonObjectConverter.h"
#include "Logging/DataAssetManagerLog.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"
#include "Utils/DataAssetManagerPathUtils.h"

namespace
{
	bool ShouldResetPropertyToDefault(const FProperty* Property)
	{
		if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit))
		{
			return false;
		}

		constexpr EPropertyFlags IgnoredFlags =
			CPF_Transient |
			CPF_Deprecated |
			CPF_DuplicateTransient |
			CPF_NonPIEDuplicateTransient;

		return !Property->HasAnyPropertyFlags(IgnoredFlags);
	}

	bool ResetEditablePropertiesToCDO(UObject* MutableObject, const UObject* DefaultObject)
	{
		bool bChanged = false;

		for (TFieldIterator<FProperty> PropertyIterator(MutableObject->GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIterator; ++PropertyIterator)
		{
			const FProperty* Property = *PropertyIterator;
			if (!ShouldResetPropertyToDefault(Property) || DataAssetManager::ArePropertyValuesIdentical(Property, MutableObject, DefaultObject))
			{
				continue;
			}

			if (!bChanged)
			{
				MutableObject->Modify();
				bChanged = true;
			}

			Property->CopyCompleteValue_InContainer(MutableObject, DefaultObject);
		}

		return bChanged;
	}
}

bool DataAssetManager::ArePropertyValuesIdentical(const FProperty* Property, const UObject* LeftObject, const UObject* RightObject)
{
	if (!Property || !LeftObject || !RightObject)
	{
		return true;
	}

	for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
	{
		if (!Property->Identical_InContainer(LeftObject, RightObject, Index))
		{
			return false;
		}
	}

	return true;
}

FString DataAssetManager::GetAssetDiskSize(const FAssetData& AssetData)
{
	constexpr double ConversionFactor = 1024.0;

	FString PackageFileName;
	if (!FPackageName::DoesPackageExist(AssetData.PackageName.ToString(), &PackageFileName))
	{
		return DataAssetManager::UnknownStr;
	}

	const int64 FileSize = IFileManager::Get().FileSize(*PackageFileName);
	if (FileSize == INDEX_NONE)
	{
		return DataAssetManager::UnknownStr;
	}

	const double SizeInKb = static_cast<double>(FileSize) / ConversionFactor;
	if (SizeInKb >= ConversionFactor)
	{
		return FString::Printf(TEXT("%.1f Mb"), SizeInKb / ConversionFactor);
	}

	return FString::Printf(TEXT("%.1f Kb"), SizeInKb);
}

bool DataAssetManager::DeleteMultiplyAsset(const TArray<FAssetData>& Assets, bool bShowConfirmation)
{
	if (Assets.Num() == 0)
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("%s No assets to delete!"), ANSI_TO_TCHAR(__FUNCTION__));
		return false;
	}

	const int32 DeletedCount = ObjectTools::DeleteAssets(Assets, bShowConfirmation);
	if (CVarDebugDataAssetManager.GetValueOnAnyThread())
	{
		UE_LOG(SDataAssetManagerLog, Log, TEXT("%s Deleted %d assets"), ANSI_TO_TCHAR(__FUNCTION__), DeletedCount);
	}

	return DeletedCount > 0;
}

const UDataAssetManagerSettings* DataAssetManager::GetPluginSettings()
{
	return GetDefault<UDataAssetManagerSettings>();
}

void DataAssetManager::CreateNewDataAsset(UClass* AssetClass, const FString& Directory)
{
	if (!AssetClass || !AssetClass->IsChildOf(UDataAsset::StaticClass()))
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("%s Invalid class provided for Data Asset creation."), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	FString AssetPath = Directory;
	FPaths::NormalizeDirectoryName(AssetPath);

	if (AssetPath.IsEmpty())
	{
		AssetPath = FDataAssetManagerPathUtils::GetRootPath();
	}
	else if (!AssetPath.StartsWith(TEXT("/")))
	{
		AssetPath = FDataAssetManagerPathUtils::GetRootPath() / AssetPath;
	}

	if (AssetPath.EndsWith(TEXT("/")))
	{
		AssetPath = AssetPath.LeftChop(1);
	}

	const FString BaseAssetName = TEXT("NewDataAsset");
	FString FinalAssetName = BaseAssetName;

	const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(DataAssetManager::ModuleName::AssetTools);
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);

	int32 Suffix = 1;
	FString TestPackageName = AssetPath / FinalAssetName;
	FString TestObjectPath = TestPackageName + TEXT(".") + FinalAssetName;
	while (AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(TestObjectPath)).IsValid())
	{
		FinalAssetName = BaseAssetName + FString::Printf(TEXT("_%d"), Suffix);
		TestPackageName = AssetPath / FinalAssetName;
		TestObjectPath = TestPackageName + TEXT(".") + FinalAssetName;
		++Suffix;
	}

	if (CVarDebugDataAssetManager.GetValueOnAnyThread())
	{
		UE_LOG(SDataAssetManagerLog, Log, TEXT("Creating asset: Name=%s, Path=%s, Class=%s"), *FinalAssetName, *AssetPath, *AssetClass->GetName());
	}

	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(FinalAssetName, AssetPath, AssetClass, nullptr);
	if (!NewAsset)
	{
		UE_LOG(SDataAssetManagerLog, Error, TEXT("Failed to create Data Asset: Name=%s, Path=%s"), *FinalAssetName, *AssetPath);
		return;
	}

	NewAsset->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewAsset);

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(DataAssetManager::ModuleName::ContentBrowser);
	TArray<UObject*> AssetsToSync;
	AssetsToSync.Add(NewAsset);
	ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
}

void DataAssetManager::ProcessAssetData(const TArray<FAssetData>& RefAssetData, TFunction<void(const TArray<FAssetIdentifier>&)> ProcessFunction)
{
	TArray<FAssetIdentifier> AssetIdentifiers;
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(RefAssetData, AssetIdentifiers);
	ProcessFunction(AssetIdentifiers);
}

FString DataAssetManager::BuildClipboardEntry(const FAssetData& Item, bool bCopyPaths)
{
	if (!Item.IsValid())
	{
		return FString();
	}

	if (!bCopyPaths)
	{
		return Item.GetExportTextName();
	}

	FString PackageFileName;
	if (FPackageName::DoesPackageExist(Item.PackageName.ToString(), &PackageFileName))
	{
		return FPaths::ConvertRelativePathToFull(PackageFileName);
	}

	return FPaths::ConvertRelativePathToFull(
		FPackageName::LongPackageNameToFilename(Item.PackageName.ToString(), FPackageName::GetAssetPackageExtension()));
}

void DataAssetManager::ResetToCDO(const FDetailsObjectSet& InRootObjectSet)
{
	FScopedTransaction Transaction(NSLOCTEXT("DataAssetManager", "ResetToCDO", "Reset Data Asset To Default"));
	bool bResetAnyObject = false;

	for (const UObject* RootObject : InRootObjectSet.RootObjects)
	{
		if (!IsValid(RootObject) || !RootObject->GetClass() || RootObject->HasAnyFlags(RF_ClassDefaultObject))
		{
			continue;
		}

		UObject* MutableObject = const_cast<UObject*>(RootObject);
		UObject* DefaultObject = MutableObject->GetClass()->GetDefaultObject();
		if (!IsValid(DefaultObject))
		{
			continue;
		}

		if (ResetEditablePropertiesToCDO(MutableObject, DefaultObject))
		{
			MutableObject->PostEditChange();
			MutableObject->MarkPackageDirty();
			bResetAnyObject = true;
		}
	}

	if (!bResetAnyObject)
	{
		Transaction.Cancel();
	}
}

FString DataAssetManager::PathNormalize(const FString& InPath)
{
	return FDataAssetManagerPathUtils::Normalize(InPath);
}

TTuple<FString, FString> DataAssetManager::GetNormalizedAndProjectPath(const FString& InPath)
{
	return FDataAssetManagerPathUtils::GetNormalizedAndProjectPath(InPath);
}

FString DataAssetManager::PathConvert(const FString& InPath, bool bToAbsolute)
{
	return FDataAssetManagerPathUtils::Convert(InPath, bToAbsolute);
}

FString DataAssetManager::PathConvertToAbsolute(const FString& InPath)
{
	return FDataAssetManagerPathUtils::ConvertToAbsolute(InPath);
}

FString DataAssetManager::PathConvertToRelative(const FString& InPath)
{
	return FDataAssetManagerPathUtils::ConvertToRelative(InPath);
}

bool DataAssetManager::FolderIsEmpty(const FString& InPath)
{
	return FDataAssetManagerPathUtils::IsFolderEmpty(InPath);
}

FString DataAssetManager::GetPathExternalActors()
{
	return FDataAssetManagerPathUtils::GetExternalActorsPath();
}

FString DataAssetManager::GetPathExternalObjects()
{
	return FDataAssetManagerPathUtils::GetExternalObjectsPath();
}

bool DataAssetManager::FolderIsExternal(const FString& InPath)
{
	return FDataAssetManagerPathUtils::IsExternalFolder(InPath);
}

bool DataAssetManager::SaveDataAssetToJsonFile(const UDataAsset* DataAsset, const FString& FilePath)
{
	if (!IsValid(DataAsset))
	{
		return false;
	}

	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(DataAsset->GetClass(), DataAsset, JsonString, 0, 0))
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to serialize Data Asset to JSON"));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to save JSON file: %s"), *FilePath);
		return false;
	}

	if (CVarDebugDataAssetManager.GetValueOnAnyThread())
	{
		UE_LOG(SDataAssetManagerLog, Log, TEXT("Data Asset saved to JSON: %s"), *FilePath);
	}

	return true;
}

bool DataAssetManager::LoadDataAssetFromJsonFile(UDataAsset* DataAsset, const FString& FilePath)
{
	if (!IsValid(DataAsset))
	{
		return false;
	}

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to load JSON file: %s"), *FilePath);
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to parse JSON from file: %s"), *FilePath);
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), DataAsset->GetClass(), DataAsset, 0, 0))
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to deserialize JSON to Data Asset"));
		return false;
	}

	DataAsset->PostEditChange();
	DataAsset->MarkPackageDirty();

	if (CVarDebugDataAssetManager.GetValueOnAnyThread())
	{
		UE_LOG(SDataAssetManagerLog, Log, TEXT("Data Asset loaded from JSON: %s"), *FilePath);
	}

	return true;
}
