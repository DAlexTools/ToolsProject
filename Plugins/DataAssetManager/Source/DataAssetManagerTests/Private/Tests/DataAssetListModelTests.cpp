// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Models/DataAssetListModel.h"
#include "Tests/DataAssetManagerTestTypes.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDataAssetListModelFiltersByNameTypeAndPluginTest,
	"DataAssetManager.Data.AssetListModel.FiltersByNameTypeAndPlugin",
	DataAssetManagerFlags::Flags)

bool FDataAssetListModelFiltersByNameTypeAndPluginTest::RunTest(const FString& Parameters)
{
	const FTopLevelAssetPath DataAssetClassPath(TEXT("/Script/Engine"), TEXT("DataAsset"));
	const FTopLevelAssetPath TextureClassPath(TEXT("/Script/Engine"), TEXT("Texture2D"));

	TArray<TSharedPtr<FAssetData>> SourceAssets;
	SourceAssets.Add(MakeShared<FAssetData>(
		FName(TEXT("/Game/DataAssetManagerTests/AlphaAsset")),
		FName(TEXT("/Game/DataAssetManagerTests")),
		FName(TEXT("AlphaAsset")),
		DataAssetClassPath,
		FAssetDataTagMap(),
		TArrayView<const int32>(),
		0));
	SourceAssets.Add(MakeShared<FAssetData>(
		FName(TEXT("/PluginA/Data/BetaAsset")),
		FName(TEXT("/PluginA/Data")),
		FName(TEXT("BetaAsset")),
		DataAssetClassPath,
		FAssetDataTagMap(),
		TArrayView<const int32>(),
		0));
	SourceAssets.Add(MakeShared<FAssetData>(
		FName(TEXT("/Game/DataAssetManagerTests/PreviewTexture")),
		FName(TEXT("/Game/DataAssetManagerTests")),
		FName(TEXT("PreviewTexture")),
		TextureClassPath,
		FAssetDataTagMap(),
		TArrayView<const int32>(),
		0));

	const TSet<FName> EmptyInvalidPackages;
	TArray<TSharedPtr<FAssetData>> FilteredAssets;
	FDataAssetListModel::ApplyFilters(SourceAssets, TEXT("Asset"), { TEXT("DataAsset") }, {}, EmptyInvalidPackages, false, false, FilteredAssets);
	TestEqual(TEXT("Name and type filters should keep two DataAsset entries"), FilteredAssets.Num(), 2);

	FDataAssetListModel::ApplyFilters(SourceAssets, TEXT("Asset"), { TEXT("DataAsset") }, { TEXT("/PluginA") }, EmptyInvalidPackages, false, false, FilteredAssets);
	TestEqual(TEXT("Plugin filter should keep only the plugin asset"), FilteredAssets.Num(), 1);
	TestEqual(TEXT("Filtered asset should be BetaAsset"), FilteredAssets[0]->AssetName, FName(TEXT("BetaAsset")));

	TSet<FName> InvalidPackages;
	InvalidPackages.Add(FName(TEXT("/PluginA/Data/BetaAsset")));
	FDataAssetListModel::ApplyFilters(SourceAssets, TEXT(""), {}, {}, InvalidPackages, false, true, FilteredAssets);
	TestEqual(TEXT("Invalid-only filter should keep only cached invalid assets"), FilteredAssets.Num(), 1);
	TestEqual(TEXT("Invalid-only filter should keep BetaAsset"), FilteredAssets[0]->AssetName, FName(TEXT("BetaAsset")));

	UPackage* DirtyPackage = CreatePackage(TEXT("/Game/DataAssetManagerTests/DirtyModelAsset"));
	DirtyPackage->SetDirtyFlag(true);
	SourceAssets.Add(MakeShared<FAssetData>(
		FName(TEXT("/Game/DataAssetManagerTests/DirtyModelAsset")),
		FName(TEXT("/Game/DataAssetManagerTests")),
		FName(TEXT("DirtyModelAsset")),
		DataAssetClassPath,
		FAssetDataTagMap(),
		TArrayView<const int32>(),
		0));

	FDataAssetListModel::ApplyFilters(SourceAssets, TEXT(""), {}, {}, EmptyInvalidPackages, true, false, FilteredAssets);
	TestEqual(TEXT("Modified-only filter should keep only dirty packages"), FilteredAssets.Num(), 1);
	TestEqual(TEXT("Modified-only filter should keep the dirty asset"), FilteredAssets[0]->AssetName, FName(TEXT("DirtyModelAsset")));
	DirtyPackage->SetDirtyFlag(false);

	const TArray<TSharedPtr<FString>> TypeItems = FDataAssetListModel::BuildAssetTypeItems(SourceAssets);
	TestEqual(TEXT("Type item list should contain unique class names"), TypeItems.Num(), 2);

	return true;
}

#endif
