#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "K2Node_CallDelegate.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Tunnel.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Tests\ValidatorXTestHelpers.h"
#include "Validators/BranchConditionValidator.h"
#include "Validators/CircularDependencyValidator.h"
#include "Validators/DeadBranchValidator.h"
#include "Validators/DebugCallValidator.h"
#include "Validators/DefaultAssignmentValidator.h"
#include "Validators/EmptyBranchValidator.h"
#include "Validators/EmptyFunctionValidator.h"
#include "Validators/EmptyMacroValidator.h"
#include "Validators/GlobalVariableNeverUsedValidator.h"
#include "Validators/LocalGlobalNameConflictValidator.h"
#include "Validators/LocalVariableNeverUsedValidator.h"
#include "Validators/LongFunctionValidator.h"
#include "Validators/TickUsageValidator.h"
#include "Validators/UnboundEventDispatcherValidator.h"
#include "Validators/UnhandledCastFailureValidator.h"
#include "Validators/UnusedFunctionValidator.h"
#include "Validators/UnusedMacroValidator.h"
#include "Validators/UnusedNodeValidator.h"

namespace ValidatorX::Tests
{
	static UK2Node_CallFunction* AddNamedCallFunctionNode(UEdGraph* Graph, const FName& FunctionName)
	{
		UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
		Node->FunctionReference.SetSelfMember(FunctionName);
		Graph->AddNode(Node, false, false);
		Node->CreateNewGuid();
		return Node;
	}

	static UK2Node_MacroInstance* AddMacroInstanceNode(UEdGraph* Graph, UEdGraph* MacroGraph)
	{
		UK2Node_MacroInstance* Node = NewObject<UK2Node_MacroInstance>(Graph);
		Node->SetMacroGraph(MacroGraph);
		Graph->AddNode(Node, false, false);
		Node->CreateNewGuid();
		return Node;
	}

	static UK2Node_CallFunction* AddExternalCallFunctionNode(UEdGraph* Graph, UFunction* Function)
	{
		UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
		if(Function)
		{
			Node->FunctionReference.SetExternalMember(Function->GetFName(), Function->GetOwnerClass());
		}
		Graph->AddNode(Node, false, false);
		Node->CreateNewGuid();
		Node->PostPlacedNewNode();
		Node->AllocateDefaultPins();
		return Node;
	}

	static UK2Node_DynamicCast* AddDynamicCastNode(UEdGraph* Graph)
	{
		UK2Node_DynamicCast* Node = NewObject<UK2Node_DynamicCast>(Graph);
		Node->TargetType = UObject::StaticClass();
		Graph->AddNode(Node, false, false);
		Node->CreateNewGuid();
		Node->PostPlacedNewNode();
		Node->AllocateDefaultPins();
		return Node;
	}

