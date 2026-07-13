// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/BranchConditionValidator.h"

#include "K2Node_IfThenElse.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UBranchConditionValidator::UBranchConditionValidator()
{
}

EDataValidationResult UBranchConditionValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
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
				UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node);
				const UEdGraphPin* ConditionPin = Branch->GetConditionPin();
				if(!ConditionPin || !ConditionPin->LinkedTo.IsEmpty())
				{
					continue;
				}

				const FText MessageText = FText::Format(
					INVTEXT("Branch node in Blueprint '{0}' graph '{1}' has no condition input. It will use the node default value."),
					FText::FromString(Blueprint->GetName()),
					FText::FromString(Graph->GetName())
				);

				TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
				ValidatorX::Actions::AddJumpToNodeAction(Message, FText::FromString("Jump to Branch"), Blueprint, Graph, Branch);

				bIsError = true;
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
