// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/LongFunctionValidator.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "BlueprintEditorModule.h"
#include "Misc/DataValidation.h"
#include "Library/BPUtilsNodeFunctionLibrary.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

ULongFunctionValidator::ULongFunctionValidator()
{
}

EDataValidationResult ULongFunctionValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	constexpr int32 NodeLimit = 200;
	bIsError = false;

	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{

		TArray<UEdGraph*> AllGraphs = Blueprint->UbergraphPages;
		AllGraphs.Append(Blueprint->FunctionGraphs);
		AllGraphs.Append(Blueprint->MacroGraphs);
		AllGraphs.Append(Blueprint->DelegateSignatureGraphs);
		AllGraphs.Append(Blueprint->IntermediateGeneratedGraphs);

		for(UEdGraph* Graph : AllGraphs)
		{
			if(!Graph) continue;

			int32 NodeCount = 0;
			for(UEdGraphNode* Node : Graph->Nodes)
			{
				if(Node && !Node->IsA<UK2Node_FunctionEntry>() && !Node->IsA<UK2Node_FunctionResult>())
				{
					NodeCount++;
				}
			}

			if(NodeCount > NodeLimit)
			{
				const FString GraphType = UBPUtilsNodeFunctionLibrary::GetGraphType(Blueprint, Graph);
				const FText MessageText = FText::Format(
					INVTEXT("'{0}' - '{1}' contains {2} nodes, which exceeds the recommended limit of {3}. Consider splitting it into smaller functions."),
					FText::FromString(GraphType),
					FText::FromString(Graph->GetName()),
					FText::AsNumber(NodeCount),
					FText::AsNumber(NodeLimit)
				);
				const FText JumpText = FText::Format(INVTEXT("Jump to '{0}' - {1}"), FText::FromString(Graph->GetName()), FText::FromString(GraphType));

				TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
				ValidatorX::Actions::AddJumpToGraphAction(Message, JumpText, Blueprint, Graph);
				bIsError = true;
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
