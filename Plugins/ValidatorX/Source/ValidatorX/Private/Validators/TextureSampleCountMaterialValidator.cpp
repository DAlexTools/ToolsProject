// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/TextureSampleCountMaterialValidator.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

namespace
{
	constexpr int32 TextureSampleLimit = 16;
}

UTextureSampleCountMaterialValidator::UTextureSampleCountMaterialValidator()
{
}

EDataValidationResult UTextureSampleCountMaterialValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	UMaterial* Material = Cast<UMaterial>(InAsset);
	if(!Material)
	{
		return EDataValidationResult::Valid;
	}

	TArray<const UMaterialExpressionTextureSample*> TextureSamples;
	Material->GetAllExpressionsOfType(TextureSamples);

	if(TextureSamples.Num() > TextureSampleLimit)
	{
		const FText MessageText = FText::Format(
			INVTEXT("Material '{0}' uses {1} texture samples, which exceeds the recommended limit of {2}. Consider channel packing, shared samplers, or simplifying the graph."),
			FText::FromString(Material->GetName()),
			FText::AsNumber(TextureSamples.Num()),
			FText::AsNumber(TextureSampleLimit)
		);

		TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
		ValidatorX::Actions::AddOpenAssetAction(Message, FText::FromString("Open Material"), Material);
		bIsError = true;
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
