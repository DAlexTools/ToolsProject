// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "DataAssetManagerTypes.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Editor/ContentBrowser/Private/ContentBrowserSingleton.h"
#include "AssetToolsModule.h"
#include "AssetManagerEditorModule.h"
#include "JsonObjectConverter.h"


namespace DataAssetManager
{
	static const FName PathDevelopers{ TEXT("/Game/Developers") };
	static const FName PathRoot{ TEXT("/Game") };

	/**
	 * @brief Returns the developer path as a string.
	 *
	 * @return FString The developer path converted to a string.
	 */
	inline FString GetPathDevToString()
	{
		return PathDevelopers.ToString();
	}

	/**
	 * @brief Returns the root path as a string.
	 *
	 * @return FString The root path converted to a string.
	 */
	inline FString GetPathRootToString()
	{
		return PathRoot.ToString();
	}

	/**
	 * @brief Gets the disk size of the specified asset.
	 *
	 * @param AssetData The asset data to get the disk size for.
	 * @return FString A string representing the disk size of the asset.
	 */
	FString GetAssetDiskSize(const FAssetData& AssetData);

	/**
	 * @brief Deletes multiple assets from the content browser.
	 *
	 * @param Assets An array of FAssetData representing the assets to delete.
	 * @return true if all assets were successfully deleted, false otherwise.
	 */
	bool DeleteMultiplyAsset(const TArray<FAssetData>& Assets);

	/**
	 * Retrieves the plugin settings from the default instance of UDataAssetManagerSettings.
	 *
	 * @return A constant pointer to the default UDataAssetManagerSettings instance.
	 */
	const UDataAssetManagerSettings* GetPluginSettings();

	/**
	 * @brief Creates a new DataAsset of the specified class in the selected or provided directory.
	 * The function checks the validity of the class and ensures it is a subclass of UDataAsset.
	 * It then attempts to generate a unique name for the new asset by checking for existing assets
	 * with the same base name in the selected directory. The new asset is created using the AssetTools module
	 * and synchronized with the Content Browser to reflect the creation.
	 *
	 * @param AssetClass The class of the DataAsset to create. It must be a subclass of UDataAsset.
	 * @param Directory The directory where the new asset will be created. If no directory is selected,
	 *													the provided directory is used as the default.
	 */
	void CreateNewDataAsset(UClass* AssetClass, const FString& Directory);

	/**
	 * @brief Processes asset data by converting it to asset identifiers and executing a callback.
	 *
	 * @param RefAssetData Array of asset data to process
	 * @param ProcessFunction Callback function that receives processed asset identifiers
	 *
	 * @note Uses IAssetManagerEditorModule to extract identifiers from asset data
	 * @see IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList()
	 */
	void ProcessAssetData(const TArray<FAssetData>& RefAssetData, TFunction<void(const TArray<FAssetIdentifier>&)> ProcessFunction);

	/**
	 * @brief Normalizes the given path by standardizing slashes and removing redundant elements.
	 *
	 * @param InPath The input path string to normalize.
	 * @return FString The normalized path.
	 */
	FString PathNormalize(const FString& InPath);

	/**
	 * @brief Returns a tuple containing the normalized input path and the full project content directory path.
	 *
	 * @param InPath The input path string.
	 * @return TTuple<FString, FString> A tuple where first element is the normalized path,
	 *         second element is the project content directory path.
	 */
	TTuple<FString, FString> GetNormalizedAndProjectPath(const FString& InPath);

	FString PathConvert(const FString& InPath, bool bToAbsolute);

	/**
	 * @brief Converts a given path to an absolute path within the project context.
	 *
	 * @param InPath The input path to convert.
	 * @return FString The absolute path corresponding to the input.
	 */
	FString PathConvertToAbsolute(const FString& InPath);

	/**
	 * @brief Converts a given path to a project-relative path.
	 *
	 * @param InPath The input path to convert.
	 * @return FString The relative path corresponding to the input.
	 */
	FString PathConvertToRelative(const FString& InPath);

	/**
	 * @brief Checks whether the specified folder is empty (contains no files or subfolders).
	 *
	 * @param InPath The path of the folder to check.
	 * @return true If the folder is empty.
	 * @return false If the folder contains any files or subfolders.
	 */
	[[nodiscard]] bool FolderIsEmpty(const FString& InPath);

	/**
	 * @brief Returns the path to the folder containing external actors in the project.
	 *
	 * @return FString The external actors folder path.
	 */
	FString GetPathExternalActors();

	/**
	 * @brief Returns the path to the folder containing external objects in the project.
	 *
	 * @return FString The external objects folder path.
	 */
	FString GetPathExternalObjects();

	/**
	 * @brief Determines whether a folder is considered "external" (outside the standard project content paths).
	 *
	 * @param InPath The folder path to check.
	 * @return true If the folder is external.
	 * @return false If the folder is not external.
	 */
	[[nodiscard]] bool FolderIsExternal(const FString& InPath);




	bool SaveDataAssetToJsonFile(const UDataAsset* DataAsset, const FString& FilePath);


	bool LoadDataAssetFromJsonFile(UDataAsset* DataAsset, const FString& FilePath);

	/**
	 * @brief Safely removes a delegate binding and resets its handle.
	 *
	 * This helper function ensures that a delegate is unbound only if its handle is valid.
	 * It prevents invalid access or double-removal errors when cleaning up event listeners.
	 *
	 * @tparam TEvent The delegate or event type (e.g., FOnSomethingChanged).
	 * @param DelegateHandle Reference to the delegate handle that was previously bound.
	 * @param Event The event object from which the delegate should be removed.
	 *
	 * @note After successful removal, the handle is reset to an invalid state
	 *       to prevent accidental reuse.
	 *
	 * @example
	 * FDelegateHandle MyHandle = SomeEvent.AddLambda([](){ UE_LOG(LogTemp, Log, TEXT("Event fired!")); });
	 * RemoveDelegateHandleSafe(MyHandle, SomeEvent);
	 */
	template<typename TEvent>
	void RemoveDelegateHandleSafe(FDelegateHandle& DelegateHandle, TEvent&& Event)
	{
		if (DelegateHandle.IsValid())
		{
			Event.Remove(DelegateHandle);
			DelegateHandle.Reset();
		}
	}
}