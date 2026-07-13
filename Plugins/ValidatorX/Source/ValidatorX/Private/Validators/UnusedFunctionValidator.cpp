// Copyright (c) 2026 DimAlek. All Rights Reserved.


#include "Validators/UnusedFunctionValidator.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintEditor.h"
#include "Misc/DataValidation.h"
#include "SMyBlueprint.h"
#include "Library/BPUtilsNodeFunctionLibrary.h"
#include "Validation/BlueprintValidatorActionHelpers.h"


UUnusedFunctionValidator::UUnusedFunctionValidator()
{
}

EDataValidationResult UUnusedFunctionValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);

		for(UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
		{
			if(!FunctionGraph) continue;

			const FName FunctionName = FunctionGraph->GetFName();
			if(FunctionName == UEdGraphSchema_K2::FN_UserConstructionScript) continue;

			bool bIsFunctionUsed = false;

			// 1. Search in this blueprint
			for(UEdGraph* Graph : AllGraphs)
			{
				if (!Graph || Graph == FunctionGraph)
				{
					continue;
				}

				for(UEdGraphNode* const Node : Graph->Nodes)
				{
					if(const UK2Node_CallFunction* const CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
					{
						if(CallFunctionNode->FunctionReference.GetMemberName() == FunctionName)
						{
							bIsFunctionUsed = true;
							break;
						}
					}
				}
				if(bIsFunctionUsed) 
				{
					break;
				}
			}

			// 2. Search in child blueprints 
			if(!bIsFunctionUsed && Blueprint->GeneratedClass)
			{
				TArray<UClass*> DerivedClasses;
				UBPUtilsNodeFunctionLibrary::GetAllDerivedBlueprintClasses(Blueprint->GeneratedClass, DerivedClasses, true);

				for(const UClass* const ChildClass : DerivedClasses)
				{
					UBlueprint* ChildBP = Cast<UBlueprint>(ChildClass->ClassGeneratedBy);
					if (!IsValid(ChildBP))
					{
						continue;
					}

					TArray<UEdGraph*> ChildGraphs;
					ChildBP->GetAllGraphs(ChildGraphs);

					for(const UEdGraph* const Graph : ChildGraphs)
					{
						for(const UEdGraphNode* const Node : Graph->Nodes)
						{
							if(const UK2Node_CallFunction* const CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
							{
								if(CallFunctionNode->FunctionReference.GetMemberName() == FunctionName)
								{
									bIsFunctionUsed = true;
									break;
								}
							}
						}
						if(bIsFunctionUsed) break;
					}
					if(bIsFunctionUsed) break;
				}
			}

			if(!bIsFunctionUsed)
			{
				const FText MessageText = FText::Format(
					INVTEXT("Function '{0}' in Blueprint '{1}' is never used."),
					FText::FromName(FunctionName),
					FText::FromString(Blueprint->GetName())
				);

				const TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);

				const FText JumpToFunctionText = FText::Format(
					INVTEXT("Jump to Function - '{0}'"),
					FText::FromName(FunctionName));

				ValidatorX::Actions::AddJumpToGraphItemAction(Message, JumpToFunctionText, Blueprint, FunctionGraph, FunctionGraph->GetFName());

				const FText DeleteFunctionText = FText::Format(
					INVTEXT("'Fix' - Delete Function - '{0}'"),
					FText::FromName(FunctionName));

				ValidatorX::Actions::AddAction(Message, DeleteFunctionText,
					FSimpleDelegate::CreateLambda([=]
						{
							if(Blueprint && FunctionGraph)
							{
								if(ValidatorX::Actions::OpenAsset(Blueprint))
								{
									FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=] (float DeltaTime)
										{
											if(FBlueprintEditor* BlueprintEditor = ValidatorX::Actions::FindBlueprintEditor(Blueprint, false))
											{
												const FText ConfirmText = FText::Format(
													INVTEXT("Are you sure you want to delete the unused Function '{0}' from Blueprint '{1}'?"),
													FText::FromName(FunctionName),
													FText::FromString(Blueprint->GetName())
												);

												if(FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
												{
													Blueprint->Modify();

													Blueprint->FunctionGraphs.Remove(FunctionGraph);
													FunctionGraph->Modify();
													FunctionGraph->MarkAsGarbage();

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
