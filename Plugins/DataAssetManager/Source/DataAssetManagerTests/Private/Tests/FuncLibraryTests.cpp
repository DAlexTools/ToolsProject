// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "FunctionLibraries/DataAssetManagerFunctionLibrary.h"
#include "Tests/DataAssetManagerTestTypes.h"
#include "Utils/DataAssetManagerPathUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"

#if !UE_BUILD_SHIPPING && WITH_DEV_AUTOMATION_TESTS

namespace DataAssetManagerTests
{
	const FString TestRootPath = TEXT("/Game/DataAssetManagerTests");

	FString MakeUniqueAssetName(const TCHAR* Prefix)
	{
		return FString::Printf(TEXT("%s_%s"), Prefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FAssetData GetAssetDataForObject(const UObject* Asset)
	{
		if (!Asset)
		{
			return FAssetData();
		}

		return FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssetByObjectPath(FSoftObjectPath(Asset));
	}

	bool SaveAssetPackage(UObject* Asset, FString& OutPackageFileName)
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

	class FScopedTestDataAsset final
	{
	public:
		explicit FScopedTestDataAsset(const FString& AssetName, const FString& PackagePath = TestRootPath)
		{
			Asset = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools")
						.Get()
						.CreateAsset(AssetName, PackagePath, UTestDataAsset::StaticClass(), nullptr);
			AssetData = GetAssetDataForObject(Asset);
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

			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().ScanPathsSynchronous({ TestRootPath }, true);
		}

		UObject* GetAsset() const
		{
			return Asset;
		}

		const FAssetData& GetAssetData() const
		{
			return AssetData;
		}

		bool Save()
		{
			const bool bSaved = SaveAssetPackage(Asset, PackageFileName);
			AssetData = GetAssetDataForObject(Asset);
			return bSaved;
		}

	private:
		UObject* Asset = nullptr;
		FAssetData AssetData;
		FString PackageFileName;
	};

	void DeleteTestRootAssets()
	{
		TArray<FAssetData> AssetsToDelete;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssetsByPath(FName(*TestRootPath), AssetsToDelete, true, false);
		if (AssetsToDelete.Num() > 0)
		{
			ObjectTools::DeleteAssets(AssetsToDelete, false);
		}

		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().ScanPathsSynchronous({ TestRootPath }, true);
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FDataAssetManagerFunctionLibraryTest,
	"DataAssetManager.FunctionLibraryTests",
	DataAssetManagerFlags::Flags)

void FDataAssetManagerFunctionLibraryTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("PathUtils.GetDevelopersPath"));
	OutTestCommands.Add(TEXT("GetDevelopersPath"));

	OutBeautifiedNames.Add(TEXT("PathUtils.GetRootPath"));
	OutTestCommands.Add(TEXT("GetRootPath"));

	OutBeautifiedNames.Add(TEXT("GetAssetDiskSize"));
	OutTestCommands.Add(TEXT("GetAssetDiskSize"));

	OutBeautifiedNames.Add(TEXT("GetPluginSettings"));
	OutTestCommands.Add(TEXT("GetPluginSettings"));

	OutBeautifiedNames.Add(TEXT("DeleteMultiplyAsset"));
	OutTestCommands.Add(TEXT("DeleteMultiplyAsset"));

	OutBeautifiedNames.Add(TEXT("CreateNewDataAsset"));
	OutTestCommands.Add(TEXT("CreateNewDataAsset"));

	OutBeautifiedNames.Add(TEXT("ProcessAssetData"));
	OutTestCommands.Add(TEXT("ProcessAssetData"));

	OutBeautifiedNames.Add(TEXT("PathFunctions"));
	OutTestCommands.Add(TEXT("PathFunctions"));

	OutBeautifiedNames.Add(TEXT("PathUtils.IsFolderEmpty"));
	OutTestCommands.Add(TEXT("IsFolderEmpty"));

	OutBeautifiedNames.Add(TEXT("PathUtils.GetExternalActorsPath"));
	OutTestCommands.Add(TEXT("GetExternalActorsPath"));

	OutBeautifiedNames.Add(TEXT("PathUtils.GetExternalObjectsPath"));
	OutTestCommands.Add(TEXT("GetExternalObjectsPath"));

	OutBeautifiedNames.Add(TEXT("PathUtils.IsExternalFolder"));
	OutTestCommands.Add(TEXT("IsExternalFolder"));

	OutBeautifiedNames.Add(TEXT("BuildClipboardEntry"));
	OutTestCommands.Add(TEXT("BuildClipboardEntry"));
}

bool FDataAssetManagerFunctionLibraryTest::RunTest(const FString& Parameters)
{
	if (Parameters == TEXT("GetDevelopersPath"))
	{
		const FString DevPath = FDataAssetManagerPathUtils::GetDevelopersPath();
		TestEqual(TEXT("GetDevelopersPath returns /Game/Developers"), DevPath, FString(TEXT("/Game/Developers")));
	}

	if (Parameters == TEXT("GetRootPath"))
	{
		const FString RootPath{ FDataAssetManagerPathUtils::GetRootPath() };
		TestEqual(TEXT("GetRootPath returns /Game"), RootPath, FString(TEXT("/Game")));
	}

	if (Parameters == TEXT("GetPluginSettings"))
	{
		const UDataAssetManagerSettings* Settings = DataAssetManager::GetPluginSettings();
		TestNotNull(TEXT("GetPluginSettings should not return null"), Settings);
	}

	if (Parameters == TEXT("PathFunctions"))
	{
		const FString NormPath = FDataAssetManagerPathUtils::Normalize(FDataAssetManagerPathUtils::GetDevelopersPath());
		TestFalse(TEXT("Normalize should remove trailing slash"), NormPath.EndsWith(TEXT("/")));

		const FString AbsPath = FDataAssetManagerPathUtils::ConvertToAbsolute(TEXT("/Game/TestFolder"));
		const FString RelPath = FDataAssetManagerPathUtils::ConvertToRelative(AbsPath);
		TestTrue(TEXT("Convert roundtrip"), RelPath.Contains(TEXT("/Game/TestFolder")));
	}

	if (Parameters == TEXT("IsFolderEmpty"))
	{
		const FString FolderDiskPath = FPaths::ProjectContentDir() / TEXT("TempEmptyTest");
		IFileManager::Get().MakeDirectory(*FolderDiskPath, true);

		const FString FolderUEPath = TEXT("/Game/TempEmptyTest");

		bool bIsEmpty = FDataAssetManagerPathUtils::IsFolderEmpty(FolderUEPath);
		TestTrue(TEXT("IsFolderEmpty should detect empty folder"), bIsEmpty);

		const FString TempFile = FolderDiskPath / TEXT("TestFile.tmp");
		FFileHelper::SaveStringToFile(TEXT("Dummy"), *TempFile);

		bIsEmpty = FDataAssetManagerPathUtils::IsFolderEmpty(FolderUEPath);
		TestFalse(TEXT("IsFolderEmpty should detect non-empty folder"), bIsEmpty);

		IFileManager::Get().Delete(*TempFile);
		IFileManager::Get().DeleteDirectory(*FolderDiskPath, false, true);
	}

	if (Parameters == TEXT("GetExternalActorsPath"))
	{
		const FString Expected = FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalActorsFolderName());
		const FString Actual = FDataAssetManagerPathUtils::GetExternalActorsPath();

		TestEqual(TEXT("GetExternalActorsPath returns correct path"), Actual, Expected);
	}

