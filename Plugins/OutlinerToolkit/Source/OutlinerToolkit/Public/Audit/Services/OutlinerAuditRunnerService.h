// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditEnums.h"
#include "Audit/Core/OutlinerAuditTypes.h"

class UOutlinerToolkitSettings;

struct FOutlinerAuditRunRequest final
{
	EOutlinerAuditScope Scope = EOutlinerAuditScope::WholeWorld;
	uint32 EnabledCriteriaMask = OutlinerAuditAllCriteriaMask;
	const UOutlinerToolkitSettings* Settings = nullptr;
	const TSet<FString>* IgnoredIssueKeys = nullptr;
};

struct FOutlinerAuditRunResult final
{
	TArray<FOutlinerAuditIssuePtr> Issues;
	int32 AuditedActorCount = 0;
};

class OUTLINERTOOLKIT_API FOutlinerAuditRunnerService final
{
public:
	static FOutlinerAuditRunResult RunAudit(const FOutlinerAuditRunRequest& Request);
	static void GatherActorsForScope(EOutlinerAuditScope Scope, TArray<AActor*>& OutActors);
	static void SortIssues(TArray<FOutlinerAuditIssuePtr>& Issues);
	static void RebuildActorResults(const TArray<FOutlinerAuditIssuePtr>& Issues, TArray<FOutlinerAuditActorResultPtr>& OutActorResults);
	static bool ApplyFix(const FOutlinerAuditIssue& Issue);
};
