// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RevisionControlStyle/RevisionControlStyle.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"

#define LOCTEXT_NAMESPACE "SDataAssetManagerWidget"

/**
 * @brief Localized menu text, icon names, and section identifiers for Data Asset Manager menus.
 */
namespace DataAssetManagerMenu
{
	/**
	 * @brief Style set names used by menu icons.
	 */
	namespace IconStyle
	{
		/** @brief Unreal editor app style set name. */
		static const FName AppStyle = FAppStyle::GetAppStyleSetName();

		/** @brief Revision control style set name. */
		static const FName RevisionControlStyle = FRevisionControlStyleManager::GetStyleSetName();
	} // namespace IconStyle

	/**
	 * @brief Icons assigned to Data Asset Manager menu entries.
	 */
	namespace Icons
	{
		const FSlateIcon AddNewAsset = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.AssetActions.ReimportAsset");
		const FSlateIcon SaveAsset = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.SaveAllCurrentFolder");
		const FSlateIcon SaveAll = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.SaveAllCurrentFolder");
		const FSlateIcon Validate = FSlateIcon(IconStyle::AppStyle, "Icons.Adjust");
		const FSlateIcon Rename = FSlateIcon(IconStyle::AppStyle, "GenericCommands.Rename");
		const FSlateIcon Delete = FSlateIcon(IconStyle::AppStyle, "GenericCommands.Delete");

		const FSlateIcon OpenAsset = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.ShowInExplorer");
		const FSlateIcon FindInCB = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.ShowInExplorer");
		const FSlateIcon Copy = FSlateIcon(IconStyle::AppStyle, "GenericCommands.Copy");
		const FSlateIcon ReferenceViewer = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.ReferenceViewer");
		const FSlateIcon ReferenceInspector = FSlateIcon(IconStyle::AppStyle, "Icons.Search");
		const FSlateIcon DataAssetDiff = FSlateIcon(IconStyle::RevisionControlStyle, "RevisionControl.Actions.Diff");
		const FSlateIcon SizeMap = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.SizeMap");
		const FSlateIcon Audit = FSlateIcon(IconStyle::AppStyle, "Icons.Audit");
		const FSlateIcon RevisionControl = FSlateIcon(IconStyle::RevisionControlStyle, "RevisionControl.Actions.Diff");

		const FSlateIcon MessageLog = FSlateIcon(IconStyle::AppStyle, "MessageLog.TabIcon");
		const FSlateIcon Visibility = FSlateIcon(IconStyle::AppStyle, "Icons.Visibility");
		const FSlateIcon Settings = FSlateIcon(IconStyle::AppStyle, "Icons.Settings");
		const FSlateIcon Refresh = FSlateIcon(IconStyle::AppStyle, "Icons.Refresh");
		const FSlateIcon OutputLog = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Log.TabIcon");

		const FSlateIcon Documentation = FSlateIcon(IconStyle::AppStyle, "GraphEditor.GoToDocumentation");
	} // namespace Icons

	/**
	 * @brief Section names used to place menu extensions.
	 */
	namespace ExtensionHookNames
	{
		const FName ExtensionHookCreateName = TEXT("Created");
		const FName ExtensionHookEditName = TEXT("Edit");
		const FName ExtensionHookValidateName = TEXT("Validate");
		const FName ExtensionHookDebugName = TEXT("Debug");
		const FName ExtensionHookSettingsName = TEXT("Settings");
		const FName ExtensionHookPluginSettingsName = TEXT("PluginSettings");
		const FName ExtensionHookRestartName = TEXT("Restart");
	} // namespace ExtensionHookNames

	/**
	 * @brief Localized labels and tooltips used by Data Asset Manager menus.
	 */
	namespace Texts
	{
		const FText CreateSectionText = LOCTEXT("CreateSection", "Create");
		const FText AddNewAssetText = LOCTEXT("AddNewAsset", "Add New Data Asset");
		const FText AddNewAssetTooltip = LOCTEXT("AddNewAssetTooltip", "Create new Data Asset in Content Browser");
		const FText EditSectionText = LOCTEXT("EditSection", "Edit");
		const FText RenameText = LOCTEXT("RenameAsset", "Rename");
		const FText RenameTooltip = LOCTEXT("RenameTooltip", "Rename selected asset");
		const FText DeleteText = LOCTEXT("DeleteAsset", "Delete");
		const FText DeleteTooltip = LOCTEXT("DeleteTooltip", "Delete selected asset");
		const FText SaveAssetText = LOCTEXT("SaveAsset", "Save");
		const FText SaveAssetTooltip = LOCTEXT("SaveAssetTooltip", "Save the selected Data Asset");
		const FText SaveAllText = LOCTEXT("SaveAll", "Save All");
		const FText SaveAllTooltip = LOCTEXT("SaveAllTooltip", "Save all modified Data Assets");

