// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditEnums.h"

class UOutlinerToolkitSettings;

struct FOutlinerAuditDetailEntry
{
	FText Subject;
	FText Detail;
};

/**
 * @brief
 */
static constexpr uint8 OutlinerAuditAllSeverityFilterMask = static_cast<uint8>(
	(1u << static_cast<uint8>(EOutlinerAuditSeverity::Info)) |
	(1u << static_cast<uint8>(EOutlinerAuditSeverity::Warning)) |
	(1u << static_cast<uint8>(EOutlinerAuditSeverity::Error)));

/**
 * @brief
 */
static constexpr uint32 OutlinerAuditAllCriteriaMask =
(1u << static_cast<uint8>(EOutlinerAuditCriterion::Count)) - 1u;

/**
 * @brief
 */
struct FOutlinerAuditIssue
{
	TWeakObjectPtr<AActor> Actor;
	EOutlinerAuditSeverity Severity = EOutlinerAuditSeverity::Info;
	EOutlinerAuditFixAction FixAction = EOutlinerAuditFixAction::None;
	FText Category;
	FText Issue;
	FText Details;
	TArray<FOutlinerAuditDetailEntry> DetailEntries;
	FString Key;
	bool bIgnored = false;
};

/**
 * @brief
 */
using FOutlinerAuditIssuePtr = TSharedPtr<FOutlinerAuditIssue>;
	
/**
 * @brief
 */
struct FOutlinerAuditActorResult
{
	TWeakObjectPtr<AActor> Actor;
	TArray<FOutlinerAuditIssuePtr> Issues;
	EOutlinerAuditSeverity WorstSeverity = EOutlinerAuditSeverity::Info;
	int32 FixableIssueCount = 0;
	int32 IgnoredIssueCount = 0;
};

/**
 * @brief
 */
using FOutlinerAuditActorResultPtr = TSharedPtr<FOutlinerAuditActorResult>;

struct OUTLINERTOOLKIT_API FOutlinerAuditContext final
{
	const UOutlinerToolkitSettings* Settings = nullptr;
	const TSet<FString>* IgnoredIssueKeys = nullptr;
	TArray<FOutlinerAuditIssuePtr>* OutIssues = nullptr;
	uint32 EnabledCriteriaMask = OutlinerAuditAllCriteriaMask;

	bool IsCriterionEnabled(EOutlinerAuditCriterion Criterion) const;
	EOutlinerAuditSeverity GetIssueSeverity(EOutlinerAuditCriterion Criterion, EOutlinerAuditSeverity DefaultSeverity) const;
	void AddIssue(AActor* Actor, EOutlinerAuditSeverity Severity, EOutlinerAuditFixAction FixAction, const FText& Category, const FText& Issue, const FText& Details, const TArray<FOutlinerAuditDetailEntry>& DetailEntries = TArray<FOutlinerAuditDetailEntry>());
};
