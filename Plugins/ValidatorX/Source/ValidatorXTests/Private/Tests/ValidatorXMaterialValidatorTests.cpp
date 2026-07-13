#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Tests\ValidatorXTestHelpers.h"
#include "Validators/MaterialPositionOffsetValidator.h"
#include "Validators/TextureSampleCountMaterialValidator.h"
#include "Validators/TranslucentMaterialValidator.h"
#include "Validators/TwoSidedMaterialValidator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXMaterialValidatorsIgnoreNonMaterialsTest,
	"ValidatorX.Validators.Material.IgnoreNonMaterials",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXMaterialValidatorsIgnoreNonMaterialsTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UBlueprint* Blueprint = NewTransientBlueprint(TEXT("ValidatorX_NonMaterialBlueprint"));

	TestEqual(TEXT("TwoSided validator treats non-materials as valid"), Validate(NewValidator<UTwoSidedMaterialValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("Translucent validator treats non-materials as valid"), Validate(NewValidator<UTranslucentMaterialValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("Texture sample count validator treats non-materials as valid"), Validate(NewValidator<UTextureSampleCountMaterialValidator>(), Blueprint), EDataValidationResult::Valid);
	TestEqual(TEXT("Position offset validator treats non-materials as valid"), Validate(NewValidator<UMaterialPositionOffsetValidator>(), Blueprint), EDataValidationResult::Valid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXTwoSidedMaterialValidatorTest,
	"ValidatorX.Validators.Material.TwoSided",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXTwoSidedMaterialValidatorTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UTwoSidedMaterialValidator* Validator = NewValidator<UTwoSidedMaterialValidator>();
	UMaterial* Material = NewTransientMaterial(TEXT("ValidatorX_TwoSidedMaterial"));

	Material->TwoSided = false;
	TestEqual(TEXT("Single-sided material is valid"), Validate(Validator, Material), EDataValidationResult::Valid);

	Material->TwoSided = true;
	TestEqual(TEXT("Two-sided material is invalid"), Validate(Validator, Material), EDataValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXTranslucentMaterialValidatorTest,
	"ValidatorX.Validators.Material.TranslucentBlendModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXTranslucentMaterialValidatorTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UTranslucentMaterialValidator* Validator = NewValidator<UTranslucentMaterialValidator>();
	UMaterial* Material = NewTransientMaterial(TEXT("ValidatorX_TranslucentMaterial"));

	Material->BlendMode = BLEND_Opaque;
	TestEqual(TEXT("Opaque material is valid"), Validate(Validator, Material), EDataValidationResult::Valid);

	Material->BlendMode = BLEND_Masked;
	TestEqual(TEXT("Masked material is valid"), Validate(Validator, Material), EDataValidationResult::Valid);

	Material->BlendMode = BLEND_Translucent;
	TestEqual(TEXT("Translucent material is invalid"), Validate(Validator, Material), EDataValidationResult::Invalid);

	Material->BlendMode = BLEND_Additive;
	TestEqual(TEXT("Additive material is invalid"), Validate(Validator, Material), EDataValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXTextureSampleCountMaterialValidatorTest,
	"ValidatorX.Validators.Material.TextureSampleCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXTextureSampleCountMaterialValidatorTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UTextureSampleCountMaterialValidator* Validator = NewValidator<UTextureSampleCountMaterialValidator>();
	UMaterial* Material = NewTransientMaterial(TEXT("ValidatorX_TextureSampleMaterial"));

	for(int32 Index = 0; Index < 16; ++Index)
	{
		UMaterialExpressionTextureSample* Sample = NewObject<UMaterialExpressionTextureSample>(Material);
		Sample->Material = Material;
		Material->GetExpressionCollection().AddExpression(Sample);
	}

	TestEqual(TEXT("Material at texture sample limit is valid"), Validate(Validator, Material), EDataValidationResult::Valid);

	UMaterialExpressionTextureSample* ExtraSample = NewObject<UMaterialExpressionTextureSample>(Material);
	ExtraSample->Material = Material;
	Material->GetExpressionCollection().AddExpression(ExtraSample);

	TestEqual(TEXT("Material above texture sample limit is invalid"), Validate(Validator, Material), EDataValidationResult::Invalid);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FValidatorXMaterialPositionOffsetValidatorTest,
	"ValidatorX.Validators.Material.PositionOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FValidatorXMaterialPositionOffsetValidatorTest::RunTest(const FString& Parameters)
{
	using namespace ValidatorX::Tests;

	UMaterialPositionOffsetValidator* Validator = NewValidator<UMaterialPositionOffsetValidator>();
	UMaterial* Material = NewTransientMaterial(TEXT("ValidatorX_PositionOffsetMaterial"));

	TestEqual(TEXT("Material without position offsets is valid"), Validate(Validator, Material), EDataValidationResult::Valid);

	UMaterialExpressionConstant* Expression = NewObject<UMaterialExpressionConstant>(Material);
	Expression->Material = Material;
	Material->GetExpressionCollection().AddExpression(Expression);

	if(FExpressionInput* WorldPositionOffset = const_cast<FExpressionInput*>(Material->GetExpressionInputForProperty(MP_WorldPositionOffset)))
	{
		WorldPositionOffset->Expression = Expression;
	}

	TestEqual(TEXT("Material with world position offset is invalid"), Validate(Validator, Material), EDataValidationResult::Invalid);

	return true;
}

#endif