		const FText OpenAssetText = LOCTEXT("OpenAsset", "Open Asset");
		const FText OpenAssetTooltip = LOCTEXT("OpenAssetTooltip", "Open the selected Data Asset in editor");
		const FText FindInCBText = LOCTEXT("FindInContentBrowser", "Find in Content Browser");
		const FText FindInCBTooltip = LOCTEXT("FindInContentBrowserTooltip", "Locate asset in Content Browser");
		const FText CopyRefText = LOCTEXT("CopyReference", "Copy Reference");
		const FText CopyRefTooltip = LOCTEXT("CopyReferenceTooltip", "Copy asset reference to clipboard");
		const FText CopyPathsText = LOCTEXT("CopyPaths", "Copy Paths");
		const FText CopyPathsTooltip = LOCTEXT("CopyPathsTooltip", "Copy asset paths to clipboard");
		const FText RefViewerText = LOCTEXT("ReferenceViewer", "Reference Viewer");
		const FText RefViewerTooltip = LOCTEXT("ReferenceViewerTooltip", "Open reference viewer for this asset");
		const FText ReferenceInspectorText = LOCTEXT("ReferenceInspector", "Reference Inspector");
		const FText ReferenceInspectorTooltip = LOCTEXT("ReferenceInspectorTooltip", "Inspect assets referenced by this Data Asset and assets that reference it");
		const FText DataAssetDiffText = LOCTEXT("DataAssetDiff", "DataAsset Diff");
		const FText DataAssetDiffTooltip = LOCTEXT("DataAssetDiffTooltip", "Compare editable properties for two selected Data Assets of the same class");
		const FText SizeMapText = LOCTEXT("SizeMap", "Size Map");
		const FText SizeMapTooltip = LOCTEXT("SizeMapTooltip", "View asset size information");
		const FText AuditAssetText = LOCTEXT("AuditAsset", "Audit Asset");
		const FText AuditAssetTooltip = LOCTEXT("AuditAssetTooltip", "Audit asset metadata");
		const FText RevisionControlText = LOCTEXT("RevisionControl", "Revision Control");
		const FText RevisionControlTooltip = LOCTEXT("RevisionControlTooltip", "Open revision control menu");
		const FText ShowAssetMetadataText = LOCTEXT("ShowAssetMetaData", "Show Asset Metadata");
		const FText ShowAssetMetadataTooltip = LOCTEXT("ShowAssetMetadataTooltip", "Display the metadata information of the selected asset.");

		const FText DebugSectionText = LOCTEXT("DebugSection", "Debug");
		const FText OpenMessageLogText = LOCTEXT("OpenMessageLog_Label", "Open Message Log");
		const FText OpenMessageLogTooltip = LOCTEXT("OpenMessageLog_Tooltip", "Opens the Message Log window");
		const FText OpenOutputLogText = LOCTEXT("OpenOutputLog_Label", "Open Output Log");
		const FText OpenOutputLogTooltip = LOCTEXT("OpenOutputLog_Tooltip", "Opens the Output Log window");
		const FText SettingsSectionText = LOCTEXT("SettingsSection", "Settings");
		const FText ShowAssetsListText = LOCTEXT("ShowAssetsList", "Show Assets List");
		const FText ShowAssetsListTooltip = LOCTEXT("ShowAssetsListTooltip", "Toggle assets list visibility");
		const FText PluginSettingsSectionText = LOCTEXT("PluginSettingsSection", "Plugin");
		const FText PluginSettingsText = LOCTEXT("PluginSettings", "Plugin Settings");
		const FText PluginSettingsTooltip = LOCTEXT("PluginSettingsTooltip", "Open plugin settings");
		const FText RestartSectionText = LOCTEXT("RestartSection", "Maintenance");
		const FText RestartPluginText = LOCTEXT("RestartPlugin", "Restart Plugin");
		const FText RestartPluginTooltip = LOCTEXT("RestartPluginTooltip", "Restart the plugin");

		const FText DocumentationText = LOCTEXT("Documentation", "Documentation");
		const FText DocumentationTooltip = LOCTEXT("DocumentationTooltip", "Open documentation");

		const FText FileMenuText = LOCTEXT("FileMenu", "File");
		const FText FileMenuTooltip = LOCTEXT("FileMenuTooltip", "File operations");
		const FText AssetMenuText = LOCTEXT("AssetMenu", "Asset");
		const FText AssetMenuTooltip = LOCTEXT("AssetMenuTooltip", "Asset operations");
		const FText SettingsMenuText = LOCTEXT("SettingsMenu", "Settings");
		const FText SettingsMenuTooltip = LOCTEXT("SettingsMenuTooltip", "Plugin settings");
		const FText HelpMenuText = LOCTEXT("HelpMenu", "Help");
		const FText HelpMenuTooltip = LOCTEXT("HelpMenuTooltip", "Help and documentation");
	} // namespace Texts
} // namespace DataAssetManagerMenu

#undef LOCTEXT_NAMESPACE
