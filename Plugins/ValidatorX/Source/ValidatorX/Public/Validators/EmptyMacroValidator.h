// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseClasses/BlueprintValidatorBase.h"
#include "EmptyMacroValidator.generated.h"

/**
 * 
 */
UCLASS()
class VALIDATORX_API UEmptyMacroValidator : public UBlueprintValidatorBase
{
	GENERATED_BODY()

public:
	UEmptyMacroValidator();

	/**
	 * Performs validation on a loaded asset.
	 *
	 * @param InAssetData   Asset metadata
	 * @param InAsset       Loaded asset object
	 * @param Context       Validation context for reporting issues
	 * @return EDataValidationResult::Passed if valid, Failed/Invalid otherwise
	 */
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

};
