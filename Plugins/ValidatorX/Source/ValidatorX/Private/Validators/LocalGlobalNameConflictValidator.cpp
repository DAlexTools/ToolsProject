// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/LocalGlobalNameConflictValidator.h"
#include "K2Node_FunctionEntry.h"
#include "Misc/DataValidation.h"
#include "BlueprintEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SMyBlueprint.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

ULocalGlobalNameConflictValidator::ULocalGlobalNameConflictValidator()
{
}

EDataValidationResult ULocalGlobalNameConflictValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);

		for(UEdGraph* Graph : AllGraphs)
		{
			UK2Node_FunctionEntry* EntryNode = nullptr;
			for(UEdGraphNode* Node : Graph->Nodes)
			{
				if(UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
				{
					EntryNode = Entry;
					break;
				}
			}

			if(!EntryNode)
			{
				continue;
			}

			for(const FBPVariableDescription& LocalVar : EntryNode->LocalVariables)
			{
				for(const FBPVariableDescription& GlobalVar : Blueprint->NewVariables)
				{
					if(LocalVar.VarName == GlobalVar.VarName)
					{
						const FText MessageText = FText::Format(
							INVTEXT("Local variable '{0}' in function '{1}' has the same name as a global variable."),
							FText::FromName(LocalVar.VarName),
							FText::FromString(Graph->GetName()));

						const TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);

						const FText JumpToVariableText = FText::Format(
							INVTEXT("Jump to variable - '{0}'"),
							FText::FromName(LocalVar.VarName));

						ValidatorX::Actions::AddJumpToGraphItemAction(Message, JumpToVariableText, Blueprint, Graph, LocalVar.VarName);

						const FText DeleteVariableText = FText::Format(
							INVTEXT("'Fix' - Rename Local Variable - '{0}'"),
							FText::FromName(LocalVar.VarName));

						ValidatorX::Actions::AddAction(Message, DeleteVariableText,
							FSimpleDelegate::CreateLambda([=]
							{
								if(Blueprint && EntryNode)
								{
									if(ValidatorX::Actions::OpenAsset(Blueprint))
									{
										FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=] (float DeltaTime)
										{
											if(FBlueprintEditor* BlueprintEditor = ValidatorX::Actions::FindBlueprintEditor(Blueprint, /*bFocusIfOpen=*/false))
											{
												if(TSharedPtr<SGraphEditor> GraphEditor = BlueprintEditor->OpenGraphAndBringToFront(Graph, true))
												{
													if(BlueprintEditor->GetMyBlueprintWidget().IsValid())
													{
														int32 IndexToRename = INDEX_NONE;

														for(int32 i = 0; i < EntryNode->LocalVariables.Num(); ++i)
														{
															if(EntryNode->LocalVariables[i].VarName == LocalVar.VarName)
															{
																IndexToRename = i;
																break;
															}
														}

														if(IndexToRename != INDEX_NONE)
														{
															const FText ConfirmText = FText::Format(
																INVTEXT("Are you sure you want to rename the local variable '{0}' in function '{1}' to '{2}'?"),
																FText::FromName(LocalVar.VarName),
																FText::FromString(Graph->GetName()),
																FText::FromName(FName(*FString("Local") + LocalVar.VarName.ToString())));
															FName NewName = FName(*FString("Local") + LocalVar.VarName.ToString());
															if(FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
															{
																EntryNode->Modify();
																EntryNode->LocalVariables[IndexToRename].VarName = NewName;
																FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

																if(TSharedPtr<SMyBlueprint> MyBlueprintWidget = BlueprintEditor->GetMyBlueprintWidget())
																{
																	if(MyBlueprintWidget->SelectionAsLocalVar())
																	{
																		MyBlueprintWidget->SelectItemByName(NewName,
																			ESelectInfo::Direct,
																			INDEX_NONE,
																			false);
																	}
																}
															}
														}

														return false;
													}
												}
											}

											return true;
										}));
									}
								}
							}));

						bIsError = true;
					}
				}
			}
		}
	}

	return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
