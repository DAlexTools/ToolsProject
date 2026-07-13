// Copyright Epic Games, Inc. All Rights Reserved.

#include "ValidatorX.h"
#include "BaseClasses/ValidatorXBase.h"
#include "ValidatorXManager.h"
#include "Widgets/SValidatorWidget.h"
#include "EditorValidatorSubsystem.h"

#include "Layout/WidgetPath.h"
DEFINE_LOG_CATEGORY_STATIC(LogValidatorX, All, All);

#define LOCTEXT_NAMESPACE "FValidatorXModule"

const FName FValidatorXModule::ValidatorXTabName = "ValidatorX";

void FValidatorXModule::StartupModule()
{
	PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FValidatorXModule::HandlePostEngineInit);

	// add the File->DataValidation menu subsection
	UToolMenus::Get()->RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FValidatorXModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ValidatorXTabName, FOnSpawnTab::CreateRaw(this, &FValidatorXModule::OnSpawnValidatorXTab))
     .SetDisplayName(NSLOCTEXT("ValidatorX", "TabTitle", "ValidatorX"))
     .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FValidatorXModule::RegisterMenus()
{
	if(!IsRunningCommandlet() && !IsRunningGame() && FSlateApplication::IsInitialized())
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
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
	if(PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ValidatorXTabName);
	UToolMenus::UnregisterOwner(this);
}

void FValidatorXModule::HandlePostEngineInit()
{
	UE_LOG(LogTemp, Warning, TEXT("Startup Begin"));

	if(GEditor)
	{
		UE_LOG(LogTemp, Warning, TEXT("GEditor is valid"));

		UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
		if(ValidatorSubsystem)
		{
			UE_LOG(LogTemp, Warning, TEXT("ValidatorSubsystem is valid"));

			ValidatorSubsystem->ForEachEnabledValidator(
				[this] (UEditorValidatorBase* Validator)
				{
					UE_LOG(LogTemp, Warning, TEXT("Validator found: %s"), *Validator->GetName());

					if(UValidatorXBase* ValidatorX = Cast<UValidatorXBase>(Validator))
					{
						UE_LOG(LogTemp, Warning, TEXT("Registering ValidatorX validator: %s"), *ValidatorX->GetName());
						FValidatorXManager::Get().RegisterValidator(ValidatorX);
						ValidatorX->SetValidationEnabled(false);
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

TSharedRef<SDockTab> FValidatorXModule::OnSpawnValidatorXTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SValidatorWidget)
			.Validators(FValidatorXManager::Get().GetValidators())
		];
}

void FValidatorXModule::OpenManagerTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ValidatorXTabName);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FValidatorXModule, ValidatorX)
