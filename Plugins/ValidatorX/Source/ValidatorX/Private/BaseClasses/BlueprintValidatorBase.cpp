// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseClasses/BlueprintValidatorBase.h"

#include "Engine/Blueprint.h"

bool UBlueprintValidatorBase::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const
{
	return InAsset && InAsset->IsA<UBlueprint>();
}
