// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValidatorX.h"
#include "ValidatorXManager.h"
#include "Widgets/SValidatorWidget.h"
#include "EditorValidatorSubsystem.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Layout/WidgetPath.h"

DEFINE_LOG_CATEGORY_STATIC(LogValidatorX, All, All);

#define LOCTEXT_NAMESPACE "FValidatorXModule"

const FName FValidatorXModule::ValidatorXTabName = "ValidatorX";

void FValidatorXModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FValidatorXModule::HandlePostEngineInit);

	// add the File->DataValidation menu subsection
	UToolMenus::Get()->RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FValidatorXModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ValidatorXTabName, FOnSpawnTab::CreateRaw(this, &FValidatorXModule::OnSpawnValidatorXTab)) //
		.SetDisplayName(LOCTEXT("TabTitle", "ValidatorX"))																						 //
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())																			 //
		.SetIcon(FSlateIcon(FName("EditorStyle"), "Icons.Validate"))																			 //
		.SetMenuType(ETabSpawnerMenuType::Enabled);																								 //
}

void FValidatorXModule::RegisterMenus()
{
	if (!IsRunningCommandlet() && !IsRunningGame() && FSlateApplication::IsInitialized())
	{
		UToolMenu* const  Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		FToolMenuSection& Section = Menu->FindOrAddSection("DataValidation");
		Section.AddEntry(FToolMenuEntry::InitMenuEntry(
			"ValidatorX",
			LOCTEXT("OpenValidatorX", "Open ValidatorX"),
			LOCTEXT("OpenValidatorXTooltip", "Opens the ValidatorX tool window."),
			FSlateIcon(FSlateIcon(FName("EditorStyle"), "Icons.Validate")),
			FUIAction(FExecuteAction::CreateRaw(this, &FValidatorXModule::OpenManagerTab))));
	}
}

void FValidatorXModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ValidatorXTabName);
	UToolMenus::UnregisterOwner(this);
}

ETabSpawnerMenuType::Type FValidatorXModule::GetVisibleModule() const
{
	if (FModuleManager::Get().IsModuleLoaded("ToolProjectEditor"))
	{
		ETabSpawnerMenuType::Enabled;
	}
	return ETabSpawnerMenuType::Hidden;
}

void FValidatorXModule::HandlePostEngineInit()
{
	if (GEditor)
	{
		UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
		if (ValidatorSubsystem)
		{
			ValidatorSubsystem->ForEachEnabledValidator(
				[this](UEditorValidatorBase* Validator) {
					if (UBlueprintValidatorBase* const BlueprintValidator = Cast<UBlueprintValidatorBase>(Validator))
					{
						FValidatorXManager::Get().RegisterValidator(BlueprintValidator);
						BlueprintValidator->SetValidationEnabled(false);
					}
					return true;
				});
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ValidatorSubsystem is nullptr"));
		}
	}
}
/* clang-format off */
TSharedRef<SDockTab> FValidatorXModule::OnSpawnValidatorXTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SValidatorWidget)
				.Validators(FValidatorXManager::Get().GetValidators())
			];
}
/* clang-format on */
void FValidatorXModule::OpenManagerTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ValidatorXTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FValidatorXModule, ValidatorX)