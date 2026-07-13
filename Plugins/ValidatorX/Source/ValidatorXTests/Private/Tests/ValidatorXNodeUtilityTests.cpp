#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EdGraphNode_Comment.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Library/BPUtilsNodeFunctionLibrary.h"
#include "Tests\ValidatorXTestHelpers.h"

namespace ValidatorX::Tests
{
	static UK2Node_CallFunction* AddCallFunctionNode(UEdGraph* Graph, UFunction* Function)
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXNodeUtilityNullCasesTest,
	"ValidatorX.Utils.Node.NullCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXNodeUtilityNullCasesTest::RunTest(const FString& Parameters)
{
	TArray<UClass*> DerivedClasses;
	UBPUtilsNodeFunctionLibrary::GetDerivedBlueprintClasses(nullptr, DerivedClasses);
	TestEqual(TEXT("GetDerivedBlueprintClasses ignores null parent"), DerivedClasses.Num(), 0);

	UBPUtilsNodeFunctionLibrary::GetAllDerivedBlueprintClasses(nullptr, DerivedClasses, false);
	TestEqual(TEXT("GetAllDerivedBlueprintClasses ignores null parent"), DerivedClasses.Num(), 0);

	TestFalse(TEXT("IsEmptyEvent returns false for null"), UBPUtilsNodeFunctionLibrary::IsEmptyEvent(nullptr));
	TestFalse(TEXT("IsEmptyFunctions returns false for null"), UBPUtilsNodeFunctionLibrary::IsEmptyFunctions(nullptr));
	TestFalse(TEXT("IsUnusedVariableGet returns false for null"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableGet(nullptr));
	TestFalse(TEXT("IsUnusedVariableSet returns false for null"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableSet(nullptr));
	TestFalse(TEXT("IsUnusedMacroInstance returns false for null"), UBPUtilsNodeFunctionLibrary::IsUnusedMacroInstance(nullptr));
	TestFalse(TEXT("IsEmptyPureFunction returns false for null"), UBPUtilsNodeFunctionLibrary::IsEmptyPureFunction(nullptr));
	TestFalse(TEXT("IsUnusedVariableNode returns false for non-variable nodes"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableNode(nullptr));
	TestFalse(TEXT("IsNodeInsideComment returns false for null node"), UBPUtilsNodeFunctionLibrary::IsNodeInsideComment(nullptr, {}));
	TestFalse(TEXT("IsBoolVariableSetInThisOrParentBPs returns false for null blueprint"), UBPUtilsNodeFunctionLibrary::IsBoolVariableSetInThisOrParentBPs(nullptr, TEXT("Flag")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXNodeUtilityCommentAndExecPinsTest,
	"ValidatorX.Utils.Node.CommentAndExecPins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXNodeUtilityCommentAndExecPinsTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_NodeUtilityBlueprint"));
	UEdGraph* Graph = AddGraph(Blueprint, Blueprint->UbergraphPages, TEXT("EventGraph"));
	UEdGraphNode* Node = AddNode<UEdGraphNode>(Graph);
	UEdGraphNode_Comment* Comment = AddNode<UEdGraphNode_Comment>(Graph);

	Comment->NodePosX = 0;
	Comment->NodePosY = 0;
	Comment->NodeWidth = 400;
	Comment->NodeHeight = 300;
	Node->NodePosX = 100;
	Node->NodePosY = 100;

	TArray<UEdGraphNode_Comment*> Comments = { Comment };
	TestTrue(TEXT("Node inside comment bounds is treated as inside comment"), UBPUtilsNodeFunctionLibrary::IsNodeInsideComment(Node, Comments));

	Node->NodePosX = 500;
	TestFalse(TEXT("Node outside comment bounds is not inside comment"), UBPUtilsNodeFunctionLibrary::IsNodeInsideComment(Node, Comments));

	UEdGraphPin* ExecOut = AddPin(Node, EGPD_Output, UEdGraphSchema_K2::PN_Then, UEdGraphSchema_K2::PC_Exec);
	TestFalse(TEXT("Unlinked output exec pin is not an execution output connection"), UBPUtilsNodeFunctionLibrary::HasExecutionOutputConnections(Node));

	UEdGraphNode* TargetNode = AddNode<UEdGraphNode>(Graph);
	UEdGraphPin* ExecIn = AddPin(TargetNode, EGPD_Input, UEdGraphSchema_K2::PN_Execute, UEdGraphSchema_K2::PC_Exec);
	LinkPins(ExecOut, ExecIn);
	TestTrue(TEXT("Linked output exec pin is detected"), UBPUtilsNodeFunctionLibrary::HasExecutionOutputConnections(Node));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXNodeUtilityGraphTypesTest,
	"ValidatorX.Utils.Node.GraphTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXNodeUtilityGraphTypesTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_GraphTypeBlueprint"));
	UEdGraph* FunctionGraph = AddGraph(Blueprint, Blueprint->FunctionGraphs, TEXT("FunctionGraph"));
	UEdGraph* MacroGraph = AddGraph(Blueprint, Blueprint->MacroGraphs, TEXT("MacroGraph"));
	UEdGraph* UberGraph = AddGraph(Blueprint, Blueprint->UbergraphPages, TEXT("UberGraph"));
	UEdGraph* DelegateGraph = AddGraph(Blueprint, Blueprint->DelegateSignatureGraphs, TEXT("DelegateGraph"));
	UEdGraph* IntermediateGraph = AddGraph(Blueprint, Blueprint->IntermediateGeneratedGraphs, TEXT("IntermediateGraph"));
	UEdGraph* UnknownGraph = NewObject<UEdGraph>(Blueprint);

	TestEqual(TEXT("Function graph type"), UBPUtilsNodeFunctionLibrary::GetGraphType(Blueprint, FunctionGraph), FString(TEXT("Function")));
	TestEqual(TEXT("Macro graph type"), UBPUtilsNodeFunctionLibrary::GetGraphType(Blueprint, MacroGraph), FString(TEXT("Macro")));
	TestEqual(TEXT("Event graph type"), UBPUtilsNodeFunctionLibrary::GetGraphType(Blueprint, UberGraph), FString(TEXT("Event Graph")));
	TestEqual(TEXT("Delegate graph type"), UBPUtilsNodeFunctionLibrary::GetGraphType(Blueprint, DelegateGraph), FString(TEXT("Delegate")));
	TestEqual(TEXT("Intermediate graph type"), UBPUtilsNodeFunctionLibrary::GetGraphType(Blueprint, IntermediateGraph), FString(TEXT("Intermediate")));
	TestEqual(TEXT("Unknown graph type"), UBPUtilsNodeFunctionLibrary::GetGraphType(Blueprint, UnknownGraph), FString(TEXT("Unknown")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXNodeUtilityVariableNodesTest,
	"ValidatorX.Utils.Node.VariableNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXNodeUtilityVariableNodesTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_VariableUtilityBlueprint"));
	UEdGraph* Graph = AddGraph(Blueprint, Blueprint->UbergraphPages, TEXT("EventGraph"));
	const FName VarName(TEXT("Flag"));

	UK2Node_VariableGet* VarGet = AddNode<UK2Node_VariableGet>(Graph);
	VarGet->VariableReference.SetSelfMember(VarName);
	UEdGraphPin* GetOutput = AddPin(VarGet, EGPD_Output, VarName, UEdGraphSchema_K2::PC_Boolean);
	TestTrue(TEXT("Unlinked variable get is unused"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableGet(VarGet));
	TestTrue(TEXT("IsUnusedVariableNode dispatches variable get"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableNode(VarGet));

	UEdGraphNode* GetTarget = AddNode<UEdGraphNode>(Graph);
	UEdGraphPin* GetTargetInput = AddPin(GetTarget, EGPD_Input, TEXT("Value"), UEdGraphSchema_K2::PC_Boolean);
	LinkPins(GetOutput, GetTargetInput);
	TestFalse(TEXT("Linked variable get is used"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableGet(VarGet));

	UK2Node_VariableSet* VarSet = AddNode<UK2Node_VariableSet>(Graph);
	VarSet->VariableReference.SetSelfMember(VarName);
	UEdGraphPin* ExecutePin = VarSet->FindPin(UEdGraphSchema_K2::PN_Execute);
	if(!ExecutePin)
	{
		ExecutePin = AddPin(VarSet, EGPD_Input, UEdGraphSchema_K2::PN_Execute, UEdGraphSchema_K2::PC_Exec);
	}
	TestTrue(TEXT("Unlinked variable set is unused"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableSet(VarSet));
	TestTrue(TEXT("IsUnusedVariableNode dispatches variable set"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableNode(VarSet));

	UEdGraphNode* SetSource = AddNode<UEdGraphNode>(Graph);
	UEdGraphPin* SetSourceOutput = AddPin(SetSource, EGPD_Output, UEdGraphSchema_K2::PN_Then, UEdGraphSchema_K2::PC_Exec);
	LinkPins(SetSourceOutput, ExecutePin);
	TestFalse(TEXT("Linked variable set is used"), UBPUtilsNodeFunctionLibrary::IsUnusedVariableSet(VarSet));

	FString SourceInfo;
	TestTrue(TEXT("Blueprint with matching set node reports variable set"), UBPUtilsNodeFunctionLibrary::IsBoolVariableSetInThisOrParentBPs(Blueprint, VarName, &SourceInfo));
	TestTrue(TEXT("Source info mentions the graph"), SourceInfo.Contains(Graph->GetName()));
	TestFalse(TEXT("Blueprint without matching set node reports missing variable set"), UBPUtilsNodeFunctionLibrary::IsBoolVariableSetInThisOrParentBPs(Blueprint, TEXT("MissingFlag")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXNodeUtilityBranchAndCallableNodesTest,
	"ValidatorX.Utils.Node.BranchAndCallableNodes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXNodeUtilityBranchAndCallableNodesTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_CallableUtilityBlueprint"));
	UEdGraph* Graph = AddGraph(Blueprint, Blueprint->UbergraphPages, TEXT("EventGraph"));

	UK2Node_IfThenElse* Branch = AddNode<UK2Node_IfThenElse>(Graph);
	TestTrue(TEXT("Branch with unlinked Then/Else pins has all branch execs disconnected"), UBPUtilsNodeFunctionLibrary::AreAllBranchExecsDisconnected(Branch));

	UEdGraphNode* BranchTarget = AddNode<UEdGraphNode>(Graph);
	UEdGraphPin* BranchTargetExec = AddPin(BranchTarget, EGPD_Input, UEdGraphSchema_K2::PN_Execute, UEdGraphSchema_K2::PC_Exec);
	LinkPins(Branch->GetThenPin(), BranchTargetExec);
	TestFalse(TEXT("Branch with linked Then pin is not fully disconnected"), UBPUtilsNodeFunctionLibrary::AreAllBranchExecsDisconnected(Branch));

	UK2Node_Event* EventNode = AddNode<UK2Node_Event>(Graph);
	UEdGraphPin* EventThen = EventNode->FindPin(UEdGraphSchema_K2::PN_Then);
	if(!EventThen)
	{
		EventThen = AddPin(EventNode, EGPD_Output, UEdGraphSchema_K2::PN_Then, UEdGraphSchema_K2::PC_Exec);
	}
	TestTrue(TEXT("Event with unlinked Then pin is empty"), UBPUtilsNodeFunctionLibrary::IsEmptyEvent(EventNode));

	UEdGraphNode* EventTarget = AddNode<UEdGraphNode>(Graph);
	UEdGraphPin* EventTargetExec = AddPin(EventTarget, EGPD_Input, UEdGraphSchema_K2::PN_Execute, UEdGraphSchema_K2::PC_Exec);
	LinkPins(EventThen, EventTargetExec);
	TestFalse(TEXT("Event with linked Then pin is not empty"), UBPUtilsNodeFunctionLibrary::IsEmptyEvent(EventNode));

	UK2Node_CallFunction* PrintStringNode = AddCallFunctionNode(Graph, UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString")));
	TestTrue(TEXT("Impure call with no execution output or return value links is empty"), UBPUtilsNodeFunctionLibrary::IsEmptyFunctions(PrintStringNode));

	UEdGraphPin* PrintExecutePin = PrintStringNode->FindPin(UEdGraphSchema_K2::PN_Execute);
	if(PrintExecutePin)
	{
		UEdGraphNode* PrintSource = AddNode<UEdGraphNode>(Graph);
		UEdGraphPin* PrintSourceExec = AddPin(PrintSource, EGPD_Output, UEdGraphSchema_K2::PN_Then, UEdGraphSchema_K2::PC_Exec);
		LinkPins(PrintSourceExec, PrintExecutePin);
	}
	TestFalse(TEXT("Impure call with linked execute pin is not empty"), UBPUtilsNodeFunctionLibrary::IsEmptyFunctions(PrintStringNode));

	UK2Node_CallFunction* PureNode = AddCallFunctionNode(Graph, UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("BooleanAND")));
	TestTrue(TEXT("Pure call with unlinked outputs is empty"), UBPUtilsNodeFunctionLibrary::IsEmptyPureFunction(PureNode));

	if(UEdGraphPin* PureOutput = PureNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue))
	{
		UEdGraphNode* PureTarget = AddNode<UEdGraphNode>(Graph);
		UEdGraphPin* PureTargetInput = AddPin(PureTarget, EGPD_Input, TEXT("Value"), UEdGraphSchema_K2::PC_Boolean);
		LinkPins(PureOutput, PureTargetInput);
	}
	TestFalse(TEXT("Pure call with linked output is not empty"), UBPUtilsNodeFunctionLibrary::IsEmptyPureFunction(PureNode));

	UK2Node_MacroInstance* MacroInstance = NewObject<UK2Node_MacroInstance>(Graph);
	Graph->AddNode(MacroInstance, false, false);
	MacroInstance->CreateNewGuid();
	UEdGraphPin* MacroPin = AddPin(MacroInstance, EGPD_Input, UEdGraphSchema_K2::PN_Execute, UEdGraphSchema_K2::PC_Exec);
	TestTrue(TEXT("Macro instance with no linked pins is unused"), UBPUtilsNodeFunctionLibrary::IsUnusedMacroInstance(MacroInstance));

	UEdGraphNode* MacroSource = AddNode<UEdGraphNode>(Graph);
	UEdGraphPin* MacroSourceExec = AddPin(MacroSource, EGPD_Output, UEdGraphSchema_K2::PN_Then, UEdGraphSchema_K2::PC_Exec);
	LinkPins(MacroSourceExec, MacroPin);
	TestFalse(TEXT("Macro instance with any linked pin is used"), UBPUtilsNodeFunctionLibrary::IsUnusedMacroInstance(MacroInstance));

	return true;
}

#endif
