// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/UnusedMacroValidator.h"
#include "K2Node_MacroInstance.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintEditor.h"
#include "Misc/DataValidation.h"
#include "SMyBlueprint.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UUnusedMacroValidator::UUnusedMacroValidator()
{
}

EDataValidationResult UUnusedMacroValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if (UBlueprint* const Blueprint = Cast<UBlueprint>(InAsset))
	{
		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);

		for(UEdGraph* const MacroGraph : Blueprint->MacroGraphs)
		{
			if(!MacroGraph) 
			{
				continue;
			}

			const FName MacroName = MacroGraph->GetFName();
			bool bIsMacroUsed = false;

			for (UEdGraph* const Graph : AllGraphs)
			{
				if (!Graph || Graph == MacroGraph)
				{
					continue;
				}

				for (UEdGraphNode* const Node : Graph->Nodes)
				{
					if (const UK2Node_MacroInstance* const MacroInstance = Cast<UK2Node_MacroInstance>(Node))
					{
						if (MacroInstance->GetMacroGraph() == MacroGraph)
						{
							bIsMacroUsed = true;
							break;
						}
					}
				}

				if (bIsMacroUsed)
				{
					break;
				}
			}


			if(!bIsMacroUsed)
			{
				const FText MessageText = FText::Format(
					INVTEXT("Macro '{0}' is never used."),
					FText::FromName(MacroName)
				);

				TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
				ValidatorX::Actions::AddJumpToGraphItemAction(Message, FText::FromString("Jump to macro"), Blueprint, MacroGraph, MacroGraph->GetFName());

				const FText DeleteMacroText = FText::Format(
					INVTEXT("'Fix' - Delete Macro - '{0}'"),
					FText::FromName(MacroName));

				ValidatorX::Actions::AddAction(Message, DeleteMacroText,
					FSimpleDelegate::CreateLambda([=]
						{
							if(Blueprint && MacroGraph)
							{
								if(ValidatorX::Actions::OpenAsset(Blueprint))
								{
									FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=] (float DeltaTime)
										{
											if(FBlueprintEditor* const BlueprintEditor = ValidatorX::Actions::FindBlueprintEditor(Blueprint, false))
											{
												const FText ConfirmText = FText::Format(
													INVTEXT("Are you sure you want to delete the unused Macro '{0}' from Blueprint '{1}'?"),
													FText::FromName(MacroName),
													FText::FromString(Blueprint->GetName())
												);

												if(FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
												{
													Blueprint->Modify();

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
