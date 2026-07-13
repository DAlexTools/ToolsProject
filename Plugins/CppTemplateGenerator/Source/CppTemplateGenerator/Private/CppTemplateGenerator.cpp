// Copyright 2025 DimAlek. All Rights Reserved.

#include "CppTemplateGenerator.h"
#include "CppTemplateGeneratorSettings.h"
#include "Components/ActorComponent.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/Actor.h"
#include "GameProjectGenerationModule.h"
#include "GameProjectUtils.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "ToolMenu.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FCppTemplateGeneratorModule"

DEFINE_LOG_CATEGORY_STATIC(CppTemplateGeneratorLog, All, All);

void FCppTemplateGeneratorModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this,&FCppTemplateGeneratorModule::RegisterMenus));
}

void FCppTemplateGeneratorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		const UToolMenus* const ToolMenus = UToolMenus::Get();
		if (IsValid(ToolMenus))
		{
			ToolMenus->UnregisterOwner(this);
		}
	}
}

void FCppTemplateGeneratorModule::RegisterMenus()
{
	UToolMenu* const Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Tools");
	FToolMenuSection& Section = Menu->FindOrAddSection("Programming");

	const UCppTemplateGeneratorSettings* const Settings = GetDefault<UCppTemplateGeneratorSettings>();
	if (!IsValid(Settings))
	{
		return;
	}

	Section.AddSubMenu(
		"CreateCppTemplate",
		LOCTEXT("CreateCppTemplateLabel", "New C++ Template..."),
		LOCTEXT("CreateCppTemplateTooltip", "Create a C++ class from your predefined template"),
		FNewToolMenuDelegate::CreateLambda([this, Settings](UToolMenu* InMenu) 
		{
			FToolMenuSection& SubSection = InMenu->AddSection("CppTemplateSection");

			SubSection.AddMenuEntry(
				"OpenCppTemplateGeneratorSettings",
				LOCTEXT("OpenCppTemplateGeneratorSettingsLabel", "Template Settings..."),
				LOCTEXT("OpenCppTemplateGeneratorSettingsTooltip", "Open the C++ Template Generator settings"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings"),
				FUIAction(FExecuteAction::CreateLambda([this]()
				{
					OpenPluginSettings();
				})));

			SubSection.AddSeparator("CppTemplateGeneratorSettingsSeparator");

			for (const TSubclassOf<UObject> TemplateClass : Settings->TemplateClasses)
			{
				if (!IsValidTemplateClass(TemplateClass.Get()))
				{
					if (IsValid(TemplateClass))
					{
						UE_LOG(
							CppTemplateGeneratorLog,
							Warning,
							TEXT("Skipping invalid template class '%s'. Template classes must be native Actor or ActorComponent classes."),
							*TemplateClass->GetPathName());
					}

					continue;
				}

				const FText Label = FText::FromString(TemplateClass->GetName());

				SubSection.AddMenuEntry(
					*TemplateClass->GetName(),
					TAttribute<FText>::CreateLambda([Label]() { return Label; }),
					TAttribute<FText>::CreateLambda([]() { return LOCTEXT("CreateTemplateTooltip", "Creates a new C++ class from this template"); }),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.AddCodeToProject"),
					FUIAction(FExecuteAction::CreateLambda([this, TemplateClass]() { OpenCreateTemplateForClass(TemplateClass); })));
			}
		}),
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.AddCodeToProject"));
}

void FCppTemplateGeneratorModule::OpenCreateTemplateForClass(UClass* ParentClass)
{
	if (!IsValidTemplateClass(ParentClass))
	{
		UE_LOG(
			CppTemplateGeneratorLog,
			Warning,
			TEXT("OpenCreateTemplateForClass called with invalid ParentClass '%s'. Only native Actor or ActorComponent classes are supported."),
			ParentClass ? *ParentClass->GetPathName() : TEXT("<null>"));
		return;
	}

	FAddToProjectConfig Config;
	Config.ParentClass(ParentClass);
	Config.ParentWindow(FGlobalTabmanager::Get()->GetRootWindow());

	FGameProjectGenerationModule::Get().OpenAddCodeToProjectDialog(Config);
}

void FCppTemplateGeneratorModule::OpenPluginSettings() const
{
	ISettingsModule* const SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule == nullptr)
	{
		UE_LOG(CppTemplateGeneratorLog, Warning, TEXT("Unable to open settings because the Settings module is not loaded."));
		return;
	}

	const UCppTemplateGeneratorSettings* const Settings = GetDefault<UCppTemplateGeneratorSettings>();
	if (!IsValid(Settings))
	{
		UE_LOG(CppTemplateGeneratorLog, Warning, TEXT("Unable to open settings because the CppTemplateGenerator settings object is invalid."));
		return;
	}

	SettingsModule->ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
}

bool FCppTemplateGeneratorModule::IsValidTemplateClass(const UClass* TemplateClass) const
{
	if (!IsValid(TemplateClass))
	{
		return false;
	}

	if (!TemplateClass->HasAnyClassFlags(CLASS_Native))
	{
		return false;
	}

	const bool bIsSupportedBaseClass =
		TemplateClass->IsChildOf(AActor::StaticClass()) ||
		TemplateClass->IsChildOf(UActorComponent::StaticClass());

	if (!bIsSupportedBaseClass)
	{
		return false;
	}

	if (TemplateClass->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCppTemplateGeneratorModule, CppTemplateGenerator)
