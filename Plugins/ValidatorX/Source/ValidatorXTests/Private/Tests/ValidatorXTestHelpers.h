// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetRegistry/AssetData.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node.h"
#include "Materials/Material.h"
#include "Misc/DataValidation.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/Package.h"

namespace ValidatorX::Tests
{
	template <typename TValidator>
	TValidator* NewValidator(const TCHAR* Name = TEXT("ValidatorX_TestValidator"))
	{
		return NewObject<TValidator>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), TValidator::StaticClass(), FName(Name)));
	}

	inline FAssetData MakeAssetData(UObject* Asset)
	{
		return Asset ? FAssetData(Asset) : FAssetData();
	}

	inline FDataValidationContext MakeValidationContext()
	{
		return FDataValidationContext();
	}

	template <typename TValidator>
	EDataValidationResult Validate(TValidator* Validator, UObject* Asset)
	{
		FDataValidationContext Context;
		return Validator->ValidateLoadedAsset_Implementation(MakeAssetData(Asset), Asset, Context);
	}

	inline UBlueprint* NewTransientBlueprint(const TCHAR* Name = TEXT("ValidatorX_TestBlueprint"))
	{
		UBlueprint* Blueprint = NewObject<UBlueprint>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), FName(Name)));
		Blueprint->ParentClass = UObject::StaticClass();
		return Blueprint;
	}

	inline UMaterial* NewTransientMaterial(const TCHAR* Name = TEXT("ValidatorX_TestMaterial"))
	{
		return NewObject<UMaterial>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UMaterial::StaticClass(), FName(Name)));
	}

	template <typename TGraphArray>
	UEdGraph* AddGraph(UBlueprint* Blueprint, TGraphArray& Graphs, const TCHAR* Name)
	{
		UEdGraph* Graph = NewObject<UEdGraph>(Blueprint, MakeUniqueObjectName(Blueprint, UEdGraph::StaticClass(), FName(Name)));
		Graphs.Add(Graph);
		return Graph;
	}

	template <typename TNode>
	TNode* AddNode(UEdGraph* Graph)
	{
		TNode* Node = NewObject<TNode>(Graph);
		Graph->AddNode(Node, false, false);
		Node->CreateNewGuid();
		Node->PostPlacedNewNode();
		Node->AllocateDefaultPins();
		return Node;
	}

	inline FBPVariableDescription MakeVariable(const FName& VarName, const FName& PinCategory = UEdGraphSchema_K2::PC_Boolean)
	{
		FBPVariableDescription Variable;
		Variable.VarName = VarName;
		Variable.VarType.PinCategory = PinCategory;
		return Variable;
	}

	inline UEdGraphPin* AddPin(UEdGraphNode* Node, EEdGraphPinDirection Direction, const FName& Name, const FName& Category)
	{
		UEdGraphPin* Pin = Node->CreatePin(Direction, Category, Name);
		return Pin;
	}

	inline void LinkPins(UEdGraphPin* A, UEdGraphPin* B)
	{
		if(A && B)
		{
			A->LinkedTo.AddUnique(B);
			B->LinkedTo.AddUnique(A);
		}
	}
}
