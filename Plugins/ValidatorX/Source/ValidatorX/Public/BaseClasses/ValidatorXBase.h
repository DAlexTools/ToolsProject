// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorBase.h"
#include "Interface/ValidatorToggleInterface.h"
#include "ValidatorXBase.generated.h"

UCLASS(Abstract)
class VALIDATORX_API UValidatorXBase : public UEditorValidatorBase, public IValidatorToggleInterface
{
	GENERATED_BODY()

public:
	UValidatorXBase();

	virtual FString GetTypeValidator() const
	{
		return TEXT("Asset");
	}

	virtual bool IsEnabled() const override;

#pragma region IValidatorToggleInterface
	virtual void ToggleValidationEnabled() override;
	virtual void SetValidationEnabled(bool bEnabled) override;
#pragma endregion

protected:
	bool bIsError = false;
};
