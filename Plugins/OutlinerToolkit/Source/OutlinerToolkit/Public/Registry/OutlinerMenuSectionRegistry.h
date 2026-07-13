// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

/**
 * Handles registration of custom Outliner Toolkit menu sections
 * in the Unreal Engine Level Editor context menu.
 *
 * Responsible for creating and organizing all plugin-specific
 * actor utility menus and actions.
 */
class OUTLINERTOOLKIT_API FOutlinerMenuSectionRegistry final
{
public:
	/**
	 * Registers all Outliner Toolkit menus
	 * in the Level Editor context menu.
	 */
	static void RegisterMenus();

	/**
	 * Opens plugin settings in the editor.
	 */
	static void OpenPluginSettings();

private:
	/**
	 * Creates the main Outliner Toolkit submenu.
	 *
	 * @param SubMenu Parent menu being populated.
	 */
	static void CreateOutlinerToolkitMenuSection(UToolMenu* SubMenu);
	
	/**
	 * Creates the actor grouping tools section.
	 *
	 * Includes actor grouping and ungrouping actions.
	 *
	 * @param GroupSubMenu Menu section being populated.
	 */
	static void CreateOutlinerToolkitGroupingSection(UToolMenu* GroupSubMenu);
	
	/**
	 * Creates the actor settings copy/paste section.
	 *
	 * Includes tools for copying and applying actor settings.
	 *
	 * @param CopyMenu Menu section being populated.
	 */
	static void CreateOutlinerToolkitCopySettingsSection(UToolMenu* CopyMenu);

	/**
	 * Creates the actor modification tools section.
	 *
	 * Includes rename operations such as
	 * prefix and suffix modifications.
	 *
	 * @param ModifySectionMenu Menu section being populated.
	 */
	static void CreateOutlinerToolkitModifyActorsSection(UToolMenu* ModifySectionMenu);
	
	/**
	 * Creates the bulk script actions section.
	 *
	 * Includes utility actions for batch modification
	 * of selected actor properties.
	 *
	 * @param ScriptSectionMenu Menu section being populated.
	 */
	static void CreateOutlinerToolkitScriptSection(UToolMenu* ScriptSectionMenu);
};
