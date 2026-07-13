// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "Tests/DataAssetManagerTestTypes.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"

namespace DataAssetManagerTests
{
	inline const FString TestRootPath = TEXT("/Game/DataAssetManagerTests");
	inline const FString TestMoveRootPath = TEXT("/Game/DataAssetManagerTestsMoved");

	inline FString MakeUniqueAssetName(const TCHAR* Prefix)
	{
		return FString::Printf(TEXT("%s_%s"), Prefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	inline FAssetData GetAssetDataForObject(const UObject* Asset)
	{
		if (!IsValid(Asset))
		{
			return FAssetData();
		}

		return FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssetByObjectPath(FSoftObjectPath(Asset));
	}

	inline bool SaveAssetPackage(UObject* Asset, FString& OutPackageFileName)
	{
		if (!Asset)
		{
			return false;
		}

		UPackage* const Package = Asset->GetOutermost();
		OutPackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.bWarnOfLongFilename = false;

		return UPackage::SavePackage(Package, Asset, *OutPackageFileName, SaveArgs);
	}

	inline void ScanTestPaths()
	{
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry")
			.Get()
			.ScanPathsSynchronous({ TestRootPath, TestMoveRootPath }, true);
	}

	inline void DeleteAssetsInPath(const FString& PackagePath)
	{
		TArray<FAssetData> AssetsToDelete;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry")
			.Get()
			.GetAssetsByPath(FName(*PackagePath), AssetsToDelete, true, false);

		if (AssetsToDelete.Num() > 0)
		{
			ObjectTools::DeleteAssets(AssetsToDelete, false);
		}

		FString DiskPath = FPaths::ProjectContentDir() / PackagePath.RightChop(FString(TEXT("/Game")).Len());
		IFileManager::Get().DeleteDirectory(*DiskPath, false, true);
	}

	inline void DeleteTestRootAssets()
	{
		DeleteAssetsInPath(TestRootPath);
		DeleteAssetsInPath(TestMoveRootPath);
		ScanTestPaths();
	}

	class FScopedTestDataAsset final
	{
	public:
		explicit FScopedTestDataAsset(
			const FString& AssetName,
			const FString& PackagePath = TestRootPath,
			TSubclassOf<UDataAsset> AssetClass = UTestDataAsset::StaticClass())
		{
			Asset = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools")
						.Get()
						.CreateAsset(AssetName, PackagePath, AssetClass.Get(), nullptr);
			RefreshAssetData();
		}

		~FScopedTestDataAsset()
		{
			if (AssetData.IsValid())
			{
				ObjectTools::DeleteAssets({ AssetData }, false);
			}

			if (!PackageFileName.IsEmpty())
			{
				IFileManager::Get().Delete(*PackageFileName);
			}

			ScanTestPaths();
		}

		UObject* GetAsset() const
		{
			return Asset;
		}

		template <typename TAsset>
		TAsset* GetAssetAs() const
		{
			return Cast<TAsset>(Asset);
		}

		const FAssetData& GetAssetData() const
		{
			return AssetData;
		}

		FAssetData RefreshAssetData()
		{
			AssetData = GetAssetDataForObject(Asset);
			return AssetData;
		}

		TSharedPtr<FAssetData> MakeSharedAssetData()
		{
			RefreshAssetData();
			return MakeShared<FAssetData>(AssetData);
		}

		bool Save()
		{
			const bool bSaved = SaveAssetPackage(Asset, PackageFileName);
			RefreshAssetData();
			return bSaved;
		}

	private:
		UObject* Asset = nullptr;
		FAssetData AssetData;
		FString PackageFileName;
	};
}
