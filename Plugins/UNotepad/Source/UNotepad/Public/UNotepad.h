// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"

class SNotepadWidget;
struct FToolMenuSection;

/**
 * @brief Public interface for the UNotepad module.
 *
 * Provides access to the main Notepad functionality from other modules.
 */
class UNOTEPAD_API IUNotepadModule : public IModuleInterface
{
public:
	/**
	 * @brief Opens the Notepad manager tab.
	 */
	virtual void OpenManagerTab() = 0;

	/**
	 * @brief Opens the specified file in the Notepad editor.
	 *
	 * @param FilePath Absolute path to the file.
	 */
	virtual void OpenFileInNotepad(const FString& FilePath) = 0;
};

/**
 * @brief Main implementation of the UNotepad module.
 *
 * Handles module initialization, editor integration, menu registration,
 * Content Browser extensions, and creation of the Notepad editor tab.
 */
class UNOTEPAD_API FUNotepadModule : public IUNotepadModule
{
public:
	/** Initializes the module and registers editor extensions. */
	virtual void StartupModule() override;

	/** Shuts down the module and unregisters editor extensions. */
	virtual void ShutdownModule() override;

	/** Opens the Notepad manager tab. */
	virtual void OpenManagerTab() override;

	/** Opens the specified file in the Notepad editor. */
	virtual void OpenFileInNotepad(const FString& FilePath) override;

	/** Name of the Notepad dock tab. */
	static const FName UNotepadTabName;

private:
	/**
	 * @brief Determines whether the specified file is supported by the editor.
	 *
	 * @param FilePath Absolute file path.
	 *
	 * @return True if the file extension is supported.
	 */
	[[nodiscard]] static bool IsSupportedSourceFilePath(const FString& FilePath);

	/**
	 * @brief Creates the main Notepad dock tab.
	 *
	 * @param Args Tab spawn arguments.
	 *
	 * @return Newly created dock tab.
	 */
	[[nodiscard]] TSharedRef<SDockTab> CreateNotepadManagerTab(const FSpawnTabArgs& Args);

	/** Registers editor menus and menu extensions. */
	void RegisterMenus();

	/**
	 * @brief Adds "Open in Notepad" entries to the Content Browser context menu.
	 *
	 * @param Section Menu section to extend.
	 */
	void AddContentBrowserOpenEntries(FToolMenuSection& Section);

	/**
	 * @brief Opens the specified files in the Notepad editor.
	 *
	 * @param FilePaths List of file paths to open.
	 */
	void OpenContentBrowserSelectionInNotepad(TArray<FString> FilePaths);

	/**
	 * @brief Returns supported file paths from the current Content Browser selection.
	 *
	 * @param Section Content Browser menu section.
	 *
	 * @return List of supported file paths.
	 */
	TArray<FString> GetSupportedContentBrowserFilePaths(const FToolMenuSection& Section) const;

	/** Weak reference to the active Notepad widget. */
	TWeakPtr<SNotepadWidget> ActiveNotepadWidget;
};
