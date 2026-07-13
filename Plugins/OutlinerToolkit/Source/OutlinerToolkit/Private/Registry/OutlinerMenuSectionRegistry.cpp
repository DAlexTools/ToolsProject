// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Registry/OutlinerMenuSectionRegistry.h"
#include "Columns/OutlinerColumnUtils.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "Settings/OutlinerToolkitSettings.h"
#include "ToolMenu.h"

#define LOCTEXT_NAMESPACE "FOutlinerMenuSectionRegistry"

void FOutlinerMenuSectionRegistry::RegisterMenus()
{
	{
		UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		FToolMenuSection& ToolsSection = ToolsMenu->FindOrAddSection("OutlinerToolkit", LOCTEXT("OutlinerToolkitToolsSection", "Outliner Toolkit"));
		ToolsSection.AddMenuEntry(
			"OpenOutlinerToolkitAudit",
			LOCTEXT("OpenOutlinerToolkitAuditLabel", "Outliner Toolkit Audit"),
			LOCTEXT("OpenOutlinerToolkitAuditTooltip", "Open the Outliner Toolkit scene audit panel."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Warning"),
			FUIAction(FExecuteAction::CreateLambda([]() 
				{
					FGlobalTabmanager::Get()->TryInvokeTab(OutlinerToolkitAuditTabId);
				})));
		ToolsSection.AddMenuEntry(
			"OpenOutlinerToolkitSettings",
			LOCTEXT("OpenOutlinerToolkitSettingsLabel", "Outliner Toolkit Settings"),
			LOCTEXT("OpenOutlinerToolkitSettingsTooltip", "Open Outliner Toolkit settings in Project Settings."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings"),
			FUIAction(FExecuteAction::CreateStatic(&FOutlinerMenuSectionRegistry::OpenPluginSettings)));
	}

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu");
		FToolMenuSection& Section = Menu->FindOrAddSection("Section", LOCTEXT("OutlinerSectionLabel", "Outliner Section"));
		Section.AddSubMenu(
			"OutlinerToolkit",
			LOCTEXT("OutlinerToolkitLabel", "Outliner Toolkit"),
			LOCTEXT("OutlinerToolkitTooltip", "Custom tools for selected actors"),
			FNewToolMenuDelegate::CreateStatic(&FOutlinerMenuSectionRegistry::CreateOutlinerToolkitMenuSection),
			false, FSlateIcon(FName("AutomationWindowStyle"), "Automation.InProcess"));
	}
}

void FOutlinerMenuSectionRegistry::OpenPluginSettings()
{
	const UOutlinerToolkitSettings* Settings = GetDefault<UOutlinerToolkitSettings>();
	if (!Settings)
	{
		return;
	}

	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(
		Settings->GetContainerName(),
		Settings->GetCategoryName(),
		Settings->GetSectionName());
}

void FOutlinerMenuSectionRegistry::CreateOutlinerToolkitMenuSection(UToolMenu* SubMenu)
{
	FToolMenuSection& SubSection = SubMenu->AddSection("OutlinerSection");
	SubSection.AddSubMenu(
		"Grouping",
		LOCTEXT("GroupingSectionLabel", "Grouping Section"),
		LOCTEXT("GroupingSectionTooltip", "Grouping Section Tools"),
		FNewToolMenuDelegate::CreateStatic(&FOutlinerMenuSectionRegistry::CreateOutlinerToolkitGroupingSection),
		false, FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.GroupActors"));

	SubSection.AddSubMenu(
		"CopySettings",
		LOCTEXT("CopySettingsLabel", "Copy Settings"),
		LOCTEXT("CopySettingsTooltip", "Copy settings from the first selected actor"),
		FNewToolMenuDelegate::CreateStatic(&FOutlinerMenuSectionRegistry::CreateOutlinerToolkitCopySettingsSection),
		false, FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"));

	SubSection.AddSubMenu(
		"ModifyActors",
		LOCTEXT("ModifyActorsLabel", "Modify Actors"),
		LOCTEXT("ModifyActorsTooltip", "Rename and paste tools"),
		FNewToolMenuDelegate::CreateStatic(&FOutlinerMenuSectionRegistry::CreateOutlinerToolkitModifyActorsSection),
		false, FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"));

	SubSection.AddSeparator("OutlinerToolkitBulkActions");

	SubSection.AddSubMenu(
		"Scripts",
		LOCTEXT("ScriptsLabel", "Scripts "),
		LOCTEXT("ScriptsTooltip", "Other Scripts "),
		FNewToolMenuDelegate::CreateStatic(&FOutlinerMenuSectionRegistry::CreateOutlinerToolkitScriptSection),
		false, FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"));
}

void FOutlinerMenuSectionRegistry::CreateOutlinerToolkitGroupingSection(UToolMenu* GroupSubMenu)
{
	FToolMenuSection& GroupingSection = GroupSubMenu->AddSection("GroupingTools");
	GroupingSection.AddMenuEntry(
		"GroupActors",
		LOCTEXT("GroupActorsLabel", "Group Actors"),
		LOCTEXT("GroupActorsTooltip", "Group selected actors and move them into a new folder."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.GroupActors"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::GroupSelectedActors)));

	GroupingSection.AddMenuEntry(
		"UngroupActors",
		LOCTEXT("UngroupActorsLabel", "Ungroup Actors"),
		LOCTEXT("UngroupActorsTooltip", "Ungroup selected group actors and keep members selected."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.GroupActors"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::UngroupSelectedActors)));
}

void FOutlinerMenuSectionRegistry::CreateOutlinerToolkitCopySettingsSection(UToolMenu* CopyMenu)
{
	FToolMenuSection& CopySection = CopyMenu->AddSection("CopySection");

	CopySection.AddMenuEntry(
		"CopyCommonSettings",
		LOCTEXT("CopyCommonSettingsLabel", "Common"),
		LOCTEXT("CopyCommonSettingsTooltip", "Copy Tick, HiddenInGame and Mobility"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::CopyCommonActorSettings)));

	CopySection.AddMenuEntry(
		"CopyRenderingSettings",
		LOCTEXT("CopyRenderingSettingsLabel", "Rendering"),
		LOCTEXT("CopyRenderingSettingsTooltip", "Copy shadow and CustomDepth settings"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::CopyRenderingActorSettings)));

	CopySection.AddMenuEntry(
		"CopyCollisionSettings",
		LOCTEXT("CopyCollisionSettingsLabel", "Collision"),
		LOCTEXT("CopyCollisionSettingsTooltip", "Copy overlap settings"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::CopyCollisionActorSettings)));

	CopySection.AddMenuEntry(
		"CopyPhysicsSettings",
		LOCTEXT("CopyPhysicsSettingsLabel", "Physics"),
		LOCTEXT("CopyPhysicsSettingsTooltip", "Copy physics simulation"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::CopyPhysicsActorSettings)));

	CopySection.AddMenuEntry(
		"CopyTagsSettings",
		LOCTEXT("CopyTagsSettingsLabel", "Tags"),
		LOCTEXT("CopyTagsSettingsTooltip", "Copy actor tags"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::CopyTagsActorSettings)));

	CopySection.AddMenuEntry(
		"CopyFolderSettings",
		LOCTEXT("CopyFolderSettingsLabel", "Folder"),
		LOCTEXT("CopyFolderSettingsTooltip", "Copy actor folder"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::CopyFolderActorSettings)));

	CopySection.AddMenuEntry(
		"PasteSettings",
		LOCTEXT("PasteSettingsLabel", "Paste Settings"),
		LOCTEXT("PasteSettingsTooltip", "Paste copied settings"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Paste"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::PasteSettingsToSelectedActors)));
}

void FOutlinerMenuSectionRegistry::CreateOutlinerToolkitModifyActorsSection(UToolMenu* ModifySectionMenu)
{
	FToolMenuSection& ModifySection = ModifySectionMenu->AddSection("ModifySection");
	ModifySection.AddMenuEntry(
		"AddPrefix",
		LOCTEXT("AddPrefixLabel", "Add Prefix"),
		LOCTEXT("AddPrefixTooltip", "Add prefix to labels"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::AddPrefixToSelectedActors)));

	ModifySection.AddMenuEntry(
		"AddSuffix",
		LOCTEXT("AddSuffixLabel", "Add Suffix"),
		LOCTEXT("AddSuffixTooltip", "Add suffix to labels"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::AddSuffixToSelectedActors)));

	ModifySection.AddMenuEntry(
		"RemovePrefix",
		LOCTEXT("RemovePrefixLabel", "Remove Prefix"),
		LOCTEXT("RemovePrefixTooltip", "Remove prefix to labels"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::AddPrefixToSelectedActors)));

	ModifySection.AddMenuEntry(
		"RemoveSuffix",
		LOCTEXT("RemoveSuffixLabel", "Remove Suffix"),
		LOCTEXT("AddSuffixTooltip", "Add suffix to labels"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"),
		FUIAction(FSimpleDelegate::CreateStatic(&OutlinerColumnUtils::AddSuffixToSelectedActors)));
}

void FOutlinerMenuSectionRegistry::CreateOutlinerToolkitScriptSection(UToolMenu* ScriptSectionMenu)
{
	FToolMenuSection& ScriptSection = ScriptSectionMenu->AddSection("Scripts");
	ScriptSection.AddMenuEntry(
		"EnableTick",
		LOCTEXT("EnableTickLabel", "Enable Tick"),
		LOCTEXT("EnableTickTooltip", "Enable Tick for selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsTickEnabled(true); })));
	ScriptSection.AddMenuEntry(
		"DisableTick",
		LOCTEXT("DisableTickLabel", "Disable Tick"),
		LOCTEXT("DisableTickTooltip", "Disable Tick for selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsTickEnabled(false); })));
	ScriptSection.AddMenuEntry(
		"ShowInGame",
		LOCTEXT("ShowInGameLabel", "Show In Game"),
		LOCTEXT("ShowInGameTooltip", "Disable Hidden In Game on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsHiddenInGame(false); })));
	ScriptSection.AddMenuEntry(
		"HideInGame",
		LOCTEXT("HideInGameLabel", "Hide In Game"),
		LOCTEXT("HideInGameTooltip", "Enable Hidden In Game on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsHiddenInGame(true); })));
	ScriptSection.AddMenuEntry(
		"EnableShadows",
		LOCTEXT("EnableShadowsLabel", "Enable Shadows"),
		LOCTEXT("EnableShadowsTooltip", "Enable Cast Shadows on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsCastShadows(true); })));
	ScriptSection.AddMenuEntry(
		"DisableShadows",
		LOCTEXT("DisableShadowsLabel", "Disable Shadows"),
		LOCTEXT("DisableShadowsTooltip", "Disable Cast Shadows on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsCastShadows(false); })));
	ScriptSection.AddMenuEntry(
		"EnableCustomDepth",
		LOCTEXT("EnableCustomDepthLabel", "Enable CustomDepth"),
		LOCTEXT("EnableCustomDepthTooltip", "Enable CustomDepth on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsCustomDepth(true); })));
	ScriptSection.AddMenuEntry(
		"DisableCustomDepth",
		LOCTEXT("DisableCustomDepthLabel", "Disable CustomDepth"),
		LOCTEXT("DisableCustomDepthTooltip", "Disable CustomDepth on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsCustomDepth(false); })));
	ScriptSection.AddMenuEntry(
		"EnableOverlapEvents",
		LOCTEXT("EnableOverlapEventsLabel", "Enable Overlap Events"),
		LOCTEXT("EnableOverlapEventsTooltip", "Enable overlap events on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsGenerateOverlapEvents(true); })));
	ScriptSection.AddMenuEntry(
		"DisableOverlapEvents",
		LOCTEXT("DisableOverlapEventsLabel", "Disable Overlap Events"),
		LOCTEXT("DisableOverlapEventsTooltip", "Disable overlap events on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsGenerateOverlapEvents(false); })));
	ScriptSection.AddMenuEntry(
		"EnablePhysics",
		LOCTEXT("EnablePhysicsLabel", "Enable Physics"),
		LOCTEXT("EnablePhysicsTooltip", "Enable physics simulation on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsSimulatePhysics(true); })));
	ScriptSection.AddMenuEntry(
		"DisablePhysics",
		LOCTEXT("DisablePhysicsLabel", "Disable Physics"),
		LOCTEXT("DisablePhysicsTooltip", "Disable physics simulation on selected actors."),
		FSlateIcon(),
		FUIAction(FSimpleDelegate::CreateLambda([]() { OutlinerColumnUtils::SetSelectedActorsSimulatePhysics(false); })));
}

#undef LOCTEXT_NAMESPACE
