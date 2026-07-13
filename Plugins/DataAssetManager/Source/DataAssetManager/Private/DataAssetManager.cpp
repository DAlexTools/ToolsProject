// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "DataAssetManager.h"
#include "Modules/ModuleManager.h"
#include "StatusBarSubsystem.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "UI/SDataAssetManagerWidget.h"
#include "UI/SDeveloperSettingsWidget.h"
#include "DataAssetManagerTypes.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FDataAssetManagerModule"

namespace DataAssetManager::UI
{
	inline constexpr float TabReopenDelaySeconds = 0.1f;
	const FName DataAssetClassicIconName("LevelEditor.Tabs.StatsViewer");

}

void FDataAssetManagerModule::StartupModule()
{
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(DataAssetManager::ModuleName::PropertyEditor);
		PropertyEditorModule.RegisterCustomClassLayout(UDataAssetManagerSettings::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&SDeveloperSettingsWidget::MakeInstance));
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}

	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			DataAssetManager::DataAssetManagerTabName,
			FOnSpawnTab::CreateRaw(this, &FDataAssetManagerModule::CreateDataAssetManagerTab))
			.SetDisplayName(LOCTEXT("FDataAssetManagerModule", "Data Asset Manager"))
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), DataAssetManager::UI::DataAssetClassicIconName))
			.SetMenuType(ETabSpawnerMenuType::Hidden);
	}

	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDataAssetManagerModule::RegisterMenus));
	}
}

void FDataAssetManagerModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded(DataAssetManager::ModuleName::PropertyEditor))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(DataAssetManager::ModuleName::PropertyEditor);
		PropertyEditorModule.UnregisterCustomClassLayout(UDataAssetManagerSettings::StaticClass()->GetFName());
	}

	if (FGlobalTabmanager::Get()->HasTabSpawner(DataAssetManager::DataAssetManagerTabName))
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DataAssetManager::DataAssetManagerTabName);
	}

	UToolMenus::UnRegisterStartupCallback(this);
	ClearAllTimerIfNeeded();
}

void FDataAssetManagerModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = Menu->FindOrAddSection("Tools");

	Section.AddMenuEntry(
		"OpenDataAssetManager",
		LOCTEXT("OpenDataAssetManager", "Data Asset Manager"),
		LOCTEXT("OpenDataAssetManagerTooltip", "Open Data Asset Manager tab"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.DataAsset"),
		FUIAction(FExecuteAction::CreateRaw(this, &FDataAssetManagerModule::OpenManagerTab))
	);
}

void FDataAssetManagerModule::ClearAllTimerIfNeeded()
{
	if (!IsValid(GEditor))
	{
		return;
	}

	GEditor->GetTimerManager().Get().ClearTimer(RestartTimerHandle);
}

[[nodiscard]] TSharedRef<SDockTab> FDataAssetManagerModule::CreateDataAssetManagerTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> DockTab = SNew(SDockTab).TabRole(ETabRole::NomadTab).Icon(FAppStyle::Get().GetBrush("ClassIcon.DataAsset"));
	DockTab->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(DataAssetWidget, SDataAssetManagerWidget)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			CreateStatusBarWidget(DockTab)
		]
	);

	return DockTab;
}

[[nodiscard]] TSharedRef<SWidget> FDataAssetManagerModule::CreateStatusBarWidget(const TSharedRef<SDockTab>& Tab) const
{
	if (UStatusBarSubsystem* StatusBar = GetStatusBarSubsystem(); IsValid(StatusBar))
	{
		return StatusBar->MakeStatusBarWidget(DataAssetManager::StatusBarName, Tab);
	}

	return SNullWidget::NullWidget;
}

[[nodiscard]] UStatusBarSubsystem* FDataAssetManagerModule::GetStatusBarSubsystem() const
{
	return GEditor ? GEditor->GetEditorSubsystem<UStatusBarSubsystem>() : nullptr;
}

void FDataAssetManagerModule::RestartWidget()
{
	if (RestartTimerHandle.IsValid())
	{
		GEditor->GetTimerManager().Get().ClearTimer(RestartTimerHandle);
	}

	const TSharedPtr<SDockTab> DataAssetManagerTab = FGlobalTabmanager::Get()->FindExistingLiveTab(DataAssetManager::DataAssetManagerTabName);
	if (!DataAssetManagerTab.IsValid())
	{
		return;
	}

	DataAssetManagerTab->RequestCloseTab();
	if (!IsValid(GEditor))
	{
		return;
	}

	/* Set a timer to reopen the tab after a short delay. */
	/* Non-obvious case: you can't open a tab within one tick. */
	GEditor->GetTimerManager().Get().SetTimer(
		RestartTimerHandle,
		[this]()
		{
			OpenManagerTab();
		},
		DataAssetManager::TabReopenDelaySeconds,
		false);
}

void FDataAssetManagerModule::OpenManagerTab()
{
	if (!FGlobalTabmanager::Get()->FindExistingLiveTab(DataAssetManager::DataAssetManagerTabName))
	{
		FGlobalTabmanager::Get()->TryInvokeTab(DataAssetManager::DataAssetManagerTabName);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDataAssetManagerModule, DataAssetManager)
