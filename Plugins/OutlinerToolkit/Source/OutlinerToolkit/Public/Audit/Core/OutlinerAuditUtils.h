// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditTypes.h"

enum class EOutlinerAuditSeverity : uint8;
enum class EOutlinerAuditScope : uint8;
class UOutlinerToolkitSettings;

namespace OutlinerAuditUtils
{


	FString MakeIssueKey(const AActor* Actor, const FText& Category, const FText& Issue);
	int32 CountEnabledStringFilterOptions(const TArray<TSharedPtr<FString>>& Options, const TSet<FString>& EnabledValues);
	[[nodiscard]] bool IsNonUniformOrNegativeScale(const FVector& Scale, float ScaleUniformTolerance);


	FOutlinerAuditDetailEntry MakeDetailEntry(const FString& Subject, const FString& Detail);


	

}