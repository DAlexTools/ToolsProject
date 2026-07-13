// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/**
 * @brief Stateless helpers for sorting and filtering Data Asset list data.
 */
class DATAASSETMANAGER_API FDataAssetListModel final
{
public:
	/**
	 * @brief Sorts asset data pointers by asset name.
	 * @param InOutAssets Asset list to sort in place.
	 */
	static void SortByAssetName(TArray<TSharedPtr<FAssetData>>& InOutAssets);

	/**
	 * @brief Applies search, type, plugin, dirty, and validation filters to an asset list.
	 * @param SourceAssets Source asset list.
	 * @param SearchString Name search string.
	 * @param ActiveAssetTypeFilters Active asset type names.
	 * @param ActivePluginFilters Active plugin mount paths.
	 * @param InvalidAssetPackages Package names reported as invalid by validation.
	 * @param bModifiedOnly true to include dirty packages only.
	 * @param bInvalidOnly true to include invalid packages only.
	 * @param OutFilteredAssets Receives the filtered asset list.
	 */
	static void ApplyFilters(
		const TArray<TSharedPtr<FAssetData>>& SourceAssets,
		const FString& SearchString,
		const TSet<FString>& ActiveAssetTypeFilters,
		const TSet<FString>& ActivePluginFilters,
		const TSet<FName>& InvalidAssetPackages,
		bool bModifiedOnly,
		bool bInvalidOnly,
		TArray<TSharedPtr<FAssetData>>& OutFilteredAssets);

	/**
	 * @brief Checks whether one asset passes the active filter set.
	 * @param AssetData Asset data to evaluate.
	 * @param SearchString Name search string.
	 * @param ActiveAssetTypeFilters Active asset type names.
	 * @param ActivePluginFilters Active plugin mount paths.
	 * @param InvalidAssetPackages Package names reported as invalid by validation.
	 * @param bModifiedOnly true to require a dirty package.
	 * @param bInvalidOnly true to require an invalid package.
	 * @return true when the asset passes every active filter.
	 */
	static bool MatchesFilters(
		const FAssetData& AssetData,
		const FString& SearchString,
		const TSet<FString>& ActiveAssetTypeFilters,
		const TSet<FString>& ActivePluginFilters,
		const TSet<FName>& InvalidAssetPackages,
		bool bModifiedOnly,
		bool bInvalidOnly);

	/**
	 * @brief Checks whether an asset package is currently dirty in memory.
	 * @param AssetData Asset whose package should be checked.
	 * @return true when the package is loaded and dirty.
	 */
	static bool IsPackageDirty(const FAssetData& AssetData);

	/**
	 * @brief Builds unique asset type filter items from an asset list.
	 * @param AssetDataList Source asset list.
	 * @return Unique asset type names suitable for combo box items.
	 */
	static TArray<TSharedPtr<FString>> BuildAssetTypeItems(const TArray<TSharedPtr<FAssetData>>& AssetDataList);
};
