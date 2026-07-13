// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/DebugCallValidator.h"
#include "K2Node_CallFunction.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

namespace
{
	bool IsDebugFunctionName(const FName FunctionName)
	{
		const FString FunctionNameString = FunctionName.ToString();
		return FunctionName == TEXT("PrintString")
			|| FunctionName == TEXT("PrintText")
			|| FunctionNameString.StartsWith(TEXT("DrawDebug"));
	}
}

UDebugCallValidator::UDebugCallValidator()
{
}

EDataValidationResult UDebugCallValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
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
				UK2Node_CallFunction* CallFunction = Cast<UK2Node_CallFunction>(Node);
				if(!CallFunction)
				{
					continue;
				}

				const FName FunctionName = CallFunction->GetFunctionName();
				if(!IsDebugFunctionName(FunctionName))
				{
					continue;
				}

				const FText MessageText = FText::Format(
					INVTEXT("Debug call '{0}' found in Blueprint '{1}' graph '{2}'. Remove or gate debug-only calls before production."),
					FText::FromName(FunctionName),
					FText::FromString(Blueprint->GetName()),
					FText::FromString(Graph->GetName())
				);

				TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
				ValidatorX::Actions::AddJumpToNodeAction(Message, FText::FromString("Jump to Debug Call"), Blueprint, Graph, CallFunction);

				bIsError = true;
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
