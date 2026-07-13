// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/TwoSidedMaterialValidator.h"

#include "Materials/Material.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UTwoSidedMaterialValidator::UTwoSidedMaterialValidator()
{
}

EDataValidationResult UTwoSidedMaterialValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	UMaterial* Material = Cast<UMaterial>(InAsset);
	if(Material && Material->IsTwoSided())
	{
		const FText MessageText = FText::Format(
			INVTEXT("Material '{0}' is two-sided. Review whether double-sided shading is required for this asset."),
			FText::FromString(Material->GetName())
		);

		TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
		ValidatorX::Actions::AddOpenAssetAction(Message, FText::FromString("Open Material"), Material);
		bIsError = true;
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
