// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "GameProjectUtils.h"

/**
 * @brief Enumeration defining the domain type of a generated class.
 *
 * This enum specifies whether a generated C++ class is intended to be used
 * as a Blueprint base class or as a native C++-only class. This distinction
 * affects how the class is configured during template generation.
 */
enum class EClassDomain : uint8
{
	Blueprint, /**< @brief Class will be exposed to Blueprint system and can be extended in Blueprints. */
	Native	   /**< @brief Class is intended for C++ use only and won't be exposed to Blueprints. */
};

/**
 * @brief Main module class for the C++ Template Generator plugin.
 *
 * This class implements the IModuleInterface and serves as the entry point
 * for the C++ Template Generator plugin. It handles plugin lifecycle events,
 * registers menu extensions, and provides the core functionality for
 * generating C++ class templates from selected parent classes.
 *
 * @extends IModuleInterface
 *
 * @see UCppTemplateGeneratorSettings for plugin configuration options.
 */
class FCppTemplateGeneratorModule : public IModuleInterface
{
public:
	/**
	 * @brief IModuleInterface implementation
	 * @{
	 */

	/**
	 * @brief Initializes the plugin module.
	 *
	 * Called when the module is loaded by the Unreal Engine. This method sets up
	 * the plugin's resources, registers menu extensions, and prepares the
	 * template generation system for use.
	 *
	 * @note This is called early in the engine startup process.
	 *
	 * @see ShutdownModule
	 */
	virtual void StartupModule() override;

	/**
	 * @brief Shuts down the plugin module.
	 *
	 * Called when the module is being unloaded. This method cleans up any
	 * resources allocated by the plugin, unregisters menu extensions, and
	 * performs any necessary cleanup before the module is removed from memory.
	 *
	 * @note This is called during engine shutdown or hot reload.
	 *
	 * @see StartupModule
	 */
	virtual void ShutdownModule() override;

	/** @} */ // End of IModuleInterface implementation

	/**
	 * @brief Opens the template creation dialog for a specific parent class.
	 *
	 * This method initiates the template generation workflow for the specified
	 * parent class. It typically opens a dialog or wizard that guides the user
	 * through creating a new C++ class that inherits from the provided parent.
	 *
	 * @param[in] ParentClass The UClass to use as the parent/base for the new template.
	 *                        Must be a valid class from the TemplateClasses array
	 *                        defined in UCppTemplateGeneratorSettings.
	 *
	 *
	 * @see UCppTemplateGeneratorSettings::TemplateClasses
	 */
	void OpenCreateTemplateForClass(UClass* ParentClass);

	/**
	 * @brief Registers custom menu entries for the plugin.
	 *
	 * This method adds the plugin's menu items to the Unreal Editor's main menu,
	 * toolbar, or context menus. Typically adds entries under "File > New C++ Class"
	 * or similar locations to provide quick access to template generation features.
	 *
	 * @note Menu registration should occur during StartupModule.
	 *
	 * @see StartupModule
	 */
	void RegisterMenus();
};