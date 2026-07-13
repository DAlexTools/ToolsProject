// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EDataAssetValidationStatus : uint8
{
	Unknown,
	Valid,
	Warning,
	Invalid
};

struct FDataAssetValidationState final
{
	EDataAssetValidationStatus Status = EDataAssetValidationStatus::Unknown;
	FText Summary;
	int32 NumWarnings = 0;
	int32 NumErrors = 0;

	bool IsInvalid() const
	{
		return Status == EDataAssetValidationStatus::Invalid;
	}
};

struct FDataAssetValidationResults final
{
	TMap<FName, FDataAssetValidationState> StatesByPackage;
	TSet<FName> InvalidPackages;
};
