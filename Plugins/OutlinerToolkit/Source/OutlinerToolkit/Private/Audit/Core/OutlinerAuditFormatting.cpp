// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Core/OutlinerAuditFormatting.h"
#include "Audit/Core/OutlinerAuditUtils.h"
#include "Audit/Core/OutlinerAuditConstants.h"

#define LOCTEXT_NAMESPACE "FOutlinerAuditReportFormatter"

FText FOutlinerAuditReportFormatter::GetSeverityText(EOutlinerAuditSeverity Severity)
{
	switch (Severity)
	{
		case EOutlinerAuditSeverity::Error:
		{
			return LOCTEXT("SeverityError", "Error");
		}

		case EOutlinerAuditSeverity::Warning:
		{
			return LOCTEXT("SeverityWarning", "Warning");
		}

		case EOutlinerAuditSeverity::Info:
		default:
		{
			return LOCTEXT("SeverityInfo", "Info");
		}
	}
}

FSlateColor FOutlinerAuditReportFormatter::GetSeverityColor(EOutlinerAuditSeverity Severity)
{
	switch (Severity)
	{
		case EOutlinerAuditSeverity::Error:
		{
			return OutlinerAudit::ErrorColor;
		}

		case EOutlinerAuditSeverity::Warning:
		{
			return OutlinerAudit::WarningColor;
		}

		case EOutlinerAuditSeverity::Info:
		default:
		{
			return OutlinerAudit::InfoColor;
		}
	}
}

FText FOutlinerAuditReportFormatter::GetActorLabelText(const TWeakObjectPtr<AActor>& Actor)
{
	return Actor.IsValid() 
		? FText::FromString(Actor->GetActorLabel())
		: LOCTEXT("InvalidActorLabel", "<invalid>");
}

FString FOutlinerAuditReportFormatter::FormatScale(const FVector& Scale)
{
	return FString::Printf(TEXT("Scale: X=%.3f Y=%.3f Z=%.3f"), Scale.X, Scale.Y, Scale.Z);
}


int32 FOutlinerAuditReportFormatter::GetSeveritySortRank(EOutlinerAuditSeverity Severity)
{
	switch (Severity)
	{
		case EOutlinerAuditSeverity::Error:
		{
			return 0;
		}

		case EOutlinerAuditSeverity::Warning:
		{
			return 1;
		}

		case EOutlinerAuditSeverity::Info:
		default:
		{
			return 2;
		}
	}
}

FText FOutlinerAuditReportFormatter::GetAuditScopeText(EOutlinerAuditScope Scope)
{
	switch (Scope)
	{
		case EOutlinerAuditScope::SelectedActors:
		{
			return LOCTEXT("SelectedActorsScope", "Selected Actors");
		}

		case EOutlinerAuditScope::CurrentLevel:
		{
			return LOCTEXT("CurrentLevelScope", "Current Level");
		}

		case EOutlinerAuditScope::VisibleActors:
		{
			return LOCTEXT("VisibleActorsScope", "Visible Actors");
		}

		case EOutlinerAuditScope::WholeWorld:
		default:
		{
			return LOCTEXT("WholeWorldScope", "Whole World");
		}
	}
}

FText FOutlinerAuditReportFormatter::GetDetailsPanelIssueText(FOutlinerAuditActorResultPtr SelectedActorResult)
{
	if (!SelectedActorResult.IsValid())
	{
		return FText::GetEmpty();
	}

	return FText::Format(LOCTEXT("DetailsPanelActorIssueFormat",
		"Worst: {0} / {1} visible issues / {2} fixable"),
		GetSeverityText(SelectedActorResult->WorstSeverity),
		FText::AsNumber(SelectedActorResult->Issues.Num()),
		FText::AsNumber(SelectedActorResult->FixableIssueCount));
}

FString FOutlinerAuditReportFormatter::GetMobilityName(EComponentMobility::Type Mobility)
{
	switch (Mobility)
	{
		case EComponentMobility::Static:
		{
			return TEXT("Static");
		}

		case EComponentMobility::Stationary:
		{
			return TEXT("Stationary");
		}

		case EComponentMobility::Movable:
		{
			return TEXT("Movable");
		}

		default:
		{
			return TEXT("Unknown");
		}
	}
}

FText FOutlinerAuditReportFormatter::GetDetailsPanelSummaryText(const FOutlinerAuditActorResultPtr& ActorResultPtr)
{
	if (!ActorResultPtr.IsValid())
	{
		return FText::GetEmpty();
	}

	int32 SelectedErrorCount = 0;
	int32 SelectedWarningCount = 0;
	int32 SelectedInfoCount = 0;
	int32 SelectedIgnoredCount = 0;
	for (const FOutlinerAuditIssuePtr& Issue : ActorResultPtr->Issues)
	{
		if (!Issue.IsValid())
		{
			continue;
		}

		switch (Issue->Severity)
		{
		case EOutlinerAuditSeverity::Error:
			++SelectedErrorCount;
			break;

		case EOutlinerAuditSeverity::Warning:
			++SelectedWarningCount;
			break;

		case EOutlinerAuditSeverity::Info:
			++SelectedInfoCount;
			break;
		}

		if (Issue->bIgnored)
		{
			++SelectedIgnoredCount;
		}
	}

	return FText::Format(LOCTEXT("DetailsPanelActorSeverityBreakdown",
		"{0} errors, {1} warnings, {2} info, {3} ignored"),
		FText::AsNumber(SelectedErrorCount),
		FText::AsNumber(SelectedWarningCount),
		FText::AsNumber(SelectedInfoCount),
		FText::AsNumber(SelectedIgnoredCount));
}


FString FOutlinerAuditReportFormatter::BuildActorIssuesReport(const FOutlinerAuditActorResult& ActorResult)
{
	const FString ActorLabel = ActorResult.Actor.IsValid()
		? ActorResult.Actor->GetActorLabel()
		: TEXT("<invalid>");

	const FString ActorPath = ActorResult.Actor.IsValid()
		? ActorResult.Actor->GetPathName()
		: TEXT("<invalid>");

	FString Report = FString::Printf(
		TEXT("Actor: %s\r\nPath: %s\r\nWorst Severity: %s\r\nVisible Issues: %d\r\nFixable Issues: %d"),
		*ActorLabel,
		*ActorPath,
		*GetSeverityText(ActorResult.WorstSeverity).ToString(),
		ActorResult.Issues.Num(),
		ActorResult.FixableIssueCount);

	for (const FOutlinerAuditIssuePtr& Issue : ActorResult.Issues)
	{
		if (!Issue.IsValid())
		{
			continue;
		}

		Report += FString::Printf(
			TEXT("\r\n\r\n[%s%s] %s / %s\r\n%s"),
			*GetSeverityText(Issue->Severity).ToString(),
			Issue->bIgnored ? TEXT(", ignored") : TEXT(""),
			*Issue->Category.ToString(),
			*Issue->Issue.ToString(),
			*Issue->Details.ToString());

		for (const FOutlinerAuditDetailEntry& Detail : Issue->DetailEntries)
		{
			Report += FString::Printf(
				TEXT("\r\n- %s: %s"),
				*Detail.Subject.ToString(),
				*Detail.Detail.ToString());
		}
	}

	return Report;
}






#undef LOCTEXT_NAMESPACE