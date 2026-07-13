// Copyright (c) 2026 DimAlek. All Rights Reserved.


#include "Validators/EmptyMacroValidator.h"
#include "K2Node_Tunnel.h"
#include "K2Node_MacroInstance.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintEditor.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UEmptyMacroValidator::UEmptyMacroValidator()
{
}

EDataValidationResult UEmptyMacroValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		for(UEdGraph* MacroGraph : Blueprint->MacroGraphs)
		{
			if(!MacroGraph) continue;

			int32 UsefulNodeCount = 0;
			for(UEdGraphNode* Node : MacroGraph->Nodes)
			{
				if(!Node) continue;

				if(Node->IsA<UK2Node_Tunnel>())
				{
					continue;
				}

				UsefulNodeCount++;
			}

			if(UsefulNodeCount == 0)
			{
				const FText MessageText = FText::Format(
					INVTEXT("Macro '{0}' in Blueprint '{1}' is empty."),
					FText::FromString(MacroGraph->GetName()),
					FText::FromString(Blueprint->GetName()));

				const TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);

				const FText JumpToMacroText = FText::Format(
					INVTEXT("Jump to Macro - '{0}'"),
					FText::FromString(MacroGraph->GetName()));

				ValidatorX::Actions::AddJumpToGraphAction(Message, JumpToMacroText, Blueprint, MacroGraph);

				const FText DeleteMacroText = FText::Format(
					INVTEXT("'Fix' - Delete Macro - '{0}'"),
					FText::FromString(MacroGraph->GetName()));

				ValidatorX::Actions::AddAction(Message, DeleteMacroText,
					FSimpleDelegate::CreateLambda([=]
						{
							if(Blueprint && MacroGraph)
							{
								if(ValidatorX::Actions::OpenAsset(Blueprint))
								{
									FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=] (float DeltaTime)
										{
											if(FBlueprintEditor* BlueprintEditor = ValidatorX::Actions::FindBlueprintEditor(Blueprint, /*bFocusIfOpen=*/false))
											{
												const FText ConfirmText = FText::Format(
													INVTEXT("Are you sure you want to delete the Macro '{0}' from Blueprint '{1}'?"),
													FText::FromString(MacroGraph->GetName()),
													FText::FromString(Blueprint->GetName())
												);

												if(FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
												{
													Blueprint->Modify();
													auto RemoveMacroInstances = [=] (TArray<TObjectPtr<UEdGraph>>& Graphs)
													{
														for(UEdGraph* Graph : Graphs)
														{
															if(!Graph) continue;

															TArray<UK2Node_MacroInstance*> MacroInstanceNodes;
															Graph->GetNodesOfClass<UK2Node_MacroInstance>(MacroInstanceNodes);

															for(UK2Node_MacroInstance* MacroInstanceNode : MacroInstanceNodes)
															{
																if(MacroInstanceNode && MacroInstanceNode->GetMacroGraph() == MacroGraph)
																{
																	Graph->Modify();
																	MacroInstanceNode->DestroyNode();
																}
															}
														}
													};


													RemoveMacroInstances(Blueprint->UbergraphPages);
													RemoveMacroInstances(Blueprint->FunctionGraphs);
													RemoveMacroInstances(Blueprint->DelegateSignatureGraphs);
													RemoveMacroInstances(Blueprint->IntermediateGeneratedGraphs);

													Blueprint->MacroGraphs.Remove(MacroGraph);
													MacroGraph->Modify();
													MacroGraph->MarkAsGarbage();

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
