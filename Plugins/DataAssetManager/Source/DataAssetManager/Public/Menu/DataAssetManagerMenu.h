// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDataAssetManagerInterface.h"

/**
 * @brief Builds the top-level Data Asset Manager menus.
 */
class FDataAssetManagerMenu final
{
public:
	/**
	 * @brief Populates the File menu section.
	 * @param MenuBuilder Menu builder receiving entries.
	 * @param ManagerInterface Manager implementation invoked by menu actions.
	 */
	static void FillFileMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface);

	/**
	 * @brief Populates the Asset menu section.
	 * @param MenuBuilder Menu builder receiving entries.
	 * @param ManagerInterface Manager implementation invoked by menu actions.
	 */
	static void FillAssetsMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface);

	/**
	 * @brief Populates the Settings menu section.
	 * @param MenuBuilder Menu builder receiving entries.
	 * @param ManagerInterface Manager implementation invoked by menu actions.
	 */
	static void FillSettingsMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface);

	/**
	 * @brief Populates the Help menu section.
	 * @param MenuBuilder Menu builder receiving entries.
	 * @param ManagerInterface Manager implementation invoked by menu actions.
	 */
	static void FillHelpMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface);
};

/**
 * @brief Factory for creating the complete Data Asset Manager menu bar widget.
 */
class FDataAssetManagerMenuFactory final
{
public:
	/**
	 * @brief Creates the menu bar bound to a manager interface implementation.
	 * @param ManagerInterface Manager implementation invoked by menu actions.
	 * @return Menu bar widget.
	 */
	[[nodiscard]] static TSharedRef<SWidget> CreateMenuBar(TSharedRef<IDataAssetManagerInterface> ManagerInterface);
};
