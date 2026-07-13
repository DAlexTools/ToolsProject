// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDeveloperSettingsWidget;
class SDataAssetManagerWidget;
class UStatusBarSubsystem;

/**
 * @brief Public module interface for opening and recreating the Data Asset Manager editor UI.
 */
class IDataAssetManagerModule : public IModuleInterface
{
public:
	/**
	 * @brief Opens the Data Asset Manager tab or focuses it if it already exists.
	 */
	virtual void OpenManagerTab() = 0;

	/**
	 * @brief Recreates the Data Asset Manager widget and restores the manager tab.
	 */
	virtual void RestartWidget() = 0;
};

/**
 * @brief Editor module implementation that registers menus, tabs, and Data Asset Manager widgets.
 */
class FDataAssetManagerModule : public IDataAssetManagerModule
{
public:
	/**
	 * @brief Initializes module startup integrations.
	 */
	virtual void StartupModule() override;

	/**
	 * @brief Unregisters module integrations and clears pending restart work.
	 */
	virtual void ShutdownModule() override;

	/**
	 * @brief Registers the editor menu entry used to open the manager tab.
	 */
	void RegisterMenus();

	/**
	 * @brief Clears the delayed restart timer when it is active.
	 */
	void ClearAllTimerIfNeeded();

	/**
	 * @brief Opens the Data Asset Manager tab or focuses an existing live tab.
	 */
	virtual void OpenManagerTab() override;

	/**
	 * @brief Closes and schedules reopening of the manager tab.
	 */
	virtual void RestartWidget() override;

	/**
	 * @brief Returns the editor status bar subsystem used by the manager tab.
	 * @return Status bar subsystem, or nullptr when the editor subsystem is unavailable.
	 */
	UStatusBarSubsystem* GetStatusBarSubsystem() const;

	/**
	 * @brief Creates status bar content for a manager dock tab.
	 * @param Tab Dock tab that owns the status bar.
	 * @return Slate widget used as status bar content.
	 */
	TSharedRef<SWidget> CreateStatusBarWidget(const TSharedRef<SDockTab>& Tab) const;

private:
	/** @brief Timer used to defer tab reopening during plugin restart. */
	FTimerHandle RestartTimerHandle;

	/**
	 * @brief Creates the dock tab that hosts the Data Asset Manager widget.
	 * @param Args Tab spawn arguments supplied by the global tab manager.
	 * @return Newly configured Data Asset Manager dock tab.
	 */
	TSharedRef<SDockTab> CreateDataAssetManagerTab(const FSpawnTabArgs& Args);

	/** @brief Detail customization widget for plugin settings. */
	TSharedPtr<SDeveloperSettingsWidget> DeveloperSettingsWidget;

	/** @brief Main Slate widget used to manage Data Assets. */
	TSharedPtr<SDataAssetManagerWidget> DataAssetWidget;

	/** @brief UI command list used by plugin menu and toolbar actions. */
	TSharedPtr<FUICommandList> DAManagerPluginCommands;
};
