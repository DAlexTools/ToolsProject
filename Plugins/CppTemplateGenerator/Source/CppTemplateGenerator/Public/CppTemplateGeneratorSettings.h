// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "CppTemplateGeneratorSettings.generated.h"

/**
 * Developer settings для плагина CppTemplateGenerator
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "C++ Template Generator"))
class CPPTEMPLATEGENERATOR_API UCppTemplateGeneratorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, config, Category = "Templates", meta = (AllowedClasses = "Actor,ActorComponent,Pawn,Character,GameModeBase,HUD", DisplayName = "Template Classes"))
	TArray<TSubclassOf<UObject>> TemplateClasses = 
	{
		AActor::StaticClass(),
		UActorComponent::StaticClass(),
		APawn::StaticClass(),
		ACharacter::StaticClass(),
		AGameModeBase::StaticClass(),
		AHUD::StaticClass()
	};
};
