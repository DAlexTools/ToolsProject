// Fill out your copyright notice in the Description page of Project Settings.

#include "Validators/DefaultAssignmentValidator.h"
#include "K2Node_VariableSet.h"
#include "Misc/DataValidation.h"
#include "BlueprintEditor.h"

UDefaultAssignmentValidator::UDefaultAssignmentValidator()
{
	SetValidationEnabled(true);
}

bool UDefaultAssignmentValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const
{
	return InAsset && InAsset->IsA<UBlueprint>();
}

bool UDefaultAssignmentValidator::IsEnabled() const
{
	static const UDefaultAssignmentValidator* CDO = GetDefault<UDefaultAssignmentValidator>();
	return CDO->bIsEnabled && !bIsConfigDisabled;
}

EDataValidationResult UDefaultAssignmentValidator::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	bIsError = false;

	if (UBlueprint* const Blueprint = Cast<UBlueprint>(InAsset))
	{
		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);

		for (UEdGraph* const Graph : AllGraphs)
		{
			for (UEdGraphNode* const Node : Graph->Nodes)
			{
				if (UK2Node_VariableSet* const VarSetNode = Cast<UK2Node_VariableSet>(Node))
				{
					const FName		 VarName = VarSetNode->GetVarName();
					const FProperty* const Property = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName);
					if (!Property)
					{
						continue;
					}

					if (UEdGraphPin* const ValuePin = VarSetNode->FindPin(VarName))
					{
						if (!ValuePin->HasAnyConnections())
						{
							const FString PinDefaultValue = ValuePin->DefaultValue;
							FString		  PropertyDefaultValue;
							if (const auto DefaultObjectPtr = Blueprint->GeneratedClass->GetDefaultObject(false))
							{
								FString Temp;
								Property->ExportText_InContainer(0, Temp, DefaultObjectPtr, DefaultObjectPtr, nullptr, PPF_None);
								PropertyDefaultValue = Temp;
							}

							if (PinDefaultValue == PropertyDefaultValue)
							{
								const FText MessageText = FText::Format(
									INVTEXT("Redundant assignment detected: variable '{0}' in Blueprint '{1}' is assigned its default value."),
									FText::FromString(Graph->GetName()),
									FText::FromString(Blueprint->GetName()));

								TSharedRef<FTokenizedMessage> Message = Context.AddMessage(EMessageSeverity::Warning, MessageText);
								Message->AddToken(FActionToken::Create(FText::FromString("Jump to Node"), FText::GetEmpty(),
									FSimpleDelegate::CreateUObject(this, &UDefaultAssignmentValidator::JumpToNode, Blueprint, Graph, VarSetNode)));

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

void UDefaultAssignmentValidator::JumpToNode(UBlueprint* Blueprint, UEdGraph* Graph, UK2Node_VariableSet* Node)
{
	if (Blueprint && Graph)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
			if (IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false))
			{
				if (IBlueprintEditor* BlueprintEditor = StaticCast<IBlueprintEditor*>(EditorInstance))
				{
					if (TSharedPtr<SGraphEditor> GraphEditor = BlueprintEditor->OpenGraphAndBringToFront(Graph, true))
					{
						GraphEditor->JumpToNode(Node, false);
					}
				}
			}
		}
	}
}
