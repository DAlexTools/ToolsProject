#pragma once

#include "FunctionLibraries/DataAssetManagerFunctionLibrary.h"

class UDataAsset;

/**
 * Compatibility header for the old singular FunctionLibrary include path.
 * New code should include FunctionLibraries/DataAssetManagerFunctionLibrary.h.
 */
namespace DataAssetManager
{
	inline const FName PathDevelopers{ TEXT("/Game/Developers") };
	inline const FName PathRoot{ TEXT("/Game") };

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
	 * @brief Normalizes the given path by standardizing slashes and removing redundant elements.
	 *
	 * @param InPath The input path string to normalize.
	 * @return FString The normalized path.
	 */
	DATAASSETMANAGER_API FString PathNormalize(const FString& InPath);

	/**
	 * @brief Returns a tuple containing the normalized input path and the full project content directory path.
	 *
	 * @param InPath The input path string.
	 * @return TTuple<FString, FString> A tuple where first element is the normalized path,
	 *         second element is the project content directory path.
	 */
	DATAASSETMANAGER_API TTuple<FString, FString> GetNormalizedAndProjectPath(const FString& InPath);

	/**
	 * @brief Converts a path between project-relative and absolute project content formats.
	 *
	 * If @p bToAbsolute is true, converts a path relative to the configured root
	 * (e.g. "/Textures") into an absolute project content path
	 * (e.g. "/Game/Textures").
	 *
	 * If @p bToAbsolute is false, performs the reverse conversion from a project
	 * content path to the configured root-relative format.
	 *
	 * If the input path cannot be converted or is invalid, an empty string is returned.
	 *
	 * @param InPath Path to convert.
	 * @param bToAbsolute True to convert to an absolute project content path,
	 *                    false to convert to a root-relative path.
	 *
	 * @return Converted path, or an empty string if conversion failed.
	 */
	DATAASSETMANAGER_API FString PathConvert(const FString& InPath, bool bToAbsolute);

	/**
	 * @brief Converts a given path to an absolute path within the project context.
	 *
	 * @param InPath The input path to convert.
	 * @return FString The absolute path corresponding to the input.
	 */
	DATAASSETMANAGER_API FString PathConvertToAbsolute(const FString& InPath);

	/**
	 * @brief Converts a given path to a project-relative path.
	 *
	 * @param InPath The input path to convert.
	 * @return FString The relative path corresponding to the input.
	 */
	DATAASSETMANAGER_API FString PathConvertToRelative(const FString& InPath);

	/**
	 * @brief Checks whether the specified folder is empty (contains no files or subfolders).
	 *
	 * @param InPath The path of the folder to check.
	 * @return true If the folder is empty.
	 * @return false If the folder contains any files or subfolders.
	 */
	[[nodiscard]] DATAASSETMANAGER_API bool FolderIsEmpty(const FString& InPath);

	/**
	 * @brief Returns the path to the folder containing external actors in the project.
	 *
	 * @return FString The external actors folder path.
	 */
	DATAASSETMANAGER_API FString GetPathExternalActors();

	/**
	 * @brief Returns the path to the folder containing external objects in the project.
	 *
	 * @return FString The external objects folder path.
	 */
	DATAASSETMANAGER_API FString GetPathExternalObjects();

	/**
	 * @brief Determines whether a folder is considered "external" (outside the standard project content paths).
	 *
	 * @param InPath The folder path to check.
	 * @return true If the folder is external.
	 * @return false If the folder is not external.
	 */
	[[nodiscard]] DATAASSETMANAGER_API bool FolderIsExternal(const FString& InPath);

	/**
	 * @brief Serializes a Data Asset to a JSON file.
	 *
	 * Converts the Data Asset into its JSON representation using Unreal Engine's
	 * JSON serialization system and writes the result to the specified file.
	 *
	 * @param DataAsset Data Asset to serialize.
	 * @param FilePath Destination JSON file path.
	 *
	 * @return True if the asset was successfully serialized and saved; otherwise false.
	 */
	DATAASSETMANAGER_API bool SaveDataAssetToJsonFile(const UDataAsset* DataAsset, const FString& FilePath);
	
	/**
	 * @brief Deserializes a Data Asset from a JSON file.
	 *
	 * Loads a JSON file, deserializes its contents into the specified Data Asset,
	 * and applies the changes. After successful deserialization, the asset is
	 * marked as modified and PostEditChange() is called to notify the editor.
	 *
	 * @param DataAsset Data Asset instance to populate.
	 * @param FilePath Source JSON file path.
	 *
	 * @return True if the asset was successfully loaded and deserialized; otherwise false.
	 */
	DATAASSETMANAGER_API bool LoadDataAssetFromJsonFile(UDataAsset* DataAsset, const FString& FilePath);
}
