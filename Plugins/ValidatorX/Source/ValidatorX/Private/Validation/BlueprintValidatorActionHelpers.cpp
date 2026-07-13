// Fill out your copyright notice in the Description page of Project Settings.

#include "Validation/BlueprintValidatorActionHelpers.h"

#include "BlueprintEditor.h"
#include "EdGraph/EdGraphNode.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "GraphEditor.h"
#include "Misc/DataValidation.h"
#include "SMyBlueprint.h"
#include "Subsystems/AssetEditorSubsystem.h"

UAssetEditorSubsystem* ValidatorX::Actions::OpenAsset(UObject* Asset)
{
	if(!Asset || !GEditor)
	{
		return nullptr;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if(!AssetEditorSubsystem)
	{
		return nullptr;
	}

	AssetEditorSubsystem->OpenEditorForAsset(Asset);
	return AssetEditorSubsystem;
}

FBlueprintEditor* ValidatorX::Actions::FindBlueprintEditor(UBlueprint* Blueprint, bool bFocusIfOpen)
{
	if(!Blueprint || !GEditor)
	{
		return nullptr;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if(!AssetEditorSubsystem)
	{
		return nullptr;
	}

	IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, bFocusIfOpen);
	return EditorInstance ? StaticCast<FBlueprintEditor*>(EditorInstance) : nullptr;
}

FBlueprintEditor* ValidatorX::Actions::OpenBlueprintEditor(UBlueprint* Blueprint, bool bFocusIfOpen)
{
	return OpenAsset(Blueprint) ? FindBlueprintEditor(Blueprint, bFocusIfOpen) : nullptr;
}

void ValidatorX::Actions::AddAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, FSimpleDelegate Action)
{
	AddAction(Message, ActionText, FText::GetEmpty(), Action);
}

void ValidatorX::Actions::AddAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, const FText& ToolTipText, FSimpleDelegate Action)
{
	Message->AddToken(FActionToken::Create(ActionText, ToolTipText, Action));
}

void ValidatorX::Actions::AddOpenAssetAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UObject* Asset)
{
	AddAction(
		Message,
		ActionText,
		FSimpleDelegate::CreateLambda([Asset]
			{
				OpenAsset(Asset);
			}));
}

void ValidatorX::Actions::AddJumpToGraphAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, UEdGraph* Graph)
{
	AddJumpToGraphAction(Message, ActionText, FText::GetEmpty(), Blueprint, Graph);
}

void ValidatorX::Actions::AddJumpToGraphAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, const FText& ToolTipText, UBlueprint* Blueprint, UEdGraph* Graph)
{
	AddAction(
		Message,
		ActionText,
		ToolTipText,
		FSimpleDelegate::CreateLambda([Blueprint, Graph]
			{
				if(!Graph)
				{
					return;
				}

				if(FBlueprintEditor* BlueprintEditor = OpenBlueprintEditor(Blueprint))
				{
					BlueprintEditor->OpenGraphAndBringToFront(Graph, true);
				}
			}));
}

void ValidatorX::Actions::AddJumpToBlueprintItemAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, FName ItemName)
{
	AddAction(
		Message,
		ActionText,
		FSimpleDelegate::CreateLambda([Blueprint, ItemName]
			{
				if(FBlueprintEditor* BlueprintEditor = OpenBlueprintEditor(Blueprint))
				{
					if(TSharedPtr<SMyBlueprint> MyBlueprintWidget = BlueprintEditor->GetMyBlueprintWidget())
					{
						MyBlueprintWidget->SelectItemByName(ItemName, ESelectInfo::Direct, INDEX_NONE, false);
					}
				}
			}));
}

void ValidatorX::Actions::AddJumpToGraphItemAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, UEdGraph* Graph, FName ItemName)
{
	AddAction(
		Message,
		ActionText,
		FSimpleDelegate::CreateLambda([Blueprint, Graph, ItemName]
			{
				if(!Graph)
				{
					return;
				}

				if(FBlueprintEditor* BlueprintEditor = OpenBlueprintEditor(Blueprint))
				{
					BlueprintEditor->OpenGraphAndBringToFront(Graph, true);
					if(TSharedPtr<SMyBlueprint> MyBlueprintWidget = BlueprintEditor->GetMyBlueprintWidget())
					{
						MyBlueprintWidget->SelectItemByName(ItemName, ESelectInfo::Direct, INDEX_NONE, false);
					}
				}
			}));
}

void ValidatorX::Actions::AddJumpToNodeAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node)
{
	AddJumpToNodeAction(Message, ActionText, Blueprint, Graph, Node, TFunction<void(SGraphEditor&)>());
}

void ValidatorX::Actions::AddJumpToNodeAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node, TFunction<void(SGraphEditor&)> PostJumpAction)
{
	AddAction(
		Message,
		ActionText,
		FSimpleDelegate::CreateLambda([Blueprint, Graph, Node, PostJumpAction]
			{
				if(!Blueprint || !Graph || !Node)
				{
					return;
				}

				if(FBlueprintEditor* BlueprintEditor = OpenBlueprintEditor(Blueprint))
				{
					if(TSharedPtr<SGraphEditor> GraphEditor = BlueprintEditor->OpenGraphAndBringToFront(Graph, true))
					{
						GraphEditor->JumpToNode(Node, false);
						if(PostJumpAction)
						{
							PostJumpAction(*GraphEditor);
						}
					}
				}
			}));
}
