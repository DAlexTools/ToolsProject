// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/EmptyFunctionValidator.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintEditor.h"
#include "Misc/DataValidation.h"
#include "Validation/BlueprintValidatorActionHelpers.h"

UEmptyFunctionValidator::UEmptyFunctionValidator()
{
}

EDataValidationResult UEmptyFunctionValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
		{
			if(!FunctionGraph) continue;

			if(FunctionGraph->GetFName() == UEdGraphSchema_K2::FN_UserConstructionScript) continue;

			int32 UsefulNodeCount = 0;
			for (UEdGraphNode* Node : FunctionGraph->Nodes)
			{
				if(!Node) continue;

				if (Node->IsA<UK2Node_FunctionEntry>() || Node->IsA<UK2Node_FunctionResult>())
				{
					continue;
				}

				UsefulNodeCount++;
			}

			if (UsefulNodeCount == 0)
			{
				const FText MessageText = FText::Format(
					INVTEXT("Function '{0}' in Blueprint '{1}' is empty."),
					FText::FromString(FunctionGraph->GetName()),
					FText::FromString(Blueprint->GetName()));

				const TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);

				const FText JumpToFunctionText = FText::Format(
					INVTEXT("Jump to Function - '{0}'"),
					FText::FromString(FunctionGraph->GetName()));

				ValidatorX::Actions::AddJumpToGraphAction(Message, JumpToFunctionText, Blueprint, FunctionGraph);

				const FText DeleteFunctionText = FText::Format(
					INVTEXT("'Fix' - Delete Function - '{0}'"),
					FText::FromString(FunctionGraph->GetName()));

				ValidatorX::Actions::AddAction(Message, DeleteFunctionText,
					FSimpleDelegate::CreateLambda([=]
						{
							if(Blueprint && FunctionGraph)
							{
								if(ValidatorX::Actions::OpenAsset(Blueprint))
								{
									FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=] (float DeltaTime)
										{
											if(FBlueprintEditor* BlueprintEditor = ValidatorX::Actions::FindBlueprintEditor(Blueprint, /*bFocusIfOpen=*/false))
											{
												const FText ConfirmText = FText::Format(
													INVTEXT("Are you sure you want to delete the Function '{0}' from Blueprint '{1}'?"),
													FText::FromString(FunctionGraph->GetName()),
													FText::FromString(Blueprint->GetName())
												);

												if(FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
												{
													Blueprint->Modify();
													FunctionGraph->Modify();

													Blueprint->FunctionGraphs.Remove(FunctionGraph);
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
