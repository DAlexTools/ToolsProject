// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/UnhandledCastFailureValidator.h"

#include "K2Node_DynamicCast.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UUnhandledCastFailureValidator::UUnhandledCastFailureValidator()
{
}

EDataValidationResult UUnhandledCastFailureValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);

		for(UEdGraph* Graph : AllGraphs)
		{
			if(!Graph)
			{
				continue;
			}

			for(UEdGraphNode* Node : Graph->Nodes)
			{
				UK2Node_DynamicCast* DynamicCast = Cast<UK2Node_DynamicCast>(Node);
				if(!DynamicCast)
				{
					continue;
				}

				const UEdGraphPin* SuccessPin = DynamicCast->GetValidCastPin();
				const UEdGraphPin* FailurePin = DynamicCast->GetInvalidCastPin();

				const bool bHasSuccessPath = SuccessPin && !SuccessPin->LinkedTo.IsEmpty();
				const bool bMissingFailurePath = FailurePin && FailurePin->LinkedTo.IsEmpty();
				if(!bHasSuccessPath || !bMissingFailurePath)
				{
					continue;
				}

				const FText MessageText = FText::Format(
					INVTEXT("Dynamic cast in Blueprint '{0}' graph '{1}' has no Cast Failed path. Handle failure explicitly or use a validated reference flow."),
					FText::FromString(Blueprint->GetName()),
					FText::FromString(Graph->GetName())
				);

				TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
				ValidatorX::Actions::AddJumpToNodeAction(Message, FText::FromString("Jump to Dynamic Cast"), Blueprint, Graph, DynamicCast);

				bIsError = true;
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
