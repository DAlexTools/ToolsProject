// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseClasses/MaterialValidatorBase.h"

#include "Materials/Material.h"

bool UMaterialValidatorBase::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const
{
	return InAsset && InAsset->IsA<UMaterial>();
}
