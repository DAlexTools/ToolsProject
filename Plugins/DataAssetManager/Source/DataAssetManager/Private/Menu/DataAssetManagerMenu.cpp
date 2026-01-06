// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/DataAssetManagerMenu.h"
#include "DataAssetManagerTypes.h"

/*--------------------------------------------------------------------------------------------------------------*/
/**
 * Macros to simplify creation of FUIAction and FNewMenuDelegate instances
 * for Slate UI elements.
 */

 /** Creates a basic FUIAction that calls a member function on a shared pointer object. */
#define CREATE_ACTION(Manager, Method) \
	FUIAction(FExecuteAction::CreateSP(Manager, Method))

/** Creates a FUIAction that calls a member function with one parameter on a shared pointer object. */
#define CREATE_ACTION_WITH_PARAM(Manager, Method, Param) \
	FUIAction(FExecuteAction::CreateSP(Manager, Method, Param))

/** Creates a static FNewMenuDelegate calling a static method with a manager pointer. */
#define CREATE_DELEGATE_ACTION(Manager, Method) \
	FNewMenuDelegate::CreateStatic(Method, Manager)

/** Creates a FUIAction with both Execute and CanExecute delegates on a shared pointer object. */
#define CREATE_ACTION_WITH_CAN_EXECUTE(Manager, ExecuteMethod, CanExecuteMethod) \
	FUIAction( \
		FExecuteAction::CreateSP(Manager, ExecuteMethod), \
		FCanExecuteAction::CreateSP(Manager, CanExecuteMethod) \
	)
/*--------------------------------------------------------------------------------------------------------------*/

void FDataAssetManagerMenu::FillFileMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface)
{
	using namespace DataAssetManagerMenu;

	MenuBuilder.BeginSection(ExtensionHookNames::ExtensionHookCreateName, Texts::CreateSectionText);
	MenuBuilder.AddMenuEntry(
		Texts::AddNewAssetText,
		Texts::AddNewAssetTooltip,
		Icons::AddNewAsset,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::CreateNewDataAsset));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(ExtensionHookNames::ExtensionHookEditName, Texts::EditSectionText);
	MenuBuilder.AddMenuEntry(
		Texts::SaveAssetText,
		Texts::SaveAssetTooltip,
		Icons::SaveAsset,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::SaveDataAsset));

	MenuBuilder.AddMenuEntry(
		Texts::SaveAllText, 
		Texts::SaveAllTooltip, 
		Icons::SaveAll,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::SaveAllData));

	MenuBuilder.AddMenuEntry(
		Texts::RenameText,
		Texts::RenameTooltip,
		Icons::Rename,
		CREATE_ACTION_WITH_CAN_EXECUTE(ManagerInterface, &IDataAssetManagerInterface::FocusOnSelectedAsset,
			&IDataAssetManagerInterface::CanRename));

	MenuBuilder.AddMenuEntry(
		Texts::DeleteText,
		Texts::DeleteTooltip,
		Icons::Delete,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::DeleteDataAsset));
	MenuBuilder.EndSection();
}

void FDataAssetManagerMenu::FillAssetsMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface)
{
	using namespace DataAssetManagerMenu;

	MenuBuilder.AddMenuEntry(
		Texts::OpenAssetText,
		Texts::OpenAssetTooltip,
		Icons::OpenAsset,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::OpenSelectedDataAssetInEditor));

	MenuBuilder.AddMenuEntry(
		Texts::FindInCBText,
		Texts::FindInCBTooltip,
		Icons::FindInCB,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::SyncContentBrowserToSelectedAsset));

	MenuBuilder.AddMenuEntry(
		Texts::ShowAssetMetadataText,
		Texts::ShowAssetMetadataTooltip,
		Icons::FindInCB,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::ShowAssetMetaData));

	MenuBuilder.AddMenuEntry(
		Texts::CopyRefText,
		Texts::CopyRefTooltip,
		Icons::Copy,
		CREATE_ACTION_WITH_PARAM(ManagerInterface, &IDataAssetManagerInterface::CopyToClipboard, false));

	MenuBuilder.AddMenuEntry(
		Texts::CopyPathsText,
		Texts::CopyPathsTooltip,
		Icons::Copy,
		CREATE_ACTION_WITH_PARAM(ManagerInterface, &IDataAssetManagerInterface::CopyToClipboard, true));

	MenuBuilder.AddMenuEntry(
		Texts::RefViewerText,
		Texts::RefViewerTooltip,
		Icons::ReferenceViewer,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::OpenReferenceViewer));

	MenuBuilder.AddMenuEntry(
		Texts::SizeMapText,
		Texts::SizeMapTooltip,
		Icons::SizeMap,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::OpenSizeMap));

	MenuBuilder.AddMenuEntry(
		Texts::AuditAssetText,
		Texts::AuditAssetTooltip,
		Icons::Audit,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::OpenAuditAsset));

	MenuBuilder.AddMenuEntry(
		Texts::RevisionControlText,
		Texts::RevisionControlTooltip,
		Icons::RevisionControl,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::ShowSourceControlDialog));
}

