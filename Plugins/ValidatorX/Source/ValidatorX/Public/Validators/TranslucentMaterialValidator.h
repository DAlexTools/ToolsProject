// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseClasses/MaterialValidatorBase.h"
#include "TranslucentMaterialValidator.generated.h"

UCLASS()
class VALIDATORX_API UTranslucentMaterialValidator : public UMaterialValidatorBase
{
	GENERATED_BODY()

public:
	UTranslucentMaterialValidator();

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
};
