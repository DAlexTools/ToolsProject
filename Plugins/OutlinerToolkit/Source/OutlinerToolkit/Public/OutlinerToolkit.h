// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FSpawnTabArgs;
class SDockTab;

class FOutlinerToolkitModule : public IModuleInterface
{
public:
	/** ~Begin IModuleInterface implementation	*/
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	/** ~End IModuleInterface implementation	*/

private:
	/**
	 * Spawns the audit log tab.
	 *
	 * @param SpawnTabArgs - Tab spawn parameters (position, parent window, etc.)
	 * @return Shared reference to the newly created dock tab widget
	 * 
	 * @see SOutlinerToolkitAuditPanel Widget
	 */
	TSharedRef<SDockTab> CreateAuditTab(const FSpawnTabArgs& SpawnTabArgs);
};