void FDataAssetManagerMenu::FillSettingsMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface)
{
	using namespace DataAssetManagerMenu;

	MenuBuilder.BeginSection(ExtensionHookNames::ExtensionHookDebugName, Texts::DebugSectionText);
	MenuBuilder.AddMenuEntry(
		Texts::OpenMessageLogText,
		Texts::OpenMessageLogTooltip,
		Icons::MessageLog,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::OpenMessageLogWindow));

	MenuBuilder.AddMenuEntry(
		Texts::OpenOutputLogText,
		Texts::OpenOutputLogTooltip,
		Icons::OutputLog,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::OpenOutputLogWindow));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(ExtensionHookNames::ExtensionHookSettingsName, Texts::SettingsSectionText);
	MenuBuilder.AddMenuEntry(
		Texts::ShowAssetsListText,
		Texts::ShowAssetsListTooltip,
		Icons::Visibility,
		CREATE_ACTION(ManagerInterface,&IDataAssetManagerInterface::ToggleDataAssetListVisibility));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(ExtensionHookNames::ExtensionHookPluginSettingsName, Texts::PluginSettingsSectionText);
	MenuBuilder.AddMenuEntry(
		Texts::PluginSettingsText,
		Texts::PluginSettingsTooltip,
		Icons::Settings,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::OpenPluginSettings));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(ExtensionHookNames::ExtensionHookRestartName, Texts::RestartSectionText);
	MenuBuilder.AddMenuEntry(
		Texts::RestartPluginText,
		Texts::RestartPluginTooltip,
		Icons::Refresh,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::RestartPlugin));
	MenuBuilder.EndSection();
}

void FDataAssetManagerMenu::FillHelpMenu(FMenuBuilder& MenuBuilder, TSharedRef<IDataAssetManagerInterface> ManagerInterface)
{
	using namespace DataAssetManagerMenu;
	MenuBuilder.AddMenuEntry(
		Texts::DocumentationText,
		Texts::DocumentationTooltip,
		Icons::Documentation,
		CREATE_ACTION(ManagerInterface, &IDataAssetManagerInterface::ShowDocumentation));
}

TSharedRef<SWidget> FDataAssetManagerMenuFactory::CreateMenuBar(TSharedRef<IDataAssetManagerInterface> ManagerInterface)
{
	using namespace DataAssetManagerMenu;

	FMenuBarBuilder MenuBuilder(NULL);
	MenuBuilder.AddPullDownMenu(
		Texts::FileMenuText,
		Texts::FileMenuTooltip,
		CREATE_DELEGATE_ACTION(ManagerInterface, &FDataAssetManagerMenu::FillFileMenu));

	MenuBuilder.AddPullDownMenu(
		Texts::AssetMenuText,
		Texts::AssetMenuTooltip,
		CREATE_DELEGATE_ACTION(ManagerInterface, &FDataAssetManagerMenu::FillAssetsMenu));

	MenuBuilder.AddPullDownMenu(
		Texts::SettingsMenuText,
		Texts::SettingsMenuTooltip,
		CREATE_DELEGATE_ACTION(ManagerInterface, &FDataAssetManagerMenu::FillSettingsMenu));

	MenuBuilder.AddPullDownMenu(
		Texts::HelpMenuText,
		Texts::HelpMenuTooltip,
		CREATE_DELEGATE_ACTION(ManagerInterface, &FDataAssetManagerMenu::FillHelpMenu));

	return MenuBuilder.MakeWidget();
}



