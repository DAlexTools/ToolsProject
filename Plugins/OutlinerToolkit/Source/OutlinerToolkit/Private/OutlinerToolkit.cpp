// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "OutlinerToolkit.h"
#include "Columns/OutlinerColumnUtils.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Registry/OutlinerColumnRegistry.h"
#include "Registry/OutlinerFilterRegistry.h"
#include "Registry/OutlinerMenuSectionRegistry.h"
#include "Audit/Widgets/SOutlinerToolkitAuditPanel.h"

#define LOCTEXT_NAMESPACE "FOutlinerToolkitModule"

void FOutlinerToolkitModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FOutlinerMenuSectionRegistry::RegisterMenus));
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(OutlinerToolkitAuditTabId, FOnSpawnTab::CreateRaw(this, &FOutlinerToolkitModule::CreateAuditTab))
		.SetDisplayName(LOCTEXT("OutlinerToolkitAuditTabTitle", "Outliner Toolkit Audit"))
		.SetTooltipText(LOCTEXT("OutlinerToolkitAuditTabTooltip", "Open the Outliner Toolkit scene audit panel."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Warning"));

	FOutlinerColumnRegistry::RegisterColumns();
	FOutlinerFilterRegistry::RegisterOutlinerFilters();
}

TSharedRef<SDockTab> FOutlinerToolkitModule::CreateAuditTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return	SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SOutlinerToolkitAuditPanel)
		];
}

void FOutlinerToolkitModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(OutlinerToolkitAuditTabId);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOutlinerToolkitModule, OutlinerToolkit)
