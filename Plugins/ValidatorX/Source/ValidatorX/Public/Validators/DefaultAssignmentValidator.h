// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseClasses/BlueprintValidatorBase.h"
#include "DefaultAssignmentValidator.generated.h"

/**
 * @class UDefaultAssignmentValidator
 * @brief Blueprint asset validator that detects redundant variable assignments.
 *
 * This validator scans Blueprint graphs and looks for variable assignment nodes
 * (Set nodes) where a variable is explicitly assigned the same value that is already
 * defined as its default value in the Blueprint class.
 *
 * Such assignments are considered redundant because they do not change runtime state
 * and may indicate unnecessary logic that can be safely removed.
 *
 * Validation issues are reported through Unreal Engine's Data Validation framework,
 * with a direct navigation action allowing the user to jump to the problematic node.
 */
UCLASS()
class VALIDATORX_API UDefaultAssignmentValidator : public UBlueprintValidatorBase
{
	GENERATED_BODY()
public:
	UDefaultAssignmentValidator();

	/**
	 * @brief Validates a Blueprint asset for redundant variable assignments.
	 *
	 * Iterates through all Blueprint graphs and searches for UK2Node_VariableSet nodes.
	 * For each variable assignment node, compares the assigned pin default value with
	 * the property's default value stored in the Blueprint's Class Default Object (CDO).
	 *
	 * If both values are identical and the input pin has no connections, the assignment
	 * is considered redundant and a validation warning is reported.
	 *
	 * @param InAssetData Metadata describing the asset being validated.
	 * @param InAsset Loaded asset object expected to be a Blueprint.
	 * @param Context Validation context used for reporting warnings and attaching actions.
	 *
	 * @return EDataValidationResult::Invalid if redundant assignments are found,
	 *         otherwise EDataValidationResult::Valid.
	 */
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

};
