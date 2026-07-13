// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetIdentifier.h"
#include "Types/DataAssetDiffTypes.h"
#include "Types/DataAssetReferenceTypes.h"
#include "Types/DataAssetValidationTypes.h"

class UDataAssetManagerSettings;

/**
 * @brief Service layer for loading, saving, moving, deleting, validating, and formatting Data Assets.
 */
class DATAASSETMANAGER_API FDataAssetManagerAssetService final
{
public:
	/**
	 * @brief Loads all configured Data Assets and available project plugin filter items.
	 * @param PluginSettings Settings that define scan directories and excluded Data Asset classes.
	 * @param OutDataAssets Receives loaded asset data pointers sorted by asset name.
	 * @param OutPluginFilterItems Receives project plugin mount paths available as filters.
	 */
	static void LoadDataAssets( const UDataAssetManagerSettings* PluginSettings, TArray<TSharedPtr<FAssetData>>& OutDataAssets, TArray<TSharedPtr<FString>>& OutPluginFilterItems);

	/**
	 * @brief Saves one Data Asset package.
	 * @param AssetData Asset data identifying the asset to save.
	 * @return true when the package was saved successfully.
	 */
	static bool SaveAsset(const TSharedPtr<FAssetData>& AssetData);

	/**
	 * @brief Saves every valid Data Asset in the provided list.
	 * @param AssetDataList Assets to save.
	 * @return Number of assets saved successfully.
	 */
	static int32 SaveAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList);

	/**
	 * @brief Renames one Data Asset through Asset Tools.
	 * @param AssetData Asset to rename.
	 * @param NewName New asset name.
	 * @return true when the asset was renamed.
	 */
	static bool RenameAsset(const TSharedPtr<FAssetData>& AssetData, const FString& NewName);

	/**
	 * @brief Duplicates Data Assets in place using unique copy names.
	 * @param AssetDataList Assets to duplicate.
	 * @param OutDuplicatedAssets Optional list of duplicated asset data.
	 * @return Number of assets duplicated.
	 */
	static int32 DuplicateAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList, TArray<FAssetData>* OutDuplicatedAssets = nullptr);

	/**
	 * @brief Moves Data Assets to another long package path using unique names when needed.
	 * @param AssetDataList Assets to move.
	 * @param DestinationPath Destination long package path.
	 * @param OutMovedAssets Optional list of moved asset data.
	 * @return Number of assets moved.
	 */
	static int32 MoveAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList, const FString& DestinationPath, TArray<FAssetData>* OutMovedAssets = nullptr);

	/**
	 * @brief Deletes the provided Data Assets.
	 * @param AssetDataList Assets to delete.
	 * @param bShowConfirmation Whether to show the engine delete confirmation dialog.
	 * @return true when the delete operation removed at least one asset.
	 */
	static bool DeleteAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList, bool bShowConfirmation = false);

	/**
	 * @brief Saves all dirty map and content packages without prompting the user.
	 * @return true when the save-all operation succeeds.
	 */
	static bool SaveAllDataAssets();

	/**
	 * @brief Runs data validation for the supplied Data Assets.
	 * @param AssetDataList Assets to validate.
	 * @param bOpenMessageLog Whether to open and write validation results to the Message Log.
	 * @return Validation states keyed by package name, plus package names that failed validation or failed to load.
	 */
	static FDataAssetValidationResults ValidateAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList, bool bOpenMessageLog = true);

	/**
	 * @brief Collects package dependencies and referencers for one Data Asset.
	 * @param AssetData Asset to inspect.
	 * @return References, referencers, and unresolved dependency package names.
	 */
	static FDataAssetReferenceInspectionResult InspectReferences(const TSharedPtr<FAssetData>& AssetData);

	/**
	 * @brief Compares editable top-level properties for two Data Assets of the same class.
	 * @param LeftAssetData Left side asset.
	 * @param RightAssetData Right side asset.
	 * @return Diff entries and comparability state.
	 */
	static FDataAssetDiffResult DiffAssets( const TSharedPtr<FAssetData>& LeftAssetData, const TSharedPtr<FAssetData>& RightAssetData);

	/**
	 * @brief Copies one editable property value from one Data Asset to another using reflection import/export.
	 * @param SourceAssetData Source asset containing the value to copy.
	 * @param TargetAssetData Target asset that receives the value.
	 * @param PropertyName Property to copy.
	 * @param OutErrorText Optional error text when copy fails.
	 * @return true when the value was copied and the target package was marked dirty.
	 */
	static bool CopyDiffPropertyValue(
		const TSharedPtr<FAssetData>& SourceAssetData,
		const TSharedPtr<FAssetData>& TargetAssetData,
		FName PropertyName,
		FText* OutErrorText = nullptr);

	/**
	 * @brief Converts valid shared asset data pointers into value asset data.
	 * @param AssetDataList Source shared pointer list.
	 * @return Value array containing only valid asset data entries.
	 */
	static TArray<FAssetData> ToAssetDataArray(const TArray<TSharedPtr<FAssetData>>& AssetDataList);

	/**
	 * @brief Builds newline-separated clipboard text for an asset list.
	 * @param AssetDataList Assets to format.
	 * @param bCopyPaths true to copy filesystem paths, false to copy export references.
	 * @return Clipboard-ready text.
	 */
	static FString BuildClipboardText(const TArray<TSharedPtr<FAssetData>>& AssetDataList, bool bCopyPaths);

	/**
	 * @brief Converts asset data into asset identifiers and forwards them to a caller callback.
	 * @param AssetDataList Assets to process.
	 * @param ProcessFunction Callback receiving resolved asset identifiers.
	 */
	static void ProcessAssetData(const TArray<FAssetData>& AssetDataList, TFunction<void(const TArray<FAssetIdentifier>&)> ProcessFunction);
};
