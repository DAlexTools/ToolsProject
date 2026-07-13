// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/TickUsageValidator.h"

#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

namespace
{
	bool IsTickEvent(const UK2Node_Event* EventNode)
	{
		if(!EventNode)
		{
			return false;
		}

		const FName FunctionName = EventNode->GetFunctionName();
		return FunctionName == TEXT("ReceiveTick") || FunctionName == TEXT("ReceiveTickAI");
	}
}

UTickUsageValidator::UTickUsageValidator()
{
}

EDataValidationResult UTickUsageValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
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
				UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
				if(!IsTickEvent(EventNode))
				{
					continue;
				}

				const UEdGraphPin* ThenPin = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
				if(!ThenPin || ThenPin->LinkedTo.IsEmpty())
				{
					continue;
				}

				const FText MessageText = FText::Format(
					INVTEXT("Event Tick in Blueprint '{0}' graph '{1}' is connected. Prefer timers, events, or explicit invalidation unless per-frame work is required."),
					FText::FromString(Blueprint->GetName()),
					FText::FromString(Graph->GetName())
				);

				TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
				ValidatorX::Actions::AddJumpToNodeAction(Message, FText::FromString("Jump to Event Tick"), Blueprint, Graph, EventNode);

				bIsError = true;
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
