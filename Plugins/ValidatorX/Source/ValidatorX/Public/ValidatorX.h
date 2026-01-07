// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UBlueprintValidatorBase;

/**
 * @brief Module interface for the ValidatorX validation plugin.
 *
 * This abstract interface defines the core API for the ValidatorX validation system.
 * It serves as the contract that all ValidatorX module implementations must fulfill,
 * enabling different validation strategies while maintaining a consistent interface.
 *
 * @extends IModuleInterface
 */
class IValidatorXModule : public IModuleInterface
{
public:
	/**
	 * @brief Opens the main ValidatorX management tab in the editor.
	 *
	 * This method is responsible for spawning or bringing to focus the primary
	 * ValidatorX user interface tab where users can configure validators,
	 * run validation checks, and view validation results.
	 *
	 * @note Must be implemented by concrete module implementations.
	 */
	virtual void OpenManagerTab() = 0;
};

/**
 * @brief Main implementation module for the ValidatorX validation plugin.
 *
 * This class provides the concrete implementation of the ValidatorX validation system.
 * It handles plugin lifecycle events, manages the validation UI, and coordinates
 * the execution of registered validators. The module integrates with the Unreal Editor
 * to provide validation capabilities for various asset types including Blueprints,
 * DataAssets, and project settings.
 *
 * @extends IValidatorXModule
 */
class FValidatorXModule : public IValidatorXModule
{
public:
	/** IModuleInterface implementation */

	/**
	 * @brief Initializes the ValidatorX module during editor startup.
	 *
	 * This method registers the module's UI components, sets up validation rules,
	 * and prepares the validator system for use. It's called automatically when
	 * the plugin is loaded by the Unreal Editor.
	 *
	 * @see ShutdownModule
	 */
	virtual void StartupModule() override;

	/**
	 * @brief Cleans up the ValidatorX module during editor shutdown.
	 *
	 * This method unregisters UI components, saves configuration, and performs
	 * necessary cleanup before the module is unloaded. It ensures proper resource
	 * management and prevents memory leaks.
	 *
	 * @see StartupModule
	 */
	virtual void ShutdownModule() override;

	/** @brief Unique identifier for the ValidatorX editor tab. */
	static const FName ValidatorXTabName;

protected:
	/**
	 * @brief Handles initialization after the engine is fully loaded.
	 *
	 * This callback is invoked once the Unreal Engine has completed its startup
	 * sequence. It's used to perform initialization tasks that require the full
	 * engine to be available, such as accessing certain subsystems or loading
	 * configuration data.
	 */
	void HandlePostEngineInit();

	/**
	 * @brief Determines where the ValidatorX tab should appear in the editor UI.
	 *
	 * @return The menu type specifying where the ValidatorX tab should be spawned
	 *         (e.g., in the main menu, tools menu, or as a standalone window).
	 */
	ETabSpawnerMenuType::Type GetVisibleModule() const;

	/**
	 * @brief Creates and initializes the ValidatorX management tab widget.
	 *
	 * This method constructs the SDockTab that serves as the main UI for the
	 * ValidatorX plugin, including all validator controls and results displays.
	 *
	 * @param Args The spawn arguments provided by the tab manager.
	 * @return A shared reference to the newly created dock tab.
	 */
	TSharedRef<SDockTab> OnSpawnValidatorXTab(const FSpawnTabArgs& Args);

	/**
	 * @brief Opens or brings to focus the ValidatorX management tab.
	 *
	 * Implements the IValidatorXModule interface. This method ensures the
	 * ValidatorX UI tab is visible to the user, either by spawning a new tab
	 * or activating an existing one.
	 *
	 * @see IValidatorXModule::OpenManagerTab
	 */
	virtual void OpenManagerTab() override;

	/**
	 * @brief Registers ValidatorX menu entries in the editor UI.
	 *
	 * This method adds ValidatorX options to appropriate editor menus,
	 * typically under "Tools" or "Window" menus, allowing users to access
	 * validation functionality through the standard editor interface.
	 */
	void RegisterMenus();

	/**
	 * @brief Collection of registered validators available in the system.
	 *
	 * This array holds all validator instances that have been registered with
	 * the ValidatorX system. Each validator checks for specific types of issues
	 * in project assets, such as missing references, unused variables, or
	 * incorrect configuration settings.
	 *
	 * @note Validators are typically loaded from configuration or discovered
	 *       automatically based on project needs.
	 */
	TArray<TSharedPtr<UBlueprintValidatorBase>> Validators;
};