	if (Parameters == TEXT("GetExternalObjectsPath"))
	{
		const FString Expected = FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalObjectsFolderName());
		const FString Actual = FDataAssetManagerPathUtils::GetExternalObjectsPath();

		TestEqual(TEXT("GetExternalObjectsPath returns correct path"), Actual, Expected);
	}

	if (Parameters == TEXT("IsExternalFolder"))
	{
		const FString ActorsPath = FDataAssetManagerPathUtils::GetExternalActorsPath();
		const FString ObjectsPath = FDataAssetManagerPathUtils::GetExternalObjectsPath();

		TestTrue(TEXT("Actors external folder detected"), FDataAssetManagerPathUtils::IsExternalFolder(ActorsPath));
		TestTrue(TEXT("Objects external folder detected"), FDataAssetManagerPathUtils::IsExternalFolder(ObjectsPath));
		TestFalse(TEXT("Regular folder should not be external"), FDataAssetManagerPathUtils::IsExternalFolder(TEXT("/Game/MyFolder")));
	}

	if (Parameters == TEXT("GetAssetDiskSize"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::FScopedTestDataAsset TempAsset(DataAssetManagerTests::MakeUniqueAssetName(TEXT("TempDiskSizeAsset")));

		TestNotNull(TEXT("Failed to create temporary asset"), TempAsset.GetAsset());

		const bool bSaved = TempAsset.Save();
		TestTrue(TEXT("Failed to save asset to disk"), bSaved);

		const FAssetData AssetData = TempAsset.GetAssetData();
		TestTrue(TEXT("AssetData should be valid"), AssetData.IsValid());

		const FString SizeStr = DataAssetManager::GetAssetDiskSize(AssetData);
		TestTrue(TEXT("GetAssetDiskSize returns valid size"), !SizeStr.IsEmpty() && SizeStr != TEXT("Unknown"));
#endif
	}

	if (Parameters == TEXT("DeleteMultiplyAsset"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::FScopedTestDataAsset TempAsset(DataAssetManagerTests::MakeUniqueAssetName(TEXT("TempDeleteAsset")));
		TestNotNull(TEXT("Temporary asset created"), TempAsset.GetAsset());

		TestTrue(TEXT("DeleteMultiplyAsset should delete asset"), DataAssetManager::DeleteMultiplyAsset({ TempAsset.GetAssetData() }));
#endif
	}

	if (Parameters == TEXT("CreateNewDataAsset"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::DeleteTestRootAssets();
		DataAssetManager::CreateNewDataAsset(UTestDataAsset::StaticClass(), DataAssetManagerTests::TestRootPath);

		const FAssetRegistryModule& RegistryModule{ FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry") };
		TArray<FAssetData> AssetsFound{};
		RegistryModule.Get().GetAssetsByPath(FName(*DataAssetManagerTests::TestRootPath), AssetsFound, true, false);

		const bool bExists = AssetsFound.Num() > 0;
		TestTrue(TEXT("CreateNewDataAsset should create an asset"), bExists);
		DataAssetManagerTests::DeleteTestRootAssets();
#endif
	}

	if (Parameters == TEXT("ProcessAssetData"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::FScopedTestDataAsset TempAsset(DataAssetManagerTests::MakeUniqueAssetName(TEXT("TempProcessAsset")));
		const FAssetData AssetData = TempAsset.GetAssetData();

		bool bCalled = false;
		DataAssetManager::ProcessAssetData({ AssetData },
			[&](const TArray<FAssetIdentifier>& Identifiers)
			{
				bCalled = true;
				TestTrue(TEXT("ProcessAssetData should return identifiers"), Identifiers.Num() > 0);
			});
		TestTrue(TEXT("ProcessAssetData callback executed"), bCalled);
#endif
	}

	if (Parameters == TEXT("BuildClipboardEntry"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::FScopedTestDataAsset TempAsset(DataAssetManagerTests::MakeUniqueAssetName(TEXT("TestClipboardAsset")));

		if (!TempAsset.GetAsset())
		{
			AddError(TEXT("Failed to create temporary asset"));
			return false;
		}

		FAssetData AssetData = TempAsset.GetAssetData();
		if (!AssetData.IsValid())
		{
			AddError(TEXT("Temp asset FAssetData invalid"));
			return false;
		}

		TempAsset.Save();
		AssetData = TempAsset.GetAssetData();

		{
			const FString Result = DataAssetManager::BuildClipboardEntry(AssetData, false);
			const FString Expected = AssetData.GetExportTextName();

			TestEqual(TEXT("BuildClipboardEntry returns export name when bCopyPaths=false"), Result, Expected);
		}

		{
			const FString Result = DataAssetManager::BuildClipboardEntry(AssetData, true);

			TestTrue(
				TEXT("BuildClipboardEntry should return an absolute path when file exists"),
				Result.EndsWith(TEXT(".uasset")) && FPaths::FileExists(Result));
		}
#endif
	}

	return true;
}

#endif
