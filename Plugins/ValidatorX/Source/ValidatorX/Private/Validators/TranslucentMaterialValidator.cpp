// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/TranslucentMaterialValidator.h"

#include "Materials/Material.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

namespace
{
	FString GetBlendModeName(EBlendMode BlendMode)
	{
		switch(BlendMode)
		{
		case BLEND_Opaque:
			return TEXT("Opaque");
		case BLEND_Masked:
			return TEXT("Masked");
		case BLEND_Translucent:
			return TEXT("Translucent");
		case BLEND_Additive:
			return TEXT("Additive");
		case BLEND_Modulate:
			return TEXT("Modulate");
		case BLEND_AlphaComposite:
			return TEXT("AlphaComposite");
		case BLEND_AlphaHoldout:
			return TEXT("AlphaHoldout");
		default:
			return TEXT("Unknown");
		}
	}
}

UTranslucentMaterialValidator::UTranslucentMaterialValidator()
{
}

EDataValidationResult UTranslucentMaterialValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	UMaterial* Material = Cast<UMaterial>(InAsset);
	if(Material && Material->GetBlendMode() != BLEND_Opaque && Material->GetBlendMode() != BLEND_Masked)
	{
		const FText MessageText = FText::Format(
			INVTEXT("Material '{0}' uses blend mode '{1}'. Review translucent/additive materials for overdraw and sorting cost."),
			FText::FromString(Material->GetName()),
			FText::FromString(GetBlendModeName(Material->GetBlendMode()))
		);

		TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
		ValidatorX::Actions::AddOpenAssetAction(Message, FText::FromString("Open Material"), Material);
		bIsError = true;
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
