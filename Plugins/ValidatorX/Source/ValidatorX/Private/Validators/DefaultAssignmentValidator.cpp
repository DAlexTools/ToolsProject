// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/DefaultAssignmentValidator.h"
#include "K2Node_VariableSet.h"
#include "Misc/DataValidation.h"
#include "BlueprintEditor.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UDefaultAssignmentValidator::UDefaultAssignmentValidator()
{
}

EDataValidationResult UDefaultAssignmentValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);

		for(UEdGraph* Graph : AllGraphs)
		{
			for(UEdGraphNode* Node : Graph->Nodes)
			{
				if(UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node))
				{
					const FName VarName = VarSetNode->GetVarName();
					const FProperty* Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
					if(!Property)
					{
						continue;
					}

					if(UEdGraphPin* ValuePin = VarSetNode->FindPin(VarName))
					{
						if(!ValuePin->HasAnyConnections())
						{
							const FString PinDefaultValue = ValuePin->DefaultValue;

							FString PropertyDefaultValue;

							if(const auto DefaultObjectPtr = Blueprint->GeneratedClass->GetDefaultObject(false))
							{
								FString Temp;
								Property->ExportText_InContainer(0, Temp, DefaultObjectPtr, DefaultObjectPtr, nullptr, PPF_None);
								PropertyDefaultValue = Temp;
							}

							if(PinDefaultValue == PropertyDefaultValue)
							{
								const FText MessageText = FText::Format(
									INVTEXT("Redundant assignment detected: variable '{0}' in Blueprint '{1}' is assigned its default value."),
									FText::FromString(Graph->GetName()),
									FText::FromString(Blueprint->GetName())
								);

								TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
								ValidatorX::Actions::AddJumpToNodeAction(Message, FText::FromString("Jump to Node"), Blueprint, Graph, VarSetNode);


								bIsError = true;



							}
						}
					}
				}
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
