// Fill out your copyright notice in the Description page of Project Settings.


#include "ValidatorXManager.h"

void FValidatorXManager::RegisterValidator(UValidatorXBase* Validator)
{
	if(!Validator)
	{
		return;
	}

	CompactValidators();

	const bool bAlreadyRegistered = Validators.ContainsByPredicate(
		[Validator] (const TWeakObjectPtr<UValidatorXBase>& RegisteredValidator)
		{
			return RegisteredValidator.Get() == Validator;
		});

	if(!bAlreadyRegistered)
	{
		Validators.Add(Validator);
	}
}

void FValidatorXManager::SetAllValidatorsEnabled(bool bEnabled)
{
	CompactValidators();

	for(const TWeakObjectPtr<UValidatorXBase>& Validator : Validators)
	{
		if(Validator.IsValid())
		{
			Validator->SetValidationEnabled(bEnabled);
		}
	}
}

const TArray<TWeakObjectPtr<UValidatorXBase>>& FValidatorXManager::GetValidators()
{
	CompactValidators();
	return Validators;
}

void FValidatorXManager::CompactValidators()
{
	Validators.RemoveAll(
		[] (const TWeakObjectPtr<UValidatorXBase>& Validator)
		{
			return !Validator.IsValid();
		});
}
