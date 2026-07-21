// Copyright 2025 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SWindow;

class ITextureChannelPackerModule : public IModuleInterface
{
public:
    virtual void OpenPluginWindow() = 0;
};

class FTextureChannelPackerModule : public ITextureChannelPackerModule
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    virtual void OpenPluginWindow() override;

private:
    TWeakPtr<SWindow> ToolWindow;
};
