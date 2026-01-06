// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "GameProjectUtils.h"

enum class EClassDomain : uint8
{
	Blueprint,
	Native
};
class FCppTemplateGeneratorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OpenCreateTemplateForClass(UClass* ParentClass);

	void RegisterMenus();
};
