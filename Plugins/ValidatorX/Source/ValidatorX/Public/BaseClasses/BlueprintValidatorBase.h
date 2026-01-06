// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "Interface/ValidatorToggleInterface.h"
#include "BlueprintValidatorBase.generated.h"

/**
 * @brief Base class for Blueprint validators in the editor.
 *
 * Inherits from `UEditorValidatorBase` and implements `IValidatorToggleInterface`.
 * Provides default behavior and properties for Blueprint validation, including
 * enable/disable state and error tracking.
 */
UCLASS(Abstract)
class VALIDATORX_API UBlueprintValidatorBase : public UEditorValidatorBase, public IValidatorToggleInterface
{
	GENERATED_BODY()

public:
	/**
	 * @brief Default constructor.
	 *
	 * Initializes the validator as enabled by default.
	 */
	UBlueprintValidatorBase()
	{
		bIsEnabled = true;
	}

	/**
	 * @brief Returns the type of this validator.
	 *
	 * @return A string describing the validator type ("Blueprint").
	 */
	virtual FString GetTypeValidator() const
	{
		return TEXT("Blueprint");
	}

	/**
	 * @brief Toggles the enabled state of this validator.
	 *
	 * Default implementation does nothing. Override in derived classes to provide functionality.
	 */
	virtual void ToggleValidationEnabled() override {}

	/**
	 * @brief Sets the enabled state of this validator.
	 *
	 * Default implementation does nothing. Override in derived classes to enable/disable validation.
	 *
	 * @param bEnabled True to enable validation, false to disable.
	 */
	virtual void SetValidationEnabled(bool bEnabled) override {}

public:
	/** @brief Whether this validator currently has an error. */
	bool bIsError = false;
};