	static UK2Node_FunctionEntry* AddFunctionEntryWithLocal(UEdGraph* Graph, const FName& LocalVariableName)
	{
		UK2Node_FunctionEntry* Entry = AddNode<UK2Node_FunctionEntry>(Graph);
		Entry->LocalVariables.Add(MakeVariable(LocalVariableName));
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXBlueprintValidatorsSmokeTest,
	"ValidatorX.Validators.Blueprint.EmptyBlueprintSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXBlueprintValidatorsSmokeTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_EmptySmokeBlueprint"));
	UMaterial* NonBlueprint = NewTransientMaterial(TEXT("ValidatorX_BlueprintSmokeNonBlueprint"));

	TestEqual(TEXT("BranchCondition empty blueprint is valid"), Validate(NewValidator<UBranchConditionValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("CircularDependency empty blueprint is valid"), Validate(NewValidator<UCircularDependencyValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("DeadBranch empty blueprint is valid"), Validate(NewValidator<UDeadBranchValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("DebugCall empty blueprint is valid"), Validate(NewValidator<UDebugCallValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("DefaultAssignment empty blueprint is valid"), Validate(NewValidator<UDefaultAssignmentValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("EmptyBranch empty blueprint is valid"), Validate(NewValidator<UEmptyBranchValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("EmptyFunction empty blueprint is valid"), Validate(NewValidator<UEmptyFunctionValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("EmptyMacro empty blueprint is valid"), Validate(NewValidator<UEmptyMacroValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("GlobalVariableNeverUsed empty blueprint is valid"), Validate(NewValidator<UGlobalVariableNeverUsedValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("LocalGlobalNameConflict empty blueprint is valid"), Validate(NewValidator<ULocalGlobalNameConflictValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("LocalVariableNeverUsed empty blueprint is valid"), Validate(NewValidator<ULocalVariableNeverUsedValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("LongFunction empty blueprint is valid"), Validate(NewValidator<ULongFunctionValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("TickUsage empty blueprint is valid"), Validate(NewValidator<UTickUsageValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("UnboundEventDispatcher empty blueprint is valid"), Validate(NewValidator<UUnboundEventDispatcherValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("UnhandledCastFailure empty blueprint is valid"), Validate(NewValidator<UUnhandledCastFailureValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("UnusedFunction empty blueprint is valid"), Validate(NewValidator<UUnusedFunctionValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("UnusedMacro empty blueprint is valid"), Validate(NewValidator<UUnusedMacroValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("UnusedNode empty blueprint is valid"), Validate(NewValidator<UUnusedNodeValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("Blueprint validators treat non-blueprint assets as valid during loaded validation"), Validate(NewValidator<UBranchConditionValidator>(), NonBlueprint), EDataValidationResult::Valid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXBranchValidatorsTest,
	"ValidatorX.Validators.Blueprint.BranchValidators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXBranchValidatorsTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_BranchBlueprint"));
	UEdGraph* Graph = AddGraph(Blueprint, Blueprint->UbergraphPages, TEXT("EventGraph"));
	AddNode<UEdGraphNode>(Graph);
	AddNode<UK2Node_IfThenElse>(Graph);

	TestEqual(TEXT("Branch with unlinked condition is invalid"), Validate(NewValidator<UBranchConditionValidator>(), Blueprint), EDataValidationResult::Invalid);
	TestEqual(TEXT("Branch with unlinked Then and Else is invalid"), Validate(NewValidator<UEmptyBranchValidator>(), Blueprint), EDataValidationResult::Invalid);
	TestEqual(TEXT("Branch with no execution logic is dead"), Validate(NewValidator<UDeadBranchValidator>(), Blueprint), EDataValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXFunctionAndMacroValidatorsTest,
	"ValidatorX.Validators.Blueprint.FunctionsAndMacros",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXFunctionAndMacroValidatorsTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_FunctionMacroBlueprint"));
	UEdGraph* EmptyFunction = AddGraph(Blueprint, Blueprint->FunctionGraphs, TEXT("EmptyFunction"));
	AddNode<UK2Node_FunctionEntry>(EmptyFunction);
	AddNode<UK2Node_FunctionResult>(EmptyFunction);

	UEdGraph* EmptyMacro = AddGraph(Blueprint, Blueprint->MacroGraphs, TEXT("EmptyMacro"));
	AddNode<UK2Node_Tunnel>(EmptyMacro);

	TestEqual(TEXT("Function with only entry/result is invalid"), Validate(NewValidator<UEmptyFunctionValidator>(), Blueprint), EDataValidationResult::Invalid);
	TestEqual(TEXT("Macro with only tunnel nodes is invalid"), Validate(NewValidator<UEmptyMacroValidator>(), Blueprint), EDataValidationResult::Invalid);
	TestEqual(TEXT("Unused function graph is invalid"), Validate(NewValidator<UUnusedFunctionValidator>(), Blueprint), EDataValidationResult::Invalid);
	TestEqual(TEXT("Unused macro graph is invalid"), Validate(NewValidator<UUnusedMacroValidator>(), Blueprint), EDataValidationResult::Invalid);

	UBlueprint* UsedBlueprint = NewTransientBlueprint(TEXT("ValidatorX_UsedFunctionMacroBlueprint"));
	UEdGraph* FunctionGraph = AddGraph(UsedBlueprint, UsedBlueprint->FunctionGraphs, TEXT("UsedFunction"));
	AddNode<UK2Node_FunctionEntry>(FunctionGraph);
	AddNode<UEdGraphNode>(FunctionGraph);

	UEdGraph* MacroGraph = AddGraph(UsedBlueprint, UsedBlueprint->MacroGraphs, TEXT("UsedMacro"));
	AddNode<UEdGraphNode>(MacroGraph);

	UEdGraph* EventGraph = AddGraph(UsedBlueprint, UsedBlueprint->UbergraphPages, TEXT("EventGraph"));
	AddNamedCallFunctionNode(EventGraph, FunctionGraph->GetFName());
	AddMacroInstanceNode(EventGraph, MacroGraph);

	TestEqual(TEXT("Called function graph is valid for unused-function validator"), Validate(NewValidator<UUnusedFunctionValidator>(), UsedBlueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("Referenced macro graph is valid for unused-macro validator"), Validate(NewValidator<UUnusedMacroValidator>(), UsedBlueprint), EDataValidationResult::Valid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXLongFunctionValidatorTest,
	"ValidatorX.Validators.Blueprint.LongFunction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXLongFunctionValidatorTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* ValidBlueprint = NewTransientBlueprint(TEXT("ValidatorX_LongFunctionValidBlueprint"));
	UEdGraph* ValidGraph = AddGraph(ValidBlueprint, ValidBlueprint->FunctionGraphs, TEXT("ShortFunction"));
	for(int32 Index = 0; Index < 200; ++Index)
	{
		AddNode<UEdGraphNode>(ValidGraph);
	}
	TestEqual(TEXT("Graph at node limit is valid"), Validate(NewValidator<ULongFunctionValidator>(), ValidBlueprint), EDataValidationResult::Valid);

	UBlueprint* InvalidBlueprint = NewTransientBlueprint(TEXT("ValidatorX_LongFunctionInvalidBlueprint"));
	UEdGraph* InvalidGraph = AddGraph(InvalidBlueprint, InvalidBlueprint->FunctionGraphs, TEXT("LongFunction"));
	for(int32 Index = 0; Index < 201; ++Index)
	{
		AddNode<UEdGraphNode>(InvalidGraph);
	}
	TestEqual(TEXT("Graph above node limit is invalid"), Validate(NewValidator<ULongFunctionValidator>(), InvalidBlueprint), EDataValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXVariableValidatorsTest,
	"ValidatorX.Validators.Blueprint.VariableValidators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXVariableValidatorsTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	const FName VarName(TEXT("Flag"));

	UBlueprint* UnusedGlobalBlueprint = NewTransientBlueprint(TEXT("ValidatorX_UnusedGlobalBlueprint"));
	UnusedGlobalBlueprint->NewVariables.Add(MakeVariable(VarName));
	TestEqual(TEXT("Unused global variable is invalid"), Validate(NewValidator<UGlobalVariableNeverUsedValidator>(), UnusedGlobalBlueprint), EDataValidationResult::Invalid);

	UBlueprint* UsedGlobalBlueprint = NewTransientBlueprint(TEXT("ValidatorX_UsedGlobalBlueprint"));
	UsedGlobalBlueprint->NewVariables.Add(MakeVariable(VarName));
	UEdGraph* UsedGraph = AddGraph(UsedGlobalBlueprint, UsedGlobalBlueprint->UbergraphPages, TEXT("EventGraph"));
	UK2Node_VariableGet* VarGet = AddNode<UK2Node_VariableGet>(UsedGraph);
	VarGet->VariableReference.SetSelfMember(VarName);
	TestEqual(TEXT("Referenced global variable is valid"), Validate(NewValidator<UGlobalVariableNeverUsedValidator>(), UsedGlobalBlueprint), EDataValidationResult::Valid);

	UBlueprint* LocalBlueprint = NewTransientBlueprint(TEXT("ValidatorX_LocalBlueprint"));
	UEdGraph* LocalFunction = AddGraph(LocalBlueprint, LocalBlueprint->FunctionGraphs, TEXT("LocalFunction"));
	AddFunctionEntryWithLocal(LocalFunction, VarName);
	TestEqual(TEXT("Unused local variable is invalid"), Validate(NewValidator<ULocalVariableNeverUsedValidator>(), LocalBlueprint), EDataValidationResult::Invalid);

	UK2Node_VariableSet* LocalSet = AddNode<UK2Node_VariableSet>(LocalFunction);
	LocalSet->VariableReference.SetSelfMember(VarName);
	TestEqual(TEXT("Referenced local variable is valid"), Validate(NewValidator<ULocalVariableNeverUsedValidator>(), LocalBlueprint), EDataValidationResult::Valid);

	UBlueprint* ConflictBlueprint = NewTransientBlueprint(TEXT("ValidatorX_LocalGlobalConflictBlueprint"));
	ConflictBlueprint->NewVariables.Add(MakeVariable(VarName));
	UEdGraph* ConflictFunction = AddGraph(ConflictBlueprint, ConflictBlueprint->FunctionGraphs, TEXT("ConflictFunction"));
	AddFunctionEntryWithLocal(ConflictFunction, VarName);
	TestEqual(TEXT("Local/global name conflict is invalid"), Validate(NewValidator<ULocalGlobalNameConflictValidator>(), ConflictBlueprint), EDataValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXEventAndCallValidatorsTest,
	"ValidatorX.Validators.Blueprint.EventAndCallValidators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXEventAndCallValidatorsTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_EventCallBlueprint"));
	UEdGraph* Graph = AddGraph(Blueprint, Blueprint->UbergraphPages, TEXT("EventGraph"));

	UK2Node_CallFunction* DebugCall = AddNamedCallFunctionNode(Graph, TEXT("PrintString"));
	TestEqual(TEXT("PrintString call is invalid"), Validate(NewValidator<UDebugCallValidator>(), Blueprint), EDataValidationResult::Invalid);

	UK2Node_Event* TickEvent = AddNode<UK2Node_Event>(Graph);
	TickEvent->CustomFunctionName = TEXT("ReceiveTick");
	UEdGraphPin* ThenPin = TickEvent->FindPin(UEdGraphSchema_K2::PN_Then);
	if(!ThenPin)
	{
		ThenPin = AddPin(TickEvent, EGPD_Output, UEdGraphSchema_K2::PN_Then, UEdGraphSchema_K2::PC_Exec);
	}
	UEdGraphNode* TickTarget = AddNode<UEdGraphNode>(Graph);
	UEdGraphPin* TickTargetExec = AddPin(TickTarget, EGPD_Input, UEdGraphSchema_K2::PN_Execute, UEdGraphSchema_K2::PC_Exec);
	LinkPins(ThenPin, TickTargetExec);
	TestEqual(TEXT("Connected Event Tick is invalid"), Validate(NewValidator<UTickUsageValidator>(), Blueprint), EDataValidationResult::Invalid);

	UK2Node_DynamicCast* DynamicCast = AddDynamicCastNode(Graph);
	if(UEdGraphPin* SuccessPin = DynamicCast->GetValidCastPin())
	{
		UEdGraphNode* CastTarget = AddNode<UEdGraphNode>(Graph);
		UEdGraphPin* CastTargetExec = AddPin(CastTarget, EGPD_Input, UEdGraphSchema_K2::PN_Execute, UEdGraphSchema_K2::PC_Exec);
		LinkPins(SuccessPin, CastTargetExec);
	}
	TestEqual(TEXT("Dynamic cast with success path and no failure path is invalid"), Validate(NewValidator<UUnhandledCastFailureValidator>(), Blueprint), EDataValidationResult::Invalid);

	TestNotNull(TEXT("Debug call node was created"), DebugCall);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXCircularDependencyValidatorTest,
	"ValidatorX.Validators.Blueprint.CircularDependency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXCircularDependencyValidatorTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_CircularDependencyBlueprint"));
	UEdGraph* FunctionA = AddGraph(Blueprint, Blueprint->FunctionGraphs, TEXT("FunctionA"));
	UEdGraph* FunctionB = AddGraph(Blueprint, Blueprint->FunctionGraphs, TEXT("FunctionB"));

	AddNamedCallFunctionNode(FunctionA, FunctionB->GetFName());
	AddNamedCallFunctionNode(FunctionB, FunctionA->GetFName());

	TestEqual(TEXT("Mutual function calls are invalid"), Validate(NewValidator<UCircularDependencyValidator>(), Blueprint), EDataValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXDispatcherAndUnusedNodeValidatorsTest,
	"ValidatorX.Validators.Blueprint.DispatcherAndUnusedNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXDispatcherAndUnusedNodeValidatorsTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* DispatcherBlueprint = NewTransientBlueprint(TEXT("ValidatorX_DispatcherBlueprint"));
	DispatcherBlueprint->NewVariables.Add(MakeVariable(TEXT("OnChanged"), UEdGraphSchema_K2::PC_MCDelegate));
	TestEqual(TEXT("Unbound event dispatcher is invalid"), Validate(NewValidator<UUnboundEventDispatcherValidator>(), DispatcherBlueprint), EDataValidationResult::Invalid);

	UBlueprint* UnusedNodeBlueprint = NewTransientBlueprint(TEXT("ValidatorX_UnusedNodeBlueprint"));
	UEdGraph* Graph = AddGraph(UnusedNodeBlueprint, UnusedNodeBlueprint->UbergraphPages, TEXT("EventGraph"));
	AddExternalCallFunctionNode(Graph, UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString")));
	TestEqual(TEXT("Unlinked call function node is invalid as unused node"), Validate(NewValidator<UUnusedNodeValidator>(), UnusedNodeBlueprint), EDataValidationResult::Invalid);

	return true;
}

#endif
