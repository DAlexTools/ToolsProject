// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"
#include "Tests/DataAssetManagerTestHelpers.h"
#include "Utils/DataAssetManagerPathUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "IDetailRootObjectCustomization.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "UObject/SavePackage.h"

#if !UE_BUILD_SHIPPING && WITH_DEV_AUTOMATION_TESTS

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

	OutBeautifiedNames.Add(TEXT("InvalidInputs"));
	OutTestCommands.Add(TEXT("InvalidInputs"));

	OutBeautifiedNames.Add(TEXT("PathUtils.EdgeCases"));
	OutTestCommands.Add(TEXT("PathUtilsEdgeCases"));

	OutBeautifiedNames.Add(TEXT("LegacyPathFunctions"));
	OutTestCommands.Add(TEXT("LegacyPathFunctions"));

	OutBeautifiedNames.Add(TEXT("JsonRoundTrip"));
	OutTestCommands.Add(TEXT("JsonRoundTrip"));

	OutBeautifiedNames.Add(TEXT("ResetToCDO"));
	OutTestCommands.Add(TEXT("ResetToCDO"));

	OutBeautifiedNames.Add(TEXT("RemoveDelegateHandleSafe"));
	OutTestCommands.Add(TEXT("RemoveDelegateHandleSafe"));
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
		DataAssetManager::CreateNewDataAsset(UTestDataAsset::StaticClass(), DataAssetManagerTests::TestRootPath);

		const FAssetRegistryModule& RegistryModule{ FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry") };
		TArray<FAssetData> AssetsFound{};
		RegistryModule.Get().GetAssetsByPath(FName(*DataAssetManagerTests::TestRootPath), AssetsFound, true, false);

		TestEqual(TEXT("CreateNewDataAsset should create unique assets on repeated calls"), AssetsFound.Num(), 2);
		TestTrue(
			TEXT("First generated asset should use the base name"),
			AssetsFound.ContainsByPredicate(
				[](const FAssetData& AssetData)
				{
					return AssetData.AssetName == FName(TEXT("NewDataAsset"));
				}));
		TestTrue(
			TEXT("Second generated asset should use a numeric suffix"),
			AssetsFound.ContainsByPredicate(
				[](const FAssetData& AssetData)
				{
					return AssetData.AssetName == FName(TEXT("NewDataAsset_1"));
				}));
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
		TestTrue(TEXT("Invalid asset data returns empty clipboard entry"), DataAssetManager::BuildClipboardEntry(FAssetData(), false).IsEmpty());
		TestTrue(TEXT("Invalid asset data returns empty path entry"), DataAssetManager::BuildClipboardEntry(FAssetData(), true).IsEmpty());

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

	if (Parameters == TEXT("InvalidInputs"))
	{
		TestEqual(TEXT("Invalid asset disk size should be Unknown"), DataAssetManager::GetAssetDiskSize(FAssetData()), FString(TEXT("Unknown")));
		TestFalse(TEXT("Deleting an empty asset list should fail"), DataAssetManager::DeleteMultiplyAsset({}));

#if WITH_EDITOR
		DataAssetManagerTests::DeleteTestRootAssets();
		DataAssetManager::CreateNewDataAsset(nullptr, DataAssetManagerTests::TestRootPath);
		DataAssetManager::CreateNewDataAsset(UObject::StaticClass(), DataAssetManagerTests::TestRootPath);

		TArray<FAssetData> AssetsFound;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry")
			.Get()
			.GetAssetsByPath(FName(*DataAssetManagerTests::TestRootPath), AssetsFound, true, false);
		TestEqual(TEXT("Invalid DataAsset creation requests should not create assets"), AssetsFound.Num(), 0);

		bool bProcessCallbackCalled = false;
		DataAssetManager::ProcessAssetData({},
			[&bProcessCallbackCalled, this](const TArray<FAssetIdentifier>& Identifiers)
			{
				bProcessCallbackCalled = true;
				TestEqual(TEXT("Empty asset data list resolves to zero identifiers"), Identifiers.Num(), 0);
			});
		TestTrue(TEXT("ProcessAssetData should call the callback for empty input"), bProcessCallbackCalled);
		DataAssetManagerTests::DeleteTestRootAssets();
#endif
	}

	if (Parameters == TEXT("PathUtilsEdgeCases"))
	{
		const FString ProjectContentPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir().LeftChop(1));

		TestTrue(TEXT("Normalize rejects empty input"), FDataAssetManagerPathUtils::Normalize(TEXT("")).IsEmpty());
		TestTrue(TEXT("Normalize rejects relative input"), FDataAssetManagerPathUtils::Normalize(TEXT("Relative/Folder")).IsEmpty());

		const FString NormalizedGamePath = FDataAssetManagerPathUtils::Normalize(TEXT("/Game//Folder/../Folder2/"));
		TestFalse(TEXT("Normalize removes trailing slash"), NormalizedGamePath.EndsWith(TEXT("/")));
		TestFalse(TEXT("Normalize removes duplicate slashes"), NormalizedGamePath.Contains(TEXT("//")));

		TestEqual(TEXT("ConvertToAbsolute maps /Game to content root"), FDataAssetManagerPathUtils::ConvertToAbsolute(TEXT("/Game")), ProjectContentPath);
		TestEqual(TEXT("ConvertToRelative maps content root to /Game"), FDataAssetManagerPathUtils::ConvertToRelative(ProjectContentPath), FString(TEXT("/Game")));
		TestTrue(TEXT("ConvertToAbsolute rejects relative input"), FDataAssetManagerPathUtils::ConvertToAbsolute(TEXT("Relative/Folder")).IsEmpty());
		TestTrue(TEXT("ConvertToRelative rejects paths outside project content"), FDataAssetManagerPathUtils::ConvertToRelative(TEXT("C:/DataAssetManagerOutside")).IsEmpty());

		const TTuple<FString, FString> Paths = FDataAssetManagerPathUtils::GetNormalizedAndProjectPath(TEXT("/Game/Test"));
		TestFalse(TEXT("GetNormalizedAndProjectPath should normalize supported paths"), Paths.Get<0>().IsEmpty());
		TestEqual(TEXT("GetNormalizedAndProjectPath should return content root"), Paths.Get<1>(), ProjectContentPath);

		TestFalse(TEXT("IsFolderEmpty rejects empty input"), FDataAssetManagerPathUtils::IsFolderEmpty(TEXT("")));
		TestFalse(TEXT("IsFolderEmpty rejects relative input"), FDataAssetManagerPathUtils::IsFolderEmpty(TEXT("Relative/Folder")));

		const FString FolderDiskPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) / TEXT("TempEmptyAbsoluteTest");
		IFileManager::Get().MakeDirectory(*FolderDiskPath, true);
		TestTrue(TEXT("IsFolderEmpty accepts an empty absolute content path"), FDataAssetManagerPathUtils::IsFolderEmpty(FolderDiskPath));

		const FString TempFile = FolderDiskPath / TEXT("TestFile.tmp");
		FFileHelper::SaveStringToFile(TEXT("Dummy"), *TempFile);
		TestFalse(TEXT("IsFolderEmpty detects files through absolute content paths"), FDataAssetManagerPathUtils::IsFolderEmpty(FolderDiskPath));

		IFileManager::Get().Delete(*TempFile);
		IFileManager::Get().DeleteDirectory(*FolderDiskPath, false, true);
	}

	if (Parameters == TEXT("LegacyPathFunctions"))
	{
		const FString ProjectContentPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()).LeftChop(1);

		TestTrue(TEXT("PathNormalize rejects relative input"), DataAssetManager::PathNormalize(TEXT("Relative/Folder")).IsEmpty());
		TestEqual(TEXT("PathConvertToAbsolute maps /Game to content root"), DataAssetManager::PathConvertToAbsolute(TEXT("/Game")), ProjectContentPath);
		TestEqual(TEXT("PathConvertToRelative maps content root to /Game"), DataAssetManager::PathConvertToRelative(ProjectContentPath), FString(TEXT("/Game")));
		TestEqual(TEXT("GetPathExternalActors delegates to package path"), DataAssetManager::GetPathExternalActors(), FDataAssetManagerPathUtils::GetExternalActorsPath());
		TestEqual(TEXT("GetPathExternalObjects delegates to package path"), DataAssetManager::GetPathExternalObjects(), FDataAssetManagerPathUtils::GetExternalObjectsPath());
		TestTrue(TEXT("FolderIsExternal detects external actors path"), DataAssetManager::FolderIsExternal(DataAssetManager::GetPathExternalActors()));
		TestFalse(TEXT("FolderIsExternal rejects regular game path"), DataAssetManager::FolderIsExternal(TEXT("/Game/Regular")));
		TestFalse(TEXT("FolderIsEmpty rejects empty input"), DataAssetManager::FolderIsEmpty(TEXT("")));
	}

	if (Parameters == TEXT("JsonRoundTrip"))
	{
		UTestDataAsset* Asset = NewObject<UTestDataAsset>(GetTransientPackage(), TEXT("JsonRoundTripAsset"));
		TestNotNull(TEXT("Transient test DataAsset should be created"), Asset);

		const FString JsonDirectory = FPaths::ProjectSavedDir() / TEXT("Automation/DataAssetManager");
		IFileManager::Get().MakeDirectory(*JsonDirectory, true);
		const FString JsonFilePath = JsonDirectory / TEXT("JsonRoundTrip.json");
		const FString BadJsonFilePath = JsonDirectory / TEXT("BadJson.json");
		IFileManager::Get().Delete(*JsonFilePath);
		IFileManager::Get().Delete(*BadJsonFilePath);

		TestFalse(TEXT("SaveDataAssetToJsonFile rejects null assets"), DataAssetManager::SaveDataAssetToJsonFile(nullptr, JsonFilePath));
		TestFalse(TEXT("LoadDataAssetFromJsonFile rejects null assets"), DataAssetManager::LoadDataAssetFromJsonFile(nullptr, JsonFilePath));
		TestFalse(TEXT("LoadDataAssetFromJsonFile rejects missing files"), DataAssetManager::LoadDataAssetFromJsonFile(Asset, JsonFilePath));

		Asset->TestProperty = 42;
		TestTrue(TEXT("SaveDataAssetToJsonFile writes valid JSON"), DataAssetManager::SaveDataAssetToJsonFile(Asset, JsonFilePath));
		TestTrue(TEXT("JSON file should exist after save"), FPaths::FileExists(JsonFilePath));

		Asset->TestProperty = 7;
		TestTrue(TEXT("LoadDataAssetFromJsonFile applies serialized values"), DataAssetManager::LoadDataAssetFromJsonFile(Asset, JsonFilePath));
		TestEqual(TEXT("LoadDataAssetFromJsonFile should restore TestProperty"), Asset->TestProperty, 42);

		FFileHelper::SaveStringToFile(TEXT("{ not valid json"), *BadJsonFilePath);
		TestFalse(TEXT("LoadDataAssetFromJsonFile rejects malformed JSON"), DataAssetManager::LoadDataAssetFromJsonFile(Asset, BadJsonFilePath));

		IFileManager::Get().Delete(*JsonFilePath);
		IFileManager::Get().Delete(*BadJsonFilePath);
	}

	if (Parameters == TEXT("ResetToCDO"))
	{
		UTestDataAsset* Asset = NewObject<UTestDataAsset>(GetTransientPackage(), TEXT("ResetToCDOAsset"));
		TestNotNull(TEXT("Transient test DataAsset should be created"), Asset);

		Asset->TestProperty = 123;

		FDetailsObjectSet RootObjectSet;
		RootObjectSet.RootObjects.Add(Asset);
		RootObjectSet.CommonBaseClass = UTestDataAsset::StaticClass();
		DataAssetManager::ResetToCDO(RootObjectSet);

		TestEqual(TEXT("ResetToCDO restores the default object value"), Asset->TestProperty, GetDefault<UTestDataAsset>()->TestProperty);

		Asset->TestProperty = 456;
		FDetailsObjectSet EmptyRootObjectSet;
		EmptyRootObjectSet.CommonBaseClass = UTestDataAsset::StaticClass();
		DataAssetManager::ResetToCDO(EmptyRootObjectSet);
		TestEqual(TEXT("ResetToCDO ignores empty object sets"), Asset->TestProperty, 456);
	}

	if (Parameters == TEXT("RemoveDelegateHandleSafe"))
	{
		FSimpleMulticastDelegate Delegate;
		int32 Calls = 0;
		FDelegateHandle Handle = Delegate.AddLambda(
			[&Calls]()
			{
				++Calls;
			});

		TestTrue(TEXT("Delegate handle should start valid"), Handle.IsValid());
		DataAssetManager::RemoveDelegateHandleSafe(Handle, Delegate);
		TestFalse(TEXT("RemoveDelegateHandleSafe resets removed handles"), Handle.IsValid());

		Delegate.Broadcast();
		TestEqual(TEXT("Removed delegate should not be called"), Calls, 0);

		FDelegateHandle InvalidHandle;
		DataAssetManager::RemoveDelegateHandleSafe(InvalidHandle, Delegate);
		TestFalse(TEXT("Invalid delegate handle stays invalid"), InvalidHandle.IsValid());
	}

	return true;
}

#endif
