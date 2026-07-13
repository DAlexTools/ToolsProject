// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FTokenizedMessage;
class FBlueprintEditor;
class SGraphEditor;
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UAssetEditorSubsystem;
class UObject;

namespace ValidatorX::Actions
{
	VALIDATORX_API UAssetEditorSubsystem* OpenAsset(UObject* Asset);
	VALIDATORX_API FBlueprintEditor* FindBlueprintEditor(UBlueprint* Blueprint, bool bFocusIfOpen = false);
	VALIDATORX_API FBlueprintEditor* OpenBlueprintEditor(UBlueprint* Blueprint, bool bFocusIfOpen = false);

	VALIDATORX_API void AddAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, FSimpleDelegate Action);
	VALIDATORX_API void AddAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, const FText& ToolTipText, FSimpleDelegate Action);
	VALIDATORX_API void AddOpenAssetAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UObject* Asset);
	VALIDATORX_API void AddJumpToGraphAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, UEdGraph* Graph);
	VALIDATORX_API void AddJumpToGraphAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, const FText& ToolTipText, UBlueprint* Blueprint, UEdGraph* Graph);
	VALIDATORX_API void AddJumpToBlueprintItemAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, FName ItemName);
	VALIDATORX_API void AddJumpToGraphItemAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, UEdGraph* Graph, FName ItemName);
	VALIDATORX_API void AddJumpToNodeAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node);
	VALIDATORX_API void AddJumpToNodeAction(const TSharedRef<FTokenizedMessage>& Message, const FText& ActionText, UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node, TFunction<void(SGraphEditor&)> PostJumpAction);
}
