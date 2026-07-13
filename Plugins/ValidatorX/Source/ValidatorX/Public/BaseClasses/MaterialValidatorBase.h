// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseClasses/ValidatorXBase.h"
#include "MaterialValidatorBase.generated.h"

UCLASS(Abstract)
class VALIDATORX_API UMaterialValidatorBase : public UValidatorXBase
{
	GENERATED_BODY()

public:
	virtual FString GetTypeValidator() const override
	{
		return TEXT("Material");
	}

	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const override;
};
