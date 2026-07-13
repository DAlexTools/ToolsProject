// Copyright 2025 DimAlek. All Rights Reserved.

#include "CppTemplateGeneratorSettings.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

namespace
{
	bool IsSupportedTemplateClass(const UClass* TemplateClass)
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
			TemplateClass->IsChildOf(AActor::StaticClass()) || TemplateClass->IsChildOf(UActorComponent::StaticClass());

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

	bool SanitizeTemplateClassesArray(TArray<TSubclassOf<UObject>>& TemplateClasses, const bool bPreserveNullEntries)
	{
		TArray<TSubclassOf<UObject>> SanitizedTemplateClasses;
		SanitizedTemplateClasses.Reserve(TemplateClasses.Num());

		for (const TSubclassOf<UObject>& TemplateClass : TemplateClasses)
		{
			UClass* const Class = TemplateClass.Get();
			if (bPreserveNullEntries && Class == nullptr)
			{
				SanitizedTemplateClasses.Add(nullptr);
				continue;
			}

			if (!IsSupportedTemplateClass(Class))
			{
				continue;
			}

			SanitizedTemplateClasses.AddUnique(Class);
		}

		const bool bWasModified = SanitizedTemplateClasses != TemplateClasses;
		if (bWasModified)
		{
			TemplateClasses = MoveTemp(SanitizedTemplateClasses);
		}

		return bWasModified;
	}
} // namespace

/**
 * @brief Initializes the default template class list.
 */
UCppTemplateGeneratorSettings::UCppTemplateGeneratorSettings()
{
	TemplateClasses = 
	{
		AActor::StaticClass(),
		UActorComponent::StaticClass(),
		APawn::StaticClass(),
		ACharacter::StaticClass(),
		AGameModeBase::StaticClass(),
		AHUD::StaticClass()
	};
}

bool UCppTemplateGeneratorSettings::SanitizeTemplateClasses()
{
	return SanitizeTemplateClassesArray(TemplateClasses, false);
}

void UCppTemplateGeneratorSettings::PostInitProperties()
{
	Super::PostInitProperties();

	SanitizeTemplateClasses();
}

#if WITH_EDITOR
void UCppTemplateGeneratorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCppTemplateGeneratorSettings, TemplateClasses))
	{
		if (SanitizeTemplateClassesArray(TemplateClasses, true))
		{
			SaveConfig();
		}
	}
}
#endif
