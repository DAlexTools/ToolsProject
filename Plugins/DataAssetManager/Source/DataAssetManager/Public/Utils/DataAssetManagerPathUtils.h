// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Utility functions for converting and validating Data Asset Manager content paths.
 */
class DATAASSETMANAGER_API FDataAssetManagerPathUtils final
{
public:
	/**
	 * @brief Returns the standard developers folder path.
	 * 
	 * @return Long package path for the developers folder.
	 */
	static FString GetDevelopersPath();

	/**
	 * @brief Returns the project content root path.
	 * 
	 * @return Long package path for the project content root.
	 */
	static FString GetRootPath();

	/**
	 * @brief Normalizes an input path and resolves the absolute project content path.
	 * 
	 * @param InPath Path to normalize.
	 * @return Tuple containing the normalized input path and absolute project content path.
	 */
	static TTuple<FString, FString> GetNormalizedAndProjectPath(const FString& InPath);

	/**
	 * @brief Normalizes an absolute filesystem path or long package path.
	 * 
	 * @param InPath Path to normalize.
	 * @return Normalized path, or an empty string when the input is not supported.
	 */
	static FString Normalize(const FString& InPath);

	/**
	 * @brief Converts a path between project-relative long package form and absolute filesystem form.
	 * 
	 * @param InPath Path to convert.
	 * @param bToAbsolute true to convert to absolute filesystem path, false to convert to long package path.
	 * @return Converted path, or an empty string when conversion is not possible.
	 */
	static FString Convert(const FString& InPath, bool bToAbsolute);

	/**
	 * @brief Converts a long package path to an absolute project content path.
	 * 
	 * @param InPath Path to convert.
	 * @return Absolute filesystem path, or an empty string when conversion fails.
	 */
	static FString ConvertToAbsolute(const FString& InPath);

	/**
	 * @brief Converts an absolute project content path to a long package path.
	 * 
	 * @param InPath Path to convert.
	 * @return Long package path, or an empty string when conversion fails.
	 */
	static FString ConvertToRelative(const FString& InPath);

	/**
	 * @brief Returns the external actors folder path.
	 * 
	 * @return Long package path for external actors.
	 */
	static FString GetExternalActorsPath();

	/**
	 * @brief Returns the external objects folder path.
	 * 
	 * @return Long package path for external objects.
	 */
	static FString GetExternalObjectsPath();

	/**
	 * @brief Checks whether a path points into an external actors or external objects folder.
	 * 
	 * @param InPath Long package path to test.
	 * @return true when the path belongs to an external object folder.
	 */
	static bool IsExternalFolder(const FString& InPath);

	/**
	 * @brief Checks whether a content folder contains no assets and no files on disk.
	 * 
	 * @param InPath Long package path or absolute folder path to inspect.
	 * @return true when the folder can be resolved and contains no assets or files.
	 */
	static bool IsFolderEmpty(const FString& InPath);
};
