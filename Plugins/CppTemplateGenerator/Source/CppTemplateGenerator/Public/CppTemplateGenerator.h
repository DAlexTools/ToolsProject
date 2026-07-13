// Copyright 2025 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @brief Main module class for the C++ Template Generator plugin.
 *
 * Registers the editor menu integration for creating C++ classes from a
 * curated set of template parent classes and provides navigation to the
 * plugin settings.
 *
 * @see UCppTemplateGeneratorSettings
 */
class FCppTemplateGeneratorModule : public IModuleInterface
{
public:
	/**
	 * @brief Registers the plugin menu extension during module startup.
	 */
	virtual void StartupModule() override;

	/**
	 * @brief Unregisters menu-related callbacks during module shutdown.
	 */
	virtual void ShutdownModule() override;

	/**
	 * @brief Opens the standard Add Code dialog for a validated template class.
	 *
	 * @param ParentClass Native Actor or ActorComponent-derived class selected
	 * from the configured template list.
	 */
	void OpenCreateTemplateForClass(UClass* ParentClass);

	/**
	 * @brief Opens this plugin's settings page in the editor settings viewer.
	 */
	void OpenPluginSettings() const;

	/**
	 * @brief Registers the main Tools submenu and its entries.
	 */
	void RegisterMenus();

private:
	/**
	 * @brief Validates whether a class can be used as a template parent.
	 *
	 * Only native Actor and ActorComponent hierarchies are supported by the
	 * Add Code flow. Deprecated and replaced classes are rejected.
	 *
	 * @param TemplateClass Class to validate.
	 * @return true if the class can be shown in the submenu and passed to the
	 * Add Code dialog; otherwise false.
	 */
	bool IsValidTemplateClass(const UClass* TemplateClass) const;
};
