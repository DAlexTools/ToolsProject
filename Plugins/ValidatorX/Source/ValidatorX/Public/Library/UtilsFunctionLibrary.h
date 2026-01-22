// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SMyBlueprint.h"

/**
 * @brief Utility function library for string and general helper functions.
 *
 * This class contains static helper functions that can be used throughout
 * the validator system. It is marked as `final` to prevent inheritance.
 */
class VALIDATORX_API FUtilsFunctionLibrary final
{
public:
	/**
	 * @brief Inserts spaces before uppercase letters in a string.
	 *
	 * Converts strings like "ValidatorName" to "Validator Name" for improved readability.
	 *
	 * @param Input The input string to process.
	 * @return A new string with spaces inserted before uppercase letters.
	 */
	[[nodiscard]] static FString AddSpacesBeforeUppercase(const FString& Input);
};

class FBlueprintHelper final
{
public:
	FORCEINLINE static UAssetEditorSubsystem* OpenBlueprintEditor(UBlueprint* Blueprint)
	{
		if (!IsValid(Blueprint) || !GEditor)
		{
			return nullptr;
		}

		UAssetEditorSubsystem* const AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (IsValid(AssetEditorSubsystem))
		{
			AssetEditorSubsystem->OpenEditorForAsset(Blueprint);
			return AssetEditorSubsystem;
		}
		return nullptr;
	}

	FORCEINLINE static void OpenGraphEditor(UBlueprint* Blueprint, UEdGraph* Graph)
	{
		if (!Blueprint || !Graph)
		{
			return;
		}
		UAssetEditorSubsystem* const AssetEditorSubsystem = OpenBlueprintEditor(Blueprint);
		IAssetEditorInstance* const	 EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false);
		
		if (!EditorInstance)
		{
			return;
		}
		
		if (FBlueprintEditor* const BlueprintEditor = StaticCast<FBlueprintEditor*>(EditorInstance))
		{
			BlueprintEditor->OpenGraphAndBringToFront(Graph, true);
		}
		
		
	}

	FORCEINLINE static void OpenGraphAndSelectItem(UBlueprint* Blueprint, UEdGraph* Graph)
	{
		if (!Blueprint || !Graph)
		{
			return;
		}

		if (UAssetEditorSubsystem* const AssetEditorSubsystem = OpenBlueprintEditor(Blueprint))
		{
			if (IAssetEditorInstance* const EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false))
			{
				if (FBlueprintEditor* const BlueprintEditor = StaticCast<FBlueprintEditor*>(EditorInstance))
				{
					BlueprintEditor->OpenGraphAndBringToFront(Graph, true);
					if (const TSharedPtr<SMyBlueprint> MyBlueprintWidget = BlueprintEditor->GetMyBlueprintWidget())
					{
						MyBlueprintWidget->SelectItemByName(Graph->GetFName(),
							ESelectInfo::Direct,
							INDEX_NONE,
							false);
					}
				}
			}
		}
	}

	FORCEINLINE static void OpenBlueprintAndSelectItemByName(UBlueprint* Blueprint, FName GraphName)
	{
		if (!Blueprint)
		{
			return;
		}

		if (UAssetEditorSubsystem* AssetEditorSubsystem = OpenBlueprintEditor(Blueprint))
		{
			if (IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false))
			{
				if (FBlueprintEditor* BlueprintEditor = StaticCast<FBlueprintEditor*>(EditorInstance))
				{
					if (TSharedPtr<SMyBlueprint> MyBlueprintWidget = BlueprintEditor->GetMyBlueprintWidget())
					{
						MyBlueprintWidget->SelectItemByName(GraphName,
							ESelectInfo::Direct,
							INDEX_NONE,
							false);
					}
				}
			}
		}
	}

	FORCEINLINE static void JumpToNode(UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node)
	{
		if (!Blueprint || !Graph)
		{
			return;
		}

		if (UAssetEditorSubsystem* const AssetEditorSubsystem = OpenBlueprintEditor(Blueprint))
		{
			if (IAssetEditorInstance* const EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false))
			{
				if (IBlueprintEditor* const BlueprintEditor = StaticCast<IBlueprintEditor*>(EditorInstance))
				{
					if (const TSharedPtr<SGraphEditor> GraphEditor = BlueprintEditor->OpenGraphAndBringToFront(Graph, true))
					{
						GraphEditor->JumpToNode(Node, false);
					}
				}
			}
		}
	}
};