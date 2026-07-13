// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Validators/LocalVariableNeverUsedValidator.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "K2Node_LocalVariable.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "Misc/DataValidation.h"
#include "BlueprintEditorModule.h"
#include "GraphEditor.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "BlueprintEditor.h"
#include "SMyBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Validation/BlueprintValidatorActionHelpers.h"


ULocalVariableNeverUsedValidator::ULocalVariableNeverUsedValidator()
{
}

EDataValidationResult ULocalVariableNeverUsedValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
    bIsError = false;
  
    if(UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
    {
        const auto AllGraphs = Blueprint->FunctionGraphs;
    
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
   
             if(!EntryNode)  continue;
     
             for(const FBPVariableDescription& LocalVar : EntryNode->LocalVariables)
             {
                 bool bUsed = false;

                 for(UEdGraphNode* Node : Graph->Nodes)
                 {
                     if(const UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
                     {
                         if(VarGet->GetVarName() == LocalVar.VarName)
                         {
                             
                             bUsed = true;
                             break;
                         }
                     }
                     else if(const UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
                     {
                         if(VarSet->GetVarName() == LocalVar.VarName)
                         {
                             
                             bUsed = true;
                             break;
                         }
                     }
                 }
     
                 if(!bUsed)
                 {
                     const FText MessageText = FText::Format(
                         INVTEXT("Local variable '{0}' in function '{1}' is never used."),
                         FText::FromName(LocalVar.VarName),
                         FText::FromString(Graph->GetName()));

                     const TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
                     const FText JumpToVariableText = FText::Format(INVTEXT("Jump to variable  - '{0}'"), FText::FromName(LocalVar.VarName));
                     ValidatorX::Actions::AddJumpToGraphItemAction(Message, JumpToVariableText, Blueprint, Graph, LocalVar.VarName);

                     const FText DeleteVariableText = FText::Format(INVTEXT("'Fix' - Delete Local Variable - '{0}'"), FText::FromName(LocalVar.VarName));
                     ValidatorX::Actions::AddAction(Message, DeleteVariableText,
                         FSimpleDelegate::CreateLambda([=]
                             {
                                 if (Blueprint && EntryNode)
                                 {
                                     if(ValidatorX::Actions::OpenAsset(Blueprint))
                                     {
                                         FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([=] (float DeltaTime)
                                             {
                                                 if(FBlueprintEditor* BlueprintEditor = ValidatorX::Actions::FindBlueprintEditor(Blueprint, /*bFocusIfOpen=*/false))
                                                 {
                                                     if(BlueprintEditor->GetMyBlueprintWidget().IsValid())
                                                     {
                                                         int32 IndexToRemove = INDEX_NONE;
                                                         for(int32 i = 0; i < EntryNode->LocalVariables.Num(); ++i)
                                                         {
                                                             if(EntryNode->LocalVariables[i].VarName == LocalVar.VarName)
                                                             {
                                                                 IndexToRemove = i;
                                                                 break;
                                                             }
                                                         }

                                                         if(IndexToRemove != INDEX_NONE)
                                                         {
                                                             const FText ConfirmText = FText::Format(
                                                                 INVTEXT("Are you sure you want to delete the dispatcher '{0}' from Blueprint '{1}'?"),
                                                                 FText::FromName(LocalVar.VarName),
                                                                 FText::FromString(Blueprint->GetName())
                                                             );

                                                             if(FMessageDialog::Open(EAppMsgType::YesNo, ConfirmText) == EAppReturnType::Yes)
                                                             {
                                                                 EntryNode->Modify();
                                                                 EntryNode->LocalVariables.RemoveAt(IndexToRemove);
                                                                 FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                                                             }
                                                         }

                                                         return false;
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
   
    return bIsError ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
