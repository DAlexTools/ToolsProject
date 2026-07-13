// Copyright 2025 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Engine/DeveloperSettings.h"
#include "CppTemplateGeneratorSettings.generated.h"

struct FPropertyChangedEvent;

/**
 * @brief Editor settings for the C++ Template Generator plugin.
 *
 * Stores the list of template root classes exposed in the plugin submenu and
 * keeps that list sanitized before use.
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "C++ Template Generator"))
class CPPTEMPLATEGENERATOR_API UCppTemplateGeneratorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * @brief Initializes the default template class list.
	 */
	UCppTemplateGeneratorSettings();

	/**
	 * @brief Removes invalid and duplicate template classes from the settings.
	 *
	 * @return true if the array was modified; otherwise false.
	 */
	bool SanitizeTemplateClasses();

	/**
	 * @brief Sanitizes the template list after the settings object is initialized.
	 */
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	/**
	 * @brief Sanitizes the template list after editor-side property changes.
	 *
	 * Null placeholders are preserved during interactive editing so users can
	 * add a new slot before choosing a class.
	 *
	 * @param PropertyChangedEvent Description of the edited property.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/**
	 * @brief Template root classes shown in the submenu.
	 *
	 * Invalid and duplicate entries are sanitized before use, while null
	 * placeholders are allowed transiently during editor editing.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Templates", meta = (AllowedClasses = "Actor,ActorComponent,Pawn,Character,GameModeBase,HUD", DisplayName = "Template Classes"))
	TArray<TSubclassOf<UObject>> TemplateClasses;
};
