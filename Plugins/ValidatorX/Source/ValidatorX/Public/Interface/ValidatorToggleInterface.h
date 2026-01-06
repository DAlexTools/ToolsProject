// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ValidatorToggleInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UValidatorToggleInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief Interface for validators that can be toggled on or off.
 *
 * Classes implementing this interface must provide functionality to enable,
 * disable, or toggle their validation state.
 */
class VALIDATORX_API IValidatorToggleInterface
{
	GENERATED_BODY()

public:
	/**
	 * @brief Toggles the enabled state of this validator.
	 *
	 * If the validator is currently enabled, it will be disabled.
	 * If it is currently disabled, it will be enabled.
	 */
	virtual void ToggleValidationEnabled() = 0;

	/**
	 * @brief Sets the enabled state of this validator.
	 *
	 * @param bEnabled True to enable validation, false to disable it.
	 */
	virtual void SetValidationEnabled(bool bEnabled) = 0;
};
