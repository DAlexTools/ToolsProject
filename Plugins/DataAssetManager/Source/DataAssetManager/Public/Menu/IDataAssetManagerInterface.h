// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Command interface implemented by widgets that can service Data Asset Manager menu actions.
 */
class IDataAssetManagerInterface
{
public:
	/** @brief Destroys the interface instance. */
	virtual ~IDataAssetManagerInterface() = default;

	/** @brief Starts creation of a new Data Asset. */
	virtual void CreateNewDataAsset() = 0;

	/** @brief Opens the selected Data Asset in its editor. */
	virtual void OpenSelectedDataAssetInEditor() = 0;

	/** @brief Toggles visibility of the asset list panel. */
	virtual void ToggleDataAssetListVisibility() = 0;

	/** @brief Opens plugin documentation from settings. */
	virtual void ShowDocumentation() = 0;

	/** @brief Saves selected Data Assets. */
	virtual void SaveDataAsset() = 0;

	/** @brief Saves all dirty Data Asset packages. */
	virtual void SaveAllData() = 0;

	/** @brief Syncs the Content Browser selection to the selected assets. */
	virtual void SyncContentBrowserToSelectedAsset() = 0;

	/**
	 * @brief Copies selected asset references or paths to the clipboard.
	 * @param bCopyPaths true to copy paths, false to copy object references.
	 */
	virtual void CopyToClipboard(bool bCopyPaths) = 0;

	/** @brief Opens Reference Viewer for selected assets. */
	virtual void OpenReferenceViewer() = 0;

	/** @brief Opens the Data Asset reference inspector for the selected asset. */
	virtual void OpenReferenceInspector() = 0;

	/** @brief Opens the Data Asset diff window for two selected assets. */
	virtual void OpenDataAssetDiff() = 0;

	/**
	 * @brief Checks whether the current selection can be diffed.
	 * @return true when exactly two assets are selected.
	 */
	virtual bool CanOpenDataAssetDiff() const = 0;

	/** @brief Opens Size Map for selected assets. */
	virtual void OpenSizeMap() = 0;

	/** @brief Opens Asset Audit for selected assets. */
	virtual void OpenAuditAsset() = 0;

	/** @brief Opens plugin settings in the editor settings UI. */
	virtual void OpenPluginSettings() = 0;

	/** @brief Opens the source control dialog for selected assets. */
	virtual void ShowSourceControlDialog() = 0;

	/** @brief Focuses inline rename editing for the selected asset. */
	virtual void FocusOnSelectedAsset() = 0;

	/** @brief Deletes selected Data Assets. */
	virtual void DeleteDataAsset() = 0;

	/** @brief Restarts the plugin UI. */
	virtual void RestartPlugin() = 0;

	/** @brief Opens the Message Log window. */
	virtual void OpenMessageLogWindow() = 0;

	/** @brief Opens the Output Log window. */
	virtual void OpenOutputLogWindow() = 0;

	/**
	 * @brief Checks whether the current selection can be renamed.
	 * @return true when inline rename is allowed.
	 */
	virtual bool CanRename() const = 0;

	/** @brief Shows metadata for selected assets. */
	virtual void ShowAssetMetaData() = 0;
};
