// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/EmptyBranchValidator.h"
#include "K2Node_IfThenElse.h"
#include "Misc/DataValidation.h"
#include "BlueprintEditorModule.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UEmptyBranchValidator::UEmptyBranchValidator()
{
}

EDataValidationResult UEmptyBranchValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);

		for(UEdGraph* Graph : AllGraphs)
		{
			if (!Graph)
			{
				continue;
			}

			for(UEdGraphNode* Node : Graph->Nodes)
			{
				if(UK2Node_IfThenElse* Branch = Cast<UK2Node_IfThenElse>(Node))
				{
					const UEdGraphPin* ThenPin = Branch->GetThenPin();
					const UEdGraphPin* ElsePin = Branch->GetElsePin();

					const bool bThenUnconnected = ThenPin && ThenPin->LinkedTo.Num() == 0;
					const bool bElseUnconnected = ElsePin && ElsePin->LinkedTo.Num() == 0;

					// Only if BOTH branches are not connected
					if(bThenUnconnected && bElseUnconnected)
					{
						const FText MessageText = FText::Format(
							INVTEXT("Branch node in graph '{0}' has both 'Then' and 'Else' execution pins unconnected."),
							FText::FromString(Graph->GetName())
						);

						TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
						ValidatorX::Actions::AddJumpToNodeAction(Message, FText::FromString("Jump to Branch"), Blueprint, Graph, Branch);

						bIsError = true;
					}
				}
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

