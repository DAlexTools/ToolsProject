// Copyright (c) 2026 DimAlek. All Rights Reserved.


#include "Validators/GlobalVariableNeverUsedValidator.h"
#include "K2Node_VariableSet.h"
#include "K2Node_VariableGet.h"
#include "Misc/DataValidation.h"
#include "BlueprintEditor.h"
#include "SMyBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UGlobalVariableNeverUsedValidator::UGlobalVariableNeverUsedValidator()
{
}

EDataValidationResult UGlobalVariableNeverUsedValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		const TArray<FBPVariableDescription>& Variables = Blueprint->NewVariables;

		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);

		for(const FBPVariableDescription& VarDesc : Variables)
		{
			bool bUsed = false;

			// Use case 1: Exposed on spawn or Config flags
			if(UClass* GenClass = Blueprint->GeneratedClass)
			{
				if(FProperty* Prop = GenClass->FindPropertyByName(VarDesc.VarName))
				{
					if(Prop->HasAnyPropertyFlags(CPF_ExposeOnSpawn | CPF_Config | CPF_Interp))
					{
						bUsed = true;
					}
				}
			}

			// Use case 2: Explicitly used in graphs
			if(!bUsed)
			{
				for(UEdGraph* Graph : AllGraphs)
				{
					if(!Graph) continue;

					for(UEdGraphNode* Node : Graph->Nodes)
					{
						if(const UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
						{
							if(VarGet->GetVarName() == VarDesc.VarName)
							{
								bUsed = true;
								break;
							}
						}
						else if(const UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
						{
							if(VarSet->GetVarName() == VarDesc.VarName)
							{
								bUsed = true;
								break;
							}
						}
					}

					if(bUsed) break;
				}
			}

			// Unused variable
			if(!bUsed)
			{
				const FText MessageText = FText::Format(
					INVTEXT("Variable '{0}' in Blueprint '{1}' is never used."),
					FText::FromName(VarDesc.VarName),
					FText::FromString(Blueprint->GetName())
				);

				const TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);

				// Jump to variable
				ValidatorX::Actions::AddJumpToBlueprintItemAction(
					Message,
					FText::Format(INVTEXT("Jump to Variable - '{0}'"), FText::FromName(VarDesc.VarName)),
					Blueprint,
					VarDesc.VarName);

				// Fix: delete variable
				ValidatorX::Actions::AddAction(
					Message,
					FText::Format(INVTEXT("Fix - Delete Variable - '{0}'"), FText::FromName(VarDesc.VarName)),
					FSimpleDelegate::CreateLambda([=]
						{
							if(Blueprint)
							{
								if(ValidatorX::Actions::OpenAsset(Blueprint))
								{
									FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=] (float)
									{
										if(FBlueprintEditor* BlueprintEditor = ValidatorX::Actions::FindBlueprintEditor(Blueprint, false))
										{
											const FText ConfirmText = FText::Format(
												INVTEXT("Are you sure you want to delete variable '{0}' from Blueprint '{1}'?"),
												FText::FromName(VarDesc.VarName),
												FText::FromString(Blueprint->GetName())
											);

											if(FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
											{
												FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, VarDesc.VarName);
												FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
											}
										}
										return false;
									}));
								}
							}
						}));

				bIsError = true;
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
