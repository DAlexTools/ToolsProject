// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditTypes.h"

/**
 *
 */
class OUTLINERTOOLKIT_API FOutlinerAuditReportFormatter final
{
public:
	static FText GetSeverityText(EOutlinerAuditSeverity Severity);
	static FSlateColor GetSeverityColor(EOutlinerAuditSeverity Severity);
	static int32 GetSeveritySortRank(EOutlinerAuditSeverity Severity);
	static FText GetAuditScopeText(EOutlinerAuditScope Scope);
	static FText GetDetailsPanelIssueText(FOutlinerAuditActorResultPtr SelectedActorResult);
	static FString GetMobilityName(EComponentMobility::Type Mobility);
	static FText GetActorLabelText(const TWeakObjectPtr<AActor>& Actor);
	static FString FormatScale(const FVector& Scale);
	static FText GetDetailsPanelSummaryText(const FOutlinerAuditActorResultPtr& ActorResultPtr);
	static FString BuildActorIssuesReport(const FOutlinerAuditActorResult& ActorResult);
};
