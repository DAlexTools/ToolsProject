// Copyright Epic Games, Inc. All Rights Reserved.

#include "CppTemplateGenerator.h"
#include "GameFramework/Actor.h"      
#include "Components/ActorComponent.h" 
#include "GameFramework/Pawn.h"       
#include "GameFramework/Character.h"   
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"     
#include "CppTemplateGeneratorSettings.h"

#define LOCTEXT_NAMESPACE "FCppTemplateGeneratorModule"

static const FName CppTemplateGeneratorTabName(TEXT("CppTemplateGenerator"));

void FCppTemplateGeneratorModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this,
			&FCppTemplateGeneratorModule::RegisterMenus));
}

void FCppTemplateGeneratorModule::ShutdownModule()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus* ToolMenus = UToolMenus::Get();
		if (ToolMenus)
		{
			ToolMenus->UnregisterOwner(this);
		}
	}
}

void FCppTemplateGeneratorModule::RegisterMenus()
{
	UToolMenu*		  Menu = UToolMenus::Get()->RegisterMenu("MainFrame.MainMenu.Tools");
	FToolMenuSection& Section = Menu->AddSection("Programming", LOCTEXT("ProgrammingHeading", "Programming"));


	const UCppTemplateGeneratorSettings* Settings = GetDefault<UCppTemplateGeneratorSettings>();
	if (!Settings)
	{
		return;
	}

	Section.AddSubMenu(
		"CreateCppTemplate",
		LOCTEXT("CreateCppTemplateLabel", "New C++ Template..."),
		LOCTEXT("CreateCppTemplateTooltip", "Create a C++ class from your predefined template"),
		FNewToolMenuDelegate::CreateLambda([this, Settings](UToolMenu* InMenu) {
			FToolMenuSection& SubSection = InMenu->AddSection("CppTemplateSection");

			for (TSubclassOf<UObject> TemplateClass : Settings->TemplateClasses)
			{
				if (!TemplateClass)
				{
					continue;
				}

				FText Label = FText::FromString(TemplateClass->GetName());

				SubSection.AddMenuEntry(
					*TemplateClass->GetName(),									  
					TAttribute<FText>::CreateLambda([Label]() { return Label; }),
					TAttribute<FText>::CreateLambda([]() { return LOCTEXT("CreateTemplateTooltip", "Creates a new C++ class from this template"); }),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.AddCodeToProject"),
					FUIAction(
						FExecuteAction::CreateLambda([this, TemplateClass]() {
							OpenCreateTemplateForClass(TemplateClass);
						})));
			}
		}),
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "MainFrame.AddCodeToProject"));
}

void FCppTemplateGeneratorModule::OpenCreateTemplateForClass(UClass* ParentClass)
{
	if (!ParentClass)
	{
		return;
	}

	FAddToProjectConfig Config;
	Config.ParentClass(ParentClass);
	Config.ParentWindow(FGlobalTabmanager::Get()->GetRootWindow());

	FGameProjectGenerationModule::Get().OpenAddCodeToProjectDialog(Config);
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCppTemplateGeneratorModule, CppTemplateGenerator)