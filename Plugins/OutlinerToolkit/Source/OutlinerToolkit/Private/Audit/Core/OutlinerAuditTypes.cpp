// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Core/OutlinerAuditTypes.h"
#include "Audit/Core/OutlinerAuditBitmask.h"
#include "Audit/Core/OutlinerAuditUtils.h"
#include "Audit/Services/OutlinerAuditSettingsService.h"
#include "GameFramework/Actor.h"

bool FOutlinerAuditContext::IsCriterionEnabled(EOutlinerAuditCriterion Criterion) const
{
	return (EnabledCriteriaMask & FOutlinerAuditBitmask::CriterionToMask(Criterion)) != 0;
}

EOutlinerAuditSeverity FOutlinerAuditContext::GetIssueSeverity(EOutlinerAuditCriterion Criterion, EOutlinerAuditSeverity DefaultSeverity) const
{
	return FOutlinerAuditSettingsService::GetIssueSeverity(Settings, Criterion, DefaultSeverity);
}

void FOutlinerAuditContext::AddIssue(AActor* Actor, EOutlinerAuditSeverity Severity, EOutlinerAuditFixAction FixAction, const FText& Category, const FText& Issue, const FText& Details, const TArray<FOutlinerAuditDetailEntry>& DetailEntries)
{
	if (!IsValid(Actor) || !OutIssues)
	{
		return;
	}

	const FString Key = OutlinerAuditUtils::MakeIssueKey(Actor, Category, Issue);

	FOutlinerAuditIssuePtr AuditIssue = MakeShared<FOutlinerAuditIssue>();
	AuditIssue->Actor = Actor;
	AuditIssue->Severity = Severity;
	AuditIssue->FixAction = FixAction;
	AuditIssue->Category = Category;
	AuditIssue->Issue = Issue;
	AuditIssue->Details = Details;
	AuditIssue->DetailEntries = DetailEntries;
	AuditIssue->Key = Key;
	AuditIssue->bIgnored = IgnoredIssueKeys && IgnoredIssueKeys->Contains(Key);
	OutIssues->Add(AuditIssue);
}
