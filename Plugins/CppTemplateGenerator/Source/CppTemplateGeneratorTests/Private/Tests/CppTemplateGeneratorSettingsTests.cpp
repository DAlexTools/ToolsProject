// Copyright 2025 DimAlek. All Rights Reserved.

#include "CppTemplateGeneratorSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCppTemplateGeneratorSanitizeTemplateClassesRemovesInvalidAndDuplicateEntriesTest,
	"CppTemplateGenerator.Settings.SanitizeTemplateClasses.RemovesInvalidAndDuplicateEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCppTemplateGeneratorSanitizeTemplateClassesRemovesInvalidAndDuplicateEntriesTest::RunTest(const FString& Parameters)
{
	UCppTemplateGeneratorSettings* const Settings = NewObject<UCppTemplateGeneratorSettings>(GetTransientPackage());
	TestNotNull(TEXT("Transient settings object should be created"), Settings);
	if (!Settings)
	{
		return false;
	}

	Settings->TemplateClasses = 
	{
		AActor::StaticClass(),
		nullptr,
		APawn::StaticClass(),
		AActor::StaticClass(),
		UObject::StaticClass()
	};

	const bool bWasModified = Settings->SanitizeTemplateClasses();

	TestTrue(TEXT("SanitizeTemplateClasses should report modifications when invalid or duplicate classes exist"), bWasModified);
	TestEqual(TEXT("Only supported unique classes should remain"), Settings->TemplateClasses.Num(), 2);
	TestEqual(TEXT("First valid class should be preserved"), Settings->TemplateClasses[0].Get(), AActor::StaticClass());
	TestEqual(TEXT("Second valid class should be preserved"), Settings->TemplateClasses[1].Get(), APawn::StaticClass());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCppTemplateGeneratorSanitizeTemplateClassesKeepsValidUniqueEntriesTest,
	"CppTemplateGenerator.Settings.SanitizeTemplateClasses.KeepsValidUniqueEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCppTemplateGeneratorSanitizeTemplateClassesKeepsValidUniqueEntriesTest::RunTest(const FString& Parameters)
{
	UCppTemplateGeneratorSettings* const Settings = NewObject<UCppTemplateGeneratorSettings>(GetTransientPackage());
	TestNotNull(TEXT("Transient settings object should be created"), Settings);
	if (!Settings)
	{
		return false;
	}

	Settings->TemplateClasses = 
	{
		AActor::StaticClass(),
		UActorComponent::StaticClass(),
		ACharacter::StaticClass()
	};

	const bool bWasModified = Settings->SanitizeTemplateClasses();

	TestFalse(TEXT("SanitizeTemplateClasses should not report modifications for already valid unique classes"), bWasModified);
	TestEqual(TEXT("Valid unique classes should be preserved"), Settings->TemplateClasses.Num(), 3);
	TestEqual(TEXT("Actor entry should remain unchanged"), Settings->TemplateClasses[0].Get(), AActor::StaticClass());
	TestEqual(TEXT("Component entry should remain unchanged"), Settings->TemplateClasses[1].Get(), UActorComponent::StaticClass());
	TestEqual(TEXT("Character entry should remain unchanged"), Settings->TemplateClasses[2].Get(), ACharacter::StaticClass());

	return true;
}

#endif
