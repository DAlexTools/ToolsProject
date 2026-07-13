// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseClasses/ValidatorXBase.h"

UValidatorXBase::UValidatorXBase()
{
	bIsEnabled = true;
}

bool UValidatorXBase::IsEnabled() const
{
	const UValidatorXBase* CDO = GetClass()->GetDefaultObject<UValidatorXBase>();
	return CDO && CDO->bIsEnabled && !bIsConfigDisabled;
}

void UValidatorXBase::ToggleValidationEnabled()
{
	SetValidationEnabled(!IsEnabled());
}

void UValidatorXBase::SetValidationEnabled(bool bEnabled)
{
	if(bIsConfigDisabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("Validator is disabled by config!"));
		return;
	}

	if(UValidatorXBase* CDO = GetClass()->GetDefaultObject<UValidatorXBase>())
	{
		CDO->bIsEnabled = bEnabled;
		CDO->SaveConfig();
	}
}
