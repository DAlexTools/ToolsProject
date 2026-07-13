// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Constants shared across the Data Asset Manager module.
 */
namespace DataAssetManager
{
	/** @brief Delay used before reopening the manager tab after a restart request. */
	constexpr float TabReopenDelaySeconds = 1.0f;

	/** @brief Fallback text used when an asset value cannot be resolved. */
	const FString UnknownStr = TEXT("Unknown");

	/** @brief Tool menu owner name used by the editor integration. */
	const FName ToolProjectEditor(TEXT("ToolProjectEditor"));

	/** @brief Status bar identifier for the Data Asset Manager tab. */
	const FName StatusBarName(TEXT("DataAssetManagerStatusBar"));

	/** @brief Tab identifier registered with the global tab manager. */
	const FName DataAssetManagerTabName(TEXT("DataAssetManager"));

	/**
	 * @brief Unreal module names loaded by Data Asset Manager features.
	 */
	namespace ModuleName
	{
		/** @brief Asset tools module name. */
		constexpr const TCHAR* AssetTools = TEXT("AssetTools");

		/** @brief Asset registry module name. */
		constexpr const TCHAR* AssetRegistry = TEXT("AssetRegistry");

		/** @brief Content Browser module name. */
		constexpr const TCHAR* ContentBrowser = TEXT("ContentBrowser");

		/** @brief Message Log module name. */
		constexpr const TCHAR* MessageLog = TEXT("MessageLog");

		/** @brief Property Editor module name. */
		constexpr const TCHAR* PropertyEditor = TEXT("PropertyEditor");

		/** @brief Output Log module name. */
		constexpr const TCHAR* OutputLog = TEXT("OutputLog");

		/** @brief Settings module name. */
		constexpr const TCHAR* Settings = TEXT("Settings");

		/** @brief Data Asset Manager module name. */
		constexpr const TCHAR* DataAssetManager = TEXT("DataAssetManager");
	} // namespace ModuleName
} // namespace DataAssetManager
