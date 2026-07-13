#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BaseClasses\BlueprintValidatorBase.h"
#include "BaseClasses/MaterialValidatorBase.h"
#include "Logging/TokenizedMessage.h"
#include "Tests\ValidatorXTestHelpers.h"
#include "Validation/BlueprintValidatorActionHelpers.h"
#include "ValidatorXManager.h"
#include "Validators/BranchConditionValidator.h"
#include "Validators/TwoSidedMaterialValidator.h"

namespace ValidatorX::Tests
{
	class FScopedValidatorStateRestore
	{
	public:
		void Capture(UValidatorXBase* Validator)
		{
			if(!Validator)
			{
				return;
			}

			UClass* ValidatorClass = Validator->GetClass();
			if(!OriginalStates.Contains(ValidatorClass))
			{
				OriginalStates.Add(ValidatorClass, Validator->IsEnabled());
			}
		}

		~FScopedValidatorStateRestore()
		{
			for(const TPair<UClass*, bool>& Pair : OriginalStates)
			{
				if(UClass* ValidatorClass = Pair.Key)
				{
					if(UValidatorXBase* CDO = ValidatorClass->GetDefaultObject<UValidatorXBase>())
					{
						CDO->SetValidationEnabled(Pair.Value);
					}
				}
			}
		}

	private:
		TMap<UClass*, bool> OriginalStates;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXBaseCanValidateAssetTest,
	"ValidatorX.Base.CanValidateAssetAndType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXBaseCanValidateAssetTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBranchConditionValidator* BlueprintValidator = NewValidator<UBranchConditionValidator>();
	UTwoSidedMaterialValidator* MaterialValidator = NewValidator<UTwoSidedMaterialValidator>();
	UBlueprint* Blueprint = NewTransientBlueprint();
	UMaterial* Material = NewTransientMaterial();
	FDataValidationContext Context;

	TestEqual(TEXT("Blueprint validators report their type"), BlueprintValidator->GetTypeValidator(), FString(TEXT("Blueprint")));
	TestEqual(TEXT("Material validators report their type"), MaterialValidator->GetTypeValidator(), FString(TEXT("Material")));
	TestTrue(TEXT("Blueprint validator accepts UBlueprint"), BlueprintValidator->CanValidateAsset_Implementation(MakeAssetData(Blueprint), Blueprint, Context));
	TestFalse(TEXT("Blueprint validator rejects non-blueprint asset"), BlueprintValidator->CanValidateAsset_Implementation(MakeAssetData(Material), Material, Context));
	TestTrue(TEXT("Material validator accepts UMaterial"), MaterialValidator->CanValidateAsset_Implementation(MakeAssetData(Material), Material, Context));
	TestFalse(TEXT("Material validator rejects non-material asset"), MaterialValidator->CanValidateAsset_Implementation(MakeAssetData(Blueprint), Blueprint, Context));
	TestFalse(TEXT("Material validator rejects null asset"), MaterialValidator->CanValidateAsset_Implementation(FAssetData(), nullptr, Context));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXBaseToggleTest,
	"ValidatorX.Base.ToggleValidationEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXBaseToggleTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBranchConditionValidator* Validator = NewValidator<UBranchConditionValidator>();
	FScopedValidatorStateRestore Restore;
	Restore.Capture(Validator);

	Validator->SetValidationEnabled(true);
	TestTrue(TEXT("Validator starts enabled after explicit enable"), Validator->IsEnabled());

	Validator->SetValidationEnabled(false);
	TestFalse(TEXT("SetValidationEnabled(false) disables validator"), Validator->IsEnabled());

	Validator->ToggleValidationEnabled();
	TestTrue(TEXT("ToggleValidationEnabled re-enables disabled validator"), Validator->IsEnabled());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXManagerTest,
	"ValidatorX.Manager.RegisterAndSetAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXManagerTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	FValidatorXManager& Manager = FValidatorXManager::Get();
	FScopedValidatorStateRestore Restore;
	for(const TWeakObjectPtr<UValidatorXBase>& ExistingValidator : Manager.GetValidators())
	{
		Restore.Capture(ExistingValidator.Get());
	}

	UBranchConditionValidator* Validator = NewValidator<UBranchConditionValidator>(TEXT("ValidatorX_ManagerTestValidator"));
	Restore.Capture(Validator);

	const int32 InitialCount = Manager.GetValidators().Num();
	Manager.RegisterValidator(nullptr);
	TestEqual(TEXT("RegisterValidator ignores null"), Manager.GetValidators().Num(), InitialCount);

	Manager.RegisterValidator(Validator);
	TestEqual(TEXT("RegisterValidator adds a new validator"), Manager.GetValidators().Num(), InitialCount + 1);

	Manager.RegisterValidator(Validator);
	TestEqual(TEXT("RegisterValidator does not duplicate the same object"), Manager.GetValidators().Num(), InitialCount + 1);

	Manager.SetAllValidatorsEnabled(false);
	TestFalse(TEXT("SetAllValidatorsEnabled(false) disables registered validator"), Validator->IsEnabled());

	Manager.SetAllValidatorsEnabled(true);
	TestTrue(TEXT("SetAllValidatorsEnabled(true) enables registered validator"), Validator->IsEnabled());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXActionHelpersTest,
	"ValidatorX.Actions.AddActionTokens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXActionHelpersTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Warning, INVTEXT("Test message"));
	const int32 InitialTokenCount = Message->GetMessageTokens().Num();

	ValidatorX::Actions::AddAction(Message, INVTEXT("Action"), FSimpleDelegate::CreateLambda([] {}));
	TestEqual(TEXT("AddAction appends one token"), Message->GetMessageTokens().Num(), InitialTokenCount + 1);

	ValidatorX::Actions::AddOpenAssetAction(Message, INVTEXT("Open"), nullptr);
	TestEqual(TEXT("AddOpenAssetAction appends one token"), Message->GetMessageTokens().Num(), InitialTokenCount + 2);

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_ActionHelperBlueprint"));
	UEdGraph* Graph = AddGraph(Blueprint, Blueprint->UbergraphPages, TEXT("ActionHelperGraph"));
	UEdGraphNode* Node = AddNode<UEdGraphNode>(Graph);

	ValidatorX::Actions::AddJumpToGraphAction(Message, INVTEXT("JumpGraph"), Blueprint, Graph);
	ValidatorX::Actions::AddJumpToBlueprintItemAction(Message, INVTEXT("JumpItem"), Blueprint, FName(TEXT("Item")));
	ValidatorX::Actions::AddJumpToGraphItemAction(Message, INVTEXT("JumpGraphItem"), Blueprint, Graph, FName(TEXT("Item")));
	ValidatorX::Actions::AddJumpToNodeAction(Message, INVTEXT("JumpNode"), Blueprint, Graph, Node);

	TestEqual(TEXT("Jump helper methods append action tokens"), Message->GetMessageTokens().Num(), InitialTokenCount + 6);
	TestNull(TEXT("OpenAsset returns null for null assets"), ValidatorX::Actions::OpenAsset(nullptr));
	TestNull(TEXT("FindBlueprintEditor returns null for null blueprints"), ValidatorX::Actions::FindBlueprintEditor(nullptr));
	TestNull(TEXT("OpenBlueprintEditor returns null for null blueprints"), ValidatorX::Actions::OpenBlueprintEditor(nullptr));

	return true;
}

#endif
