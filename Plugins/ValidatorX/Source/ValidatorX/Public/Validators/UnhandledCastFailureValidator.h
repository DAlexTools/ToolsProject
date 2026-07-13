// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseClasses/BlueprintValidatorBase.h"
#include "UnhandledCastFailureValidator.generated.h"

UCLASS()
class VALIDATORX_API UUnhandledCastFailureValidator : public UBlueprintValidatorBase
{
	GENERATED_BODY()

public:
	UUnhandledCastFailureValidator();

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
};
