// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Docking/TabManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"

class SNotepadWidget;
struct FToolMenuSection;

class UNOTEPAD_API IUNotepadModule : public IModuleInterface
{
public:
	virtual void OpenManagerTab() = 0;
	virtual void OpenFileInNotepad(const FString& FilePath) = 0;
};

class UNOTEPAD_API FUNotepadModule : public IUNotepadModule
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual void OpenManagerTab() override;
	virtual void OpenFileInNotepad(const FString& FilePath) override;

	static const FName UNotepadTabName;


private:
	[[nodiscard]] TSharedRef<SDockTab> CreateNotepadManagerTab(const FSpawnTabArgs& Args);
	void RegisterMenus();
	void AddContentBrowserOpenEntries(FToolMenuSection& Section);
	void OpenContentBrowserSelectionInNotepad(TArray<FString> FilePaths);
	TArray<FString> GetSupportedContentBrowserFilePaths(const FToolMenuSection& Section) const;
	[[nodiscard]] static bool IsSupportedSourceFilePath(const FString& FilePath);

	TWeakPtr<SNotepadWidget> ActiveNotepadWidget;
};
