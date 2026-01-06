// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDataAssetManagerInterface.h"

/**
 * Static menu builder class for Data Asset Manager UI.
 *
 * Provides methods to populate different menu sections with manager-specific actions.
 * All methods are static as they operate on the provided IDataAssetManagerInterface.
 */
class FDataAssetManagerMenu
{
public:
	/**
	 * Populates the File menu section with asset management operations.
	 *
	 * @param MenuBuilder Reference to UE's menu construction system
	 * @param Manager     Active data asset manager instance
	 */
	static void FillFileMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface);

	/**
	 * Populates the Assets menu section with asset-specific actions.
	 *
	 * @param MenuBuilder Reference to UE's menu construction system
	 * @param Manager     Active data asset manager instance
	 */
	static void FillAssetsMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface);

	/**
	 * Populates the Settings menu section with configuration options.
	 *
	 * @param MenuBuilder Reference to UE's menu construction system
	 * @param Manager     Active data asset manager instance
	 */
	static void FillSettingsMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface);

	/**
	 * Populates the Help menu section with documentation and support actions.
	 *
	 * @param MenuBuilder Reference to UE's menu construction system
	 * @param Manager     Active data asset manager instance
	 */
	static void FillHelpMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface);

};

/**
 * Factory class for creating the Data Asset Manager menu bar widget.
 *
 * Handles the construction of the complete menu bar structure
 * and connects it to manager functionality.
 */
class FDataAssetManagerMenuFactory
{
public:
	/**
	 * Creates and returns a fully configured menu bar widget.
	 *
	 * @param Manager Active data asset manager instance
	 * @return Shared reference to the constructed menu bar widget
	 */
	static TSharedRef<SWidget> CreateMenuBar(TSharedRef<IDataAssetManagerInterface> ManagerInterface);
};