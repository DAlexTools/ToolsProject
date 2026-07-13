// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetIdentifier.h"

struct FDetailsObjectSet;
class FProperty;
class UDataAsset;
class UDataAssetManagerSettings;

/**
 * @brief Free functions used by the Data Asset Manager editor module.
 */
namespace DataAssetManager
{
	/**
	 * @brief Compares all array elements of a property in two UObject containers.
	 * @param Property Reflected property to compare.
	 * @param LeftObject Left object container.
	 * @param RightObject Right object container.
	 * @return true when every property value is identical.
	 */
	DATAASSETMANAGER_API bool ArePropertyValuesIdentical(const FProperty* Property, const UObject* LeftObject, const UObject* RightObject);

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

	/**
	 * @brief Normalizes an absolute filesystem path or long package path.
	 * @param InPath Path to normalize.
	 * @return Normalized path, or an empty string when the input is not supported.
	 */
	DATAASSETMANAGER_API FString PathNormalize(const FString& InPath);

	/**
	 * @brief Normalizes an input path and resolves the absolute project content path.
	 * @param InPath Path to normalize.
	 * @return Tuple containing the normalized input path and absolute project content path.
	 */
	DATAASSETMANAGER_API TTuple<FString, FString> GetNormalizedAndProjectPath(const FString& InPath);

	/**
	 * @brief Converts a path between long package form and absolute project content form.
	 * @param InPath Path to convert.
	 * @param bToAbsolute true to convert to absolute filesystem path, false to convert to long package path.
	 * @return Converted path, or an empty string when conversion is not possible.
	 */
	DATAASSETMANAGER_API FString PathConvert(const FString& InPath, bool bToAbsolute);

	/**
	 * @brief Converts a long package path to an absolute project content path.
	 * @param InPath Path to convert.
	 * @return Absolute filesystem path, or an empty string when conversion fails.
	 */
	DATAASSETMANAGER_API FString PathConvertToAbsolute(const FString& InPath);

	/**
	 * @brief Converts an absolute project content path to a long package path.
	 * @param InPath Path to convert.
	 * @return Long package path, or an empty string when conversion fails.
	 */
	DATAASSETMANAGER_API FString PathConvertToRelative(const FString& InPath);

	/**
	 * @brief Checks whether a content folder contains no assets and no files on disk.
	 * @param InPath Long package path or absolute folder path to inspect.
	 * @return true when the folder can be resolved and contains no assets or files.
	 */
	[[nodiscard]] DATAASSETMANAGER_API bool FolderIsEmpty(const FString& InPath);

	/**
	 * @brief Returns the external actors folder path.
	 * @return Long package path for external actors.
	 */
	DATAASSETMANAGER_API FString GetPathExternalActors();

	/**
	 * @brief Returns the external objects folder path.
	 * @return Long package path for external objects.
	 */
	DATAASSETMANAGER_API FString GetPathExternalObjects();

	/**
	 * @brief Checks whether a path points into an external actors or external objects folder.
	 * @param InPath Long package path to test.
	 * @return true when the path belongs to an external object folder.
	 */
	[[nodiscard]] DATAASSETMANAGER_API bool FolderIsExternal(const FString& InPath);

	/**
	 * @brief Serializes a Data Asset to a JSON file.
	 * @param DataAsset Data Asset to serialize.
	 * @param FilePath Destination JSON file path.
	 * @return true when the asset was serialized and saved.
	 */
	DATAASSETMANAGER_API bool SaveDataAssetToJsonFile(const UDataAsset* DataAsset, const FString& FilePath);

	/**
	 * @brief Deserializes a Data Asset from a JSON file.
	 * @param DataAsset Data Asset instance to populate.
	 * @param FilePath Source JSON file path.
	 * @return true when the asset was loaded and deserialized.
	 */
	DATAASSETMANAGER_API bool LoadDataAssetFromJsonFile(UDataAsset* DataAsset, const FString& FilePath);
} // namespace DataAssetManager
