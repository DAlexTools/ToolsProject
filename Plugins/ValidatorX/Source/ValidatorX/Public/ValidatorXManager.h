// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseClasses/ValidatorXBase.h"

/**
 * 
 */
class VALIDATORX_API FValidatorXManager
{
	FValidatorXManager() {}
	FValidatorXManager(const FValidatorXManager&) = delete;
	FValidatorXManager& operator=(const FValidatorXManager&) = delete;

public:
	static FValidatorXManager& Get()
	{
		static FValidatorXManager Instance;
		return Instance;
	}

	void RegisterValidator(UValidatorXBase* Validator);
	void SetAllValidatorsEnabled(bool bEnabled);

	const TArray<TWeakObjectPtr<UValidatorXBase>>& GetValidators();

private:
	void CompactValidators();

	TArray<TWeakObjectPtr<UValidatorXBase>> Validators;

};
