// Copyright Epic Games, Inc. All Rights Reserved.

#include "PerformanceInspectorEditor.h"

#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "PerformanceCaptureSubsystem.h"
#include "SPerformanceInspectorPanel.h"
#include "Settings/PerformanceInspectorAutomationSettings.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FPerformanceInspectorEditorModule"

namespace PerformanceInspector
{
	const FName PerformanceInspectorTabId(TEXT("PerformanceInspector"));
	const FName StatsIcon("LevelEditor.Tabs.StatsViewer");

	UPerformanceCaptureSubsystem* GetPIECaptureSubsystem()
	{
		if (!GEngine)
		{
			return nullptr;
		}
	
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType != EWorldType::PIE)
			{
				continue;
			}
	
			if (const UWorld* World = WorldContext.World())
			{
				if (const UGameInstance* GameInstance = World->GetGameInstance())
				{
					return GameInstance->GetSubsystem<UPerformanceCaptureSubsystem>();
				}
			}
		}
	
		return nullptr;
	}
}

using namespace PerformanceInspector;

class FPerformanceInspectorCommands : public TCommands<FPerformanceInspectorCommands>
{
public:
	FPerformanceInspectorCommands() : TCommands<FPerformanceInspectorCommands>(
		TEXT("PerformanceInspector"),
		LOCTEXT("PerformanceInspector", "Performance Inspector"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
	{
	}

	virtual void RegisterCommands() override
	{
		UI_COMMAND(OpenTool,
			"PI",
			"Open Performance Inspector",
			EUserInterfaceActionType::Button,
			FInputChord(EKeys::Q, EModifierKey::Shift));

		UI_COMMAND(OpenSettings,
			"Settings",
			"Open Plugin Settings",
			EUserInterfaceActionType::Button,
			FInputChord());
	}

	TSharedPtr<FUICommandInfo> OpenTool;
	TSharedPtr<FUICommandInfo> OpenSettings;
};

void FPerformanceInspectorEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		PerformanceInspectorTabId,
		FOnSpawnTab::CreateRaw(this, &FPerformanceInspectorEditorModule::CreatePerformanceInspectorTab))
		.SetDisplayName(LOCTEXT("PerformanceInspectorTabTitle", "Performance Inspector"))
		.SetTooltipText(LOCTEXT("PerformanceInspectorTabTooltip", "Open the Performance Inspector"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), StatsIcon));

	FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		FPerformanceInspectorCommands::Register();
		PluginCommands = MakeShared<FUICommandList>();
		PluginCommands->MapAction(
			FPerformanceInspectorCommands::Get().OpenTool,
			FExecuteAction::CreateRaw(this, &FPerformanceInspectorEditorModule::OnOpenTool),
			FCanExecuteAction());

		PluginCommands->MapAction(
			FPerformanceInspectorCommands::Get().OpenSettings,
			FExecuteAction::CreateRaw(this, &FPerformanceInspectorEditorModule::OnOpenSettings),
			FCanExecuteAction());
	}

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
		ToolbarExtender->AddToolBarExtension(
			"Content",
			EExtensionHook::After,
			PluginCommands,
			FToolBarExtensionDelegate::CreateRaw(this, &FPerformanceInspectorEditorModule::AddPIToolBarButton));
		LevelEditor.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

	}

	BeginPIEHandle = FEditorDelegates::BeginPIE.AddRaw(this, &FPerformanceInspectorEditorModule::HandleBeginPIE);
	EndPIEHandle = FEditorDelegates::EndPIE.AddRaw(this, &FPerformanceInspectorEditorModule::HandleEndPIE);
}

void FPerformanceInspectorEditorModule::OnOpenTool()
{
	FGlobalTabmanager::Get()->TryInvokeTab(PerformanceInspectorTabId);
}

void FPerformanceInspectorEditorModule::OnOpenSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (!SettingsModule)
	{
		return;
	}

	const UPerformanceInspectorAutomationSettings* Settings = GetDefault<UPerformanceInspectorAutomationSettings>();
	if (!Settings)
	{
		return;
	}

	SettingsModule->ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
}

void FPerformanceInspectorEditorModule::AddPIToolBarButton(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(
		FPerformanceInspectorCommands::Get().OpenTool,
		NAME_None,
		LOCTEXT("OpenPerformanceInspector", "Performance Inspector"),
		LOCTEXT("OpenPerformanceInspectorTooltip", "Open the Performance Inspector"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), StatsIcon));
}

void FPerformanceInspectorEditorModule::ShutdownModule()
{
	if (BeginPIEHandle.IsValid())
	{
		FEditorDelegates::BeginPIE.Remove(BeginPIEHandle);
	}

	if (EndPIEHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPIEHandle);
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PerformanceInspectorTabId);
}

TSharedRef<SDockTab> FPerformanceInspectorEditorModule::CreatePerformanceInspectorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SPerformanceInspectorPanel)
		];
}

void FPerformanceInspectorEditorModule::HandleBeginPIE(bool bIsSimulating)
{
	const UPerformanceInspectorAutomationSettings* Settings = GetDefault<UPerformanceInspectorAutomationSettings>();
	if (!Settings || !Settings->bAutoStartCaptureOnPIE || bIsSimulating)
	{
		return;
	}

	if (UPerformanceCaptureSubsystem* Subsystem = GetPIECaptureSubsystem())
	{
		Subsystem->StartTimedCaptureSession(Settings->PIECaptureDurationSeconds, Settings->PIEThresholds, false, false);
		Subsystem->AddEventMarker(TEXT("PIECaptureStart"), TEXT("Automation"), TEXT("PIE auto capture started"));
	}
}

void FPerformanceInspectorEditorModule::HandleEndPIE(bool bIsSimulating)
{
	const UPerformanceInspectorAutomationSettings* Settings = GetDefault<UPerformanceInspectorAutomationSettings>();
	if (!Settings || !Settings->bStopCaptureOnEndPIE || bIsSimulating)
	{
		return;
	}

	if (UPerformanceCaptureSubsystem* Subsystem = GetPIECaptureSubsystem())
	{
		if (Subsystem->IsCaptureSessionActive())
		{
			Subsystem->AddEventMarker(TEXT("PIECaptureStop"), TEXT("Automation"), TEXT("PIE ended before timed capture completed"));
			Subsystem->StopCaptureSession();
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPerformanceInspectorEditorModule, PerformanceInspectorEditor)
