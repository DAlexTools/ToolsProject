// Copyright Epic Games, Inc. All Rights Reserved.

#include "DataAssetManager.h"
#include "Modules/ModuleManager.h"

// if WITH_EDITOR
#include "Editor/EditorEngine.h"
#include "StatusBarSubsystem.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "UI/SDataAssetManagerWidget.h"
#include "UI/SDeveloperSettingsWidget.h"
#include "DataAssetManagerTypes.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FDataAssetManagerModule"

void FDataAssetManagerModule::StartupModule()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(DataAssetManager::ModuleName::PropertyEditor);
	PropertyEditorModule.RegisterCustomClassLayout(UDataAssetManagerSettings::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&SDeveloperSettingsWidget::MakeInstance));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(DataAssetManager::DataAssetManagerTabName, FOnSpawnTab::CreateRaw(this, &FDataAssetManagerModule::CreateDataAssetManagerTab)) //
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())																												//
		.SetDisplayName(LOCTEXT("FDataAssetManagerModule", "Data Asset Manager"))																									//
		.SetIcon(FSlateIcon(FName("EditorStyle"), "ClassIcon.DataAsset"))																											//
		.SetMenuType(ETabSpawnerMenuType::Enabled);																																	//

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDataAssetManagerModule::ModifyMenus));
}

void FDataAssetManagerModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded(DataAssetManager::ModuleName::PropertyEditor))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(DataAssetManager::ModuleName::PropertyEditor);
		PropertyEditorModule.UnregisterCustomClassLayout(UDataAssetManagerSettings::StaticClass()->GetFName());
	}
}

ETabSpawnerMenuType::Type FDataAssetManagerModule::GetVisibleModule() const
{
	if (FModuleManager::Get().IsModuleLoaded(DataAssetManager::ToolProjectEditor))
	{
		return ETabSpawnerMenuType::Enabled;
	}
	return ETabSpawnerMenuType::Hidden;
}
/* clang-format off */
TSharedRef<SDockTab> FDataAssetManagerModule::CreateDataAssetManagerTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> DataAssetManagerTab = SNew(SDockTab).TabRole(ETabRole::NomadTab);
	UStatusBarSubsystem* StatusBarSubsystem = GEditor->GetEditorSubsystem<UStatusBarSubsystem>();
	if(StatusBarSubsystem)
	{
		TSharedRef<SWidget> StatusBarWidget = StatusBarSubsystem->MakeStatusBarWidget(DataAssetManager::StatusBarName, DataAssetManagerTab);
		DataAssetManagerTab->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SDataAssetManagerWidget)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				StatusBarWidget
			]
		);
	}
	else
	{
		DataAssetManagerTab->SetContent(SNew(SDataAssetManagerWidget));
	}

	return DataAssetManagerTab;
}
/* clang-format on */
void FDataAssetManagerModule::ModifyMenus()
{
	if (UToolMenu* Menu = UToolMenus::Get()->FindMenu("LevelEditor.MainMenu.File"))
	{
		if (FToolMenuSection* Section = Menu->FindSection("FileLoadAndSave"))
		{
			if (FToolMenuEntry* Entry = Section->FindEntry("Delete"))
			{
				FUIAction HiddenAction(
					FExecuteAction(),
					FCanExecuteAction(),
					FIsActionChecked(),
					FIsActionButtonVisible::CreateLambda([]() { return false; }));

				Entry->SetCommandList(nullptr);
			}
		}
	}
}

void FDataAssetManagerModule::RestartWidget()
{
	TSharedPtr<SDockTab> DataAssetManagerTab = FGlobalTabmanager::Get()->FindExistingLiveTab(DataAssetManager::DataAssetManagerTabName);
	if (DataAssetManagerTab.IsValid())
	{
		DataAssetManagerTab->RequestCloseTab();

		// Set timer to reopen the tab after a short delay.
		FTimerHandle TimerHandle;
		GEditor->GetTimerManager().Get().SetTimer(TimerHandle, [this]() { OpenManagerTab(); }, DataAssetManager::TabReopenDelaySeconds, false);
	}
}

void FDataAssetManagerModule::OpenManagerTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DataAssetManager::DataAssetManagerTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDataAssetManagerModule, DataAssetManager)