// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SDockTab;
class FSpawnTabArgs;

class FPerformanceInspectorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<SDockTab> CreatePerformanceInspectorTab(const FSpawnTabArgs& SpawnTabArgs);
	void OnOpenSettings();
private:
	void HandleBeginPIE(bool bIsSimulating);
	void HandleEndPIE(bool bIsSimulating);
	void AddPIToolBarButton(FToolBarBuilder& Builder);
	void OnOpenTool();

	TSharedPtr<FUICommandList> PluginCommands;

	FDelegateHandle BeginPIEHandle;
	FDelegateHandle EndPIEHandle;
};
