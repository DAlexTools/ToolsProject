// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Models/DataAssetListModel.h"

#include "UObject/Package.h"

void FDataAssetListModel::SortByAssetName(TArray<TSharedPtr<FAssetData>>& InOutAssets)
{
	InOutAssets.Sort(
		[](const TSharedPtr<FAssetData>& A, const TSharedPtr<FAssetData>& B)
		{
			if (!A.IsValid())
			{
				return false;
			}

			if (!B.IsValid())
			{
				return true;
			}

			return A->AssetName.LexicalLess(B->AssetName);
		});
}

void FDataAssetListModel::ApplyFilters(
	const TArray<TSharedPtr<FAssetData>>& SourceAssets,
	const FString& SearchString,
	const TSet<FString>& ActiveAssetTypeFilters,
	const TSet<FString>& ActivePluginFilters,
	const TSet<FName>& InvalidAssetPackages,
	bool bModifiedOnly,
	bool bInvalidOnly,
	TArray<TSharedPtr<FAssetData>>& OutFilteredAssets)
{
	OutFilteredAssets.Reset(SourceAssets.Num());

	for (const TSharedPtr<FAssetData>& AssetData : SourceAssets)
	{
		if (!AssetData.IsValid())
		{
			continue;
		}

		if (MatchesFilters(*AssetData, SearchString, ActiveAssetTypeFilters, ActivePluginFilters, InvalidAssetPackages, bModifiedOnly, bInvalidOnly))
		{
			OutFilteredAssets.Add(AssetData);
		}
	}
}

bool FDataAssetListModel::MatchesFilters(
	const FAssetData& AssetData,
	const FString& SearchString,
	const TSet<FString>& ActiveAssetTypeFilters,
	const TSet<FString>& ActivePluginFilters,
	const TSet<FName>& InvalidAssetPackages,
	bool bModifiedOnly,
	bool bInvalidOnly)
{
	const FString AssetClassName = AssetData.AssetClassPath.GetAssetName().ToString();
	const FString PackagePath = AssetData.PackagePath.ToString();

	const bool bMatchesType = ActiveAssetTypeFilters.Num() == 0 || ActiveAssetTypeFilters.Contains(AssetClassName);
	const bool bNameMatches = SearchString.IsEmpty() || AssetData.AssetName.ToString().Contains(SearchString);
	const bool bMatchesModified = !bModifiedOnly || IsPackageDirty(AssetData);
	const bool bMatchesInvalid = !bInvalidOnly || InvalidAssetPackages.Contains(AssetData.PackageName);

	bool bMatchesPlugin = true;
	if (ActivePluginFilters.Num() > 0)
	{
		bMatchesPlugin = false;
		for (const FString& PluginMount : ActivePluginFilters)
		{
			if (PackagePath.StartsWith(PluginMount))
			{
				bMatchesPlugin = true;
				break;
			}
		}
	}

	return bMatchesType && bNameMatches && bMatchesPlugin && bMatchesModified && bMatchesInvalid;
}

bool FDataAssetListModel::IsPackageDirty(const FAssetData& AssetData)
{
	const UPackage* Package = FindPackage(nullptr, *AssetData.PackageName.ToString());
	return Package && Package->IsDirty();
}

TArray<TSharedPtr<FString>> FDataAssetListModel::BuildAssetTypeItems(const TArray<TSharedPtr<FAssetData>>& AssetDataList)
{
	TArray<TSharedPtr<FString>> Items;
	TSet<FString> UniqueAssetNames;

	for (const TSharedPtr<FAssetData>& AssetData : AssetDataList)
	{
		if (!AssetData.IsValid())
		{
			continue;
		}

		const FString AssetName = AssetData->AssetClassPath.GetAssetName().ToString();
		if (!UniqueAssetNames.Contains(AssetName))
		{
			UniqueAssetNames.Add(AssetName);
			Items.Add(MakeShared<FString>(AssetName));
		}
	}

	return Items;
}
