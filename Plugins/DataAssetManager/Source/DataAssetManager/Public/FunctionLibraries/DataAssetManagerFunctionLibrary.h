// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetIdentifier.h"

struct FDetailsObjectSet;
class UDataAssetManagerSettings;

/**
 * @brief Free functions used by the Data Asset Manager editor module.
 */
namespace DataAssetManager
{



	/**
	 * @brief Returns the formatted disk size for an asset package.
	 * @param AssetData Asset data whose package file should be inspected.
	 * @return Human-readable size string, or "Unknown" when the file cannot be resolved.
	 */
	DATAASSETMANAGER_API FString GetAssetDiskSize(const FAssetData& AssetData);

	/**
	 * @brief Deletes several assets through the editor object tools.
	 * @param Assets Assets to delete.
	 * @param bShowConfirmation Whether to show the engine delete confirmation dialog.
	 * @return true when at least one asset was deleted.
	 */
	DATAASSETMANAGER_API bool DeleteMultiplyAsset(const TArray<FAssetData>& Assets, bool bShowConfirmation = false);

	/**
	 * @brief Returns the default Data Asset Manager plugin settings.
	 * @return Plugin settings default object.
	 */
	DATAASSETMANAGER_API const UDataAssetManagerSettings* GetPluginSettings();

	/**
	 * @brief Creates a new Data Asset instance of the supplied class in a directory.
	 * @param AssetClass Data Asset class to instantiate.
	 * @param Directory Long package path used as the creation target.
	 */
	DATAASSETMANAGER_API void CreateNewDataAsset(UClass* AssetClass, const FString& Directory);

	/**
	 * @brief Converts asset data into asset identifiers and forwards them to a callback.
	 * @param RefAssetData Asset data entries to convert.
	 * @param ProcessFunction Callback receiving resolved asset identifiers.
	 */
	DATAASSETMANAGER_API void ProcessAssetData(const TArray<FAssetData>& RefAssetData, TFunction<void(const TArray<FAssetIdentifier>&)> ProcessFunction);

	/**
	 * @brief Builds one clipboard entry for an asset.
	 * @param Item Asset data to format.
	 * @param bCopyPaths true to format a filesystem path, false to format an export reference.
	 * @return Clipboard entry for the asset.
	 */
	DATAASSETMANAGER_API FString BuildClipboardEntry(const FAssetData& Item, bool bCopyPaths);

	/**
	 * @brief Resets root objects to their class default object values.
	 * @param InRootObjectSet Details panel root object set to reset.
	 */
	DATAASSETMANAGER_API void ResetToCDO(const FDetailsObjectSet& InRootObjectSet);

	/**
	 * @brief Removes a delegate binding when its handle is valid and then resets the handle.
	 * @tparam TEvent Delegate/event type that exposes Remove(FDelegateHandle).
	 * @param DelegateHandle Delegate handle to remove and reset.
	 * @param Event Delegate/event from which the handle should be removed.
	 */
	template <typename TEvent>
	void RemoveDelegateHandleSafe(FDelegateHandle& DelegateHandle, TEvent&& Event)
	{
		if (DelegateHandle.IsValid())
		{
			Event.Remove(DelegateHandle);
			DelegateHandle.Reset();
		}
	}
} // namespace DataAssetManager
