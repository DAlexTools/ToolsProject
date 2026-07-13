// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/MaterialPositionOffsetValidator.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UMaterialPositionOffsetValidator::UMaterialPositionOffsetValidator()
{
}

EDataValidationResult UMaterialPositionOffsetValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	UMaterial* Material = Cast<UMaterial>(InAsset);
	if(!Material)
	{
		return EDataValidationResult::Valid;
	}

	const FExpressionInput* WorldPositionOffset = Material->GetExpressionInputForProperty(MP_WorldPositionOffset);
	const FExpressionInput* PixelDepthOffset = Material->GetExpressionInputForProperty(MP_PixelDepthOffset);

	const bool bUsesWorldPositionOffset = WorldPositionOffset && WorldPositionOffset->Expression;
	const bool bUsesPixelDepthOffset = PixelDepthOffset && PixelDepthOffset->Expression;

	if(bUsesWorldPositionOffset || bUsesPixelDepthOffset)
	{
		const FText MessageText = FText::Format(
			INVTEXT("Material '{0}' uses {1}{2}. Review bounds, depth behavior, and runtime cost."),
			FText::FromString(Material->GetName()),
			bUsesWorldPositionOffset ? INVTEXT("World Position Offset") : FText::GetEmpty(),
			bUsesWorldPositionOffset && bUsesPixelDepthOffset ? INVTEXT(" and Pixel Depth Offset") : (bUsesPixelDepthOffset ? INVTEXT("Pixel Depth Offset") : FText::GetEmpty())
		);

		TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
		ValidatorX::Actions::AddOpenAssetAction(Message, FText::FromString("Open Material"), Material);
		bIsError = true;
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
