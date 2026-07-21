// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Misc/DataValidation.h"
#include "Misc/AutomationTest.h"
#include "UI/SDataAssetManagerWidget.h"
#include "Widgets/Views/SHeaderRow.h"

#include "DataAssetManagerTestTypes.generated.h"

/**
 * @brief Shared automation-test flags used by Data Asset Manager test cases.
 */
namespace DataAssetManagerFlags
{
	/** @brief Default flag set for editor automation tests in this module. */
	constexpr EAutomationTestFlags Flags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ProductFilter |
		EAutomationTestFlags::CriticalPriority;
}

/**
 * @brief Minimal data asset type used by Data Asset Manager automation tests.
 */
UCLASS(BlueprintType)
class UTestDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief Integer property used by tests that need editable asset data. */
	UPROPERTY(EditDefaultsOnly)
	int32 TestProperty = 0;

	UPROPERTY(EditDefaultsOnly)
	FLinearColor Color = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly)
	TArray<FLinearColor> Colors;
};

/**
 * @brief Alternate data asset class used by tests that need class mismatches.
 */
UCLASS(BlueprintType)
class UAlternateTestDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief Distinct editable property used by class-filter and diff tests. */
	UPROPERTY(EditDefaultsOnly)
	FString AlternateProperty;
};

/**
 * @brief Data asset type that always fails validation for service tests.
 */
UCLASS(BlueprintType)
class UInvalidTestDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override
	{
		Context.AddError(NSLOCTEXT("DataAssetManagerTests", "InvalidTestDataAssetError", "Invalid test data asset."));
		return EDataValidationResult::Invalid;
	}
#endif
};

#if WITH_DEV_AUTOMATION_TESTS

/**
 * @brief Test wrapper that exposes selected protected SDataAssetManagerWidget methods.
 */
class STestDataAssetManagerWidget final : public SDataAssetManagerWidget
{
public:
	/** @brief Base widget type used by this test wrapper. */
	using Super = SDataAssetManagerWidget;

	/**
	 * @brief Constructs the test widget.
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs)
	{
		Super::Construct(InArgs);
	}

	/**
	 * @brief Exposes AddColumnToHeader for automation tests.
	 * @param InHeaderRow Header row that receives the column.
	 * @param ColumnId Unique column identifier.
	 * @param Label Display label for the column.
	 * @param FillWidth Relative fill width for the column.
	 */
	void AddColumnToHeader_Test(
		TSharedPtr<SHeaderRow> InHeaderRow,
		const FName& ColumnId,
		const FString& Label,
		float FillWidth)
	{
		Super::AddColumnToHeader(InHeaderRow, ColumnId, Label, FillWidth);
	}

	/**
	 * @brief Exposes CreateRevisionControlColumn for automation tests.
	 * @return Column arguments for the revision-control column.
	 */
	SHeaderRow::FColumn::FArguments CreateRevisionControlColumn_Test()
	{
		return Super::CreateRevisionControlColumn();
	}

	/**
	 * @brief Exposes IsSelectedAssetValid for automation tests.
	 * @param CustomMessage Message to include in validation feedback.
	 * @return true when the current selected asset state is valid.
	 */
	bool IsSelectedAssetValid_Test(const FString& CustomMessage) const
	{
		return Super::IsSelectedAssetValid(CustomMessage);
	}
};

#endif
