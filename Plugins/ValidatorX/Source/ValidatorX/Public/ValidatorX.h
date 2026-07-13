// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IValidatorXModule : public IModuleInterface
{
public:
	virtual void OpenManagerTab() = 0;
};

class FValidatorXModule : public IValidatorXModule
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static const FName ValidatorXTabName;
protected:
	void HandlePostEngineInit();

	TSharedRef<SDockTab> OnSpawnValidatorXTab(const FSpawnTabArgs& Args);

	virtual void OpenManagerTab() override;
	void RegisterMenus();

private:
	FDelegateHandle PostEngineInitHandle;
};
