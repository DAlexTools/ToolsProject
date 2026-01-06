// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseClasses/BlueprintValidatorBase.h"

/**
 * @brief Singleton manager for Blueprint validators.
 *
 * Manages registration and storage of all `UBlueprintValidatorBase` instances.
 * Provides global access to registered validators through a singleton pattern.
 */
class VALIDATORX_API FValidatorXManager
{
private:
	/** @brief Private default constructor for singleton pattern. */
	FValidatorXManager() {}

	/** @brief Deleted copy constructor to prevent copying. */
	FValidatorXManager(const FValidatorXManager&) = delete;

	/** @brief Deleted copy assignment operator to prevent copying. */
	FValidatorXManager& operator=(const FValidatorXManager&) = delete;

public:
	/**
	 * @brief Returns the singleton instance of the manager.
	 *
	 * @return Reference to the single `FValidatorXManager` instance.
	 */
	static FValidatorXManager& Get()
	{
		static FValidatorXManager Instance;
		return Instance;
	}

	/**
	 * @brief Registers a validator with the manager.
	 *
	 * Adds the validator to the internal list if it is valid.
	 *
	 * @param Validator The validator to register. Must not be null.
	 */
	void RegisterValidator(UBlueprintValidatorBase* Validator)
	{
		if (Validator)
		{
			Validators.Add(Validator);
		}
	}

	/**
	 * @brief Returns all registered validators.
	 *
	 * @return A constant reference to the array of registered validators as weak pointers.
	 */
	const TArray<TWeakObjectPtr<UBlueprintValidatorBase>>& GetValidators() const
	{
		return Validators;
	}

private:
	/** @brief Array storing all registered validators as weak object pointers. */
	TArray<TWeakObjectPtr<UBlueprintValidatorBase>> Validators;
};
