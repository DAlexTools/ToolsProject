#include "ToolsProjectEditor.h"

#include "DataAssetManager.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "TextureChannelPacker.h"
#include "ToolMenus.h"

DEFINE_LOG_CATEGORY(ToolsProjectEditor);

#define LOCTEXT_NAMESPACE "FToolsProjectEditor"

namespace ToolsProjectEditorModule
{
	const FName ToolbarSectionName(TEXT("ToolsProjectEditor"));
	const FName DataAssetManagerModuleName(TEXT("DataAssetManager"));
	const FName OutlinerToolkitAuditTabId(TEXT("OutlinerToolkitAudit"));
	const FName OutlinerToolkitModuleName(TEXT("OutlinerToolkit"));
	const FName TextureChannelPackerModuleName(TEXT("TextureChannelPacker"));
}

static FAutoConsoleCommand ImportTexturesCmd(
	TEXT("Import.Textures"),
	TEXT("Import textures from folder"),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
		
	}));

void FToolsProjectEditor::StartupModule()
{
	UE_LOG(ToolsProjectEditor, Warning, TEXT("ToolsProjectEditor module has been loaded"));

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FToolsProjectEditor::RegisterMenus));
}

void FToolsProjectEditor::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::Get()->UnregisterOwner(this);
	}

	UE_LOG(ToolsProjectEditor, Warning, TEXT("ToolsProjectEditor module has been unloaded"));
}

void FToolsProjectEditor::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
	FToolMenuSection& ToolbarSection = Toolbar->FindOrAddSection(ToolsProjectEditorModule::ToolbarSectionName);

	ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
		"OpenDataAssetManagerToolbar",
		FUIAction(FExecuteAction::CreateRaw(this, &FToolsProjectEditor::OpenDataAssetManager)),
		LOCTEXT("OpenDataAssetManagerToolbar", "Data Assets"),
		LOCTEXT("OpenDataAssetManagerToolbarTooltip", "Open Data Asset Manager"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.DataAsset")));

	ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
		"OpenTextureChannelPackerToolbar",
		FUIAction(FExecuteAction::CreateRaw(this, &FToolsProjectEditor::OpenTextureChannelPacker)),
		LOCTEXT("OpenTextureChannelPackerToolbar", "Texture Packer"),
		LOCTEXT("OpenTextureChannelPackerToolbarTooltip", "Open Texture Channel Packer"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Layout")));

	ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
		"OpenOutlinerToolkitAuditToolbar",
		FUIAction(FExecuteAction::CreateRaw(this, &FToolsProjectEditor::OpenOutlinerToolkitAudit)),
		LOCTEXT("OpenOutlinerToolkitAuditToolbar", "Outliner Audit"),
		LOCTEXT("OpenOutlinerToolkitAuditToolbarTooltip", "Open Outliner Toolkit Audit"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Warning")));
}

void FToolsProjectEditor::OpenDataAssetManager()
{
	IDataAssetManagerModule& DataAssetManagerModule =
		FModuleManager::LoadModuleChecked<IDataAssetManagerModule>(ToolsProjectEditorModule::DataAssetManagerModuleName);
	DataAssetManagerModule.OpenManagerTab();
}

void FToolsProjectEditor::OpenOutlinerToolkitAudit()
{
	FModuleManager::LoadModuleChecked<IModuleInterface>(ToolsProjectEditorModule::OutlinerToolkitModuleName);
	FGlobalTabmanager::Get()->TryInvokeTab(ToolsProjectEditorModule::OutlinerToolkitAuditTabId);
}

void FToolsProjectEditor::OpenTextureChannelPacker()
{
	ITextureChannelPackerModule& TextureChannelPackerModule =
		FModuleManager::LoadModuleChecked<ITextureChannelPackerModule>(ToolsProjectEditorModule::TextureChannelPackerModuleName);
	TextureChannelPackerModule.OpenPluginWindow();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FToolsProjectEditor, ToolsProjectEditor)
