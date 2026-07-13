// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "FunctionLibraries/DataAssetManagerFunctionLibrary.h"

#include "AssetManagerEditorModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "DataAssetManagerTypes.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "Engine/DataAsset.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "IDetailRootObjectCustomization.h"
#include "Logging/DataAssetManagerLog.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "Utils/DataAssetManagerPathUtils.h"

DEFINE_LOG_CATEGORY(SDataAssetManagerLog)

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

	const double SizeInKb = StaticCast<double>(FileSize) / ConversionFactor;
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
	if (!bCopyPaths)
	{
		return Item.GetExportTextName();
	}

	const FString ItemFilename = FPackageName::LongPackageNameToFilename(Item.PackageName.ToString(), FPackageName::GetAssetPackageExtension());
	if (FPaths::FileExists(ItemFilename))
	{
		return FPaths::ConvertRelativePathToFull(ItemFilename);
	}

	return FString::Printf(TEXT("%s: No file on disk"), *Item.AssetName.ToString());
}

void DataAssetManager::ResetToCDO(const FDetailsObjectSet& InRootObjectSet)
{
	for (const UObject* ConstObject : InRootObjectSet.RootObjects)
	{
		UObject* Object = const_cast<UObject*>(ConstObject);
		if (!Object)
		{
			continue;
		}

		UObject* CDO = Object->GetClass()->GetDefaultObject();
		if (!CDO)
		{
			continue;
		}

		for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
		{
			const FProperty* Property = *PropIt;
			if (!Property || Property->HasAnyPropertyFlags(CPF_Transient | CPF_SkipSerialization | CPF_NonTransactional))
			{
				continue;
			}

			void* const DestPtr = Property->ContainerPtrToValuePtr<void>(Object);
			const void* const SrcPtr = Property->ContainerPtrToValuePtr<void>(CDO);
			Property->CopyCompleteValue(DestPtr, SrcPtr);
		}

		Object->PostEditChange();
		Object->MarkPackageDirty();
	}
}
