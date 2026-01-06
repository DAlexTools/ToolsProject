// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPUtilsNodeFunctionLibrary.generated.h"

class UK2Node_Event;
class UK2Node_IfThenElse;
class UK2Node_VariableGet;
class UK2Node_VariableSet;
class UEdGraphNode_Comment;
class UK2Node_CallFunction;
class UK2Node_MacroInstance;

/**
 * @brief Blueprint utility function library.
 *
 * Provides static helper functions for working with Blueprints and K2 nodes,
 * such as finding derived classes, analyzing nodes, and inspecting graphs.
 * Inherits from `UBlueprintFunctionLibrary` to allow calling from Blueprints.
 */
UCLASS()
class VALIDATORX_API UBPUtilsNodeFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief Populates an array with all Blueprint-generated classes derived from a specified parent class.
	 *
	 * Iterates over all loaded `UBlueprintGeneratedClass` objects in memory and checks whether
	 * each is a subclass of the given `ParentClass` (excluding the `ParentClass` itself).
	 * Only loaded classes are returned.
	 *
	 * @param ParentClass The base class to search derived Blueprint classes from. Must not be null.
	 * @param OutDerived  The output array that will be filled with all matching derived Blueprint-generated classes.
	 */
	static void GetDerivedBlueprintClasses(const UClass* ParentClass, TArray<UClass*>& OutDerived);

	/**
	 * @brief Populates an array with derived Blueprint classes using the asset registry.
	 *
	 * Similar to `GetDerivedBlueprintClasses`, but also searches unloaded assets via the registry.
	 *
	 * @param ParentClass The base class to search derived Blueprint classes from.
	 * @param OutDerived  The output array that will be filled with matching classes.
	 */
	static void GetDerivedRegistryBlueprintClasses(const UClass* ParentClass, TArray<UClass*>& OutDerived);

	/**
	 * @brief Retrieves all Blueprint-generated classes derived from a parent class.
	 *
	 * Searches both loaded classes and, optionally, the asset registry.
	 *
	 * @param ParentClass        The parent class to search from.
	 * @param OutDerived         The array to fill with derived classes.
	 * @param bSearchAssetRegistry Whether to include the asset registry in the search.
	 */
	static void GetAllDerivedBlueprintClasses(const UClass* ParentClass, TArray<UClass*>& OutDerived, bool bSearchAssetRegistry = true);

	/**
	 * @brief Checks whether a given event node is empty.
	 *
	 * @param EventNode The event node to inspect.
	 * @return true if the event node has no logic connected; false otherwise.
	 */
	static bool IsEmptyEvent(const UK2Node_Event* EventNode);

	/**
	 * @brief Checks whether a given function call node is empty.
	 *
	 * @param EventNode The function call node to inspect.
	 * @return true if the function has no executable logic; false otherwise.
	 */
	static bool IsEmptyFunctions(const UK2Node_CallFunction* EventNode);

	/**
	 * @brief Determines if a "Get Variable" node is unused in the Blueprint.
	 *
	 * @param Node The variable get node.
	 * @return true if the node is unused; false otherwise.
	 */
	static bool IsUnusedVariableGet(const UK2Node_VariableGet* Node);

	/**
	 * @brief Determines if a "Set Variable" node is unused in the Blueprint.
	 *
	 * @param Node The variable set node.
	 * @return true if the node is unused; false otherwise.
	 */
	static bool IsUnusedVariableSet(const UK2Node_VariableSet* Node);

	/**
	 * @brief Checks whether a macro instance node is unused.
	 *
	 * @param Node The macro instance node.
	 * @return true if the macro instance is unused; false otherwise.
	 */
	static bool IsUnusedMacroInstance(const UK2Node_MacroInstance* Node);

	/**
	 * @brief Determines if a pure function node is empty.
	 *
	 * @param Node The pure function node.
	 * @return true if the function has no output or logic; false otherwise.
	 */
	static bool IsEmptyPureFunction(const UK2Node_CallFunction* Node);

	/**
	 * @brief Checks if a generic node represents an unused variable node.
	 *
	 * @param Node The node to inspect.
	 * @return true if the node is an unused variable node; false otherwise.
	 */
	static bool IsUnusedVariableNode(UEdGraphNode* Node);

	/**
	 * @brief Determines if a node is inside a comment bubble.
	 *
	 * @param Node         The node to check.
	 * @param CommentNodes Array of comment nodes to search within.
	 * @return true if the node is inside any of the comment bubbles; false otherwise.
	 */
	static bool IsNodeInsideComment(UEdGraphNode* Node, const TArray<UEdGraphNode_Comment*>& CommentNodes);

	/**
	 * @brief Checks whether a node has execution output connections.
	 *
	 * @param Node The node to inspect.
	 * @return true if the node has any execution output pins connected; false otherwise.
	 */
	static bool HasExecutionOutputConnections(const UEdGraphNode* Node);

	/**
	 * @brief Gets the type of a graph in a Blueprint.
	 *
	 * @param Blueprint The Blueprint containing the graph.
	 * @param Graph     The graph to query.
	 * @return A string representing the type of the graph (e.g., Function, Macro, EventGraph).
	 */
	static FString GetGraphType(UBlueprint* Blueprint, UEdGraph* Graph);

	/**
	 * @brief Checks whether a boolean variable is set in the Blueprint or its parent Blueprints.
	 *
	 * @param Blueprint      The Blueprint to inspect.
	 * @param VarName        The variable name to check.
	 * @param OutSourceInfo  Optional output string describing the source of the variable.
	 * @return true if the variable is set somewhere; false otherwise.
	 */
	static bool IsBoolVariableSetInThisOrParentBPs(UBlueprint* Blueprint, FName VarName, FString* OutSourceInfo = nullptr);

	/**
	 * @brief Checks whether all execution pins of a branch node are disconnected.
	 *
	 * @param Branch The branch node to inspect.
	 * @return true if all execution outputs are disconnected; false otherwise.
	 */
	static bool AreAllBranchExecsDisconnected(const UK2Node_IfThenElse* Branch);
};
