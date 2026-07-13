// Copyright Epic Games, Inc. All Rights Reserved.

#include "Audit/Core/OutlinerAuditBitmask.h"
#include "Audit/Core/OutlinerAuditTypes.h"
#include "Audit/Engine/IOutlinerAuditRule.h"
#include "Audit/Engine/OutlinerAuditRules.h"
#include "Audit/Services/OutlinerAuditRunnerService.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace
{
	AActor* NewAuditTestActor(const TCHAR* Name)
	{
		return NewObject<AActor>(GetTransientPackage(), AActor::StaticClass(), FName(Name));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutlinerAuditContextAddsIgnoredIssueTest,
	"OutlinerToolkit.Audit.Context.AddIssue.RespectsIgnoredKeys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOutlinerAuditContextAddsIgnoredIssueTest::RunTest(const FString& Parameters)
{
	AActor* Actor = NewAuditTestActor(TEXT("AuditContextActor"));
	const FText Category = FText::FromString(TEXT("Category"));
	const FText Issue = FText::FromString(TEXT("Issue"));
	const FString ExpectedKey = FString::Printf(TEXT("%s|%s|%s"), *Actor->GetPathName(), *Category.ToString(), *Issue.ToString());

	TSet<FString> IgnoredKeys;
	IgnoredKeys.Add(ExpectedKey);

	TArray<FOutlinerAuditIssuePtr> Issues;
	FOutlinerAuditContext Context;
	Context.IgnoredIssueKeys = &IgnoredKeys;
	Context.OutIssues = &Issues;
	Context.EnabledCriteriaMask = FOutlinerAuditBitmask::CriterionToMask(EOutlinerAuditCriterion::TickEnabled);

	TestTrue(TEXT("Enabled criterion should be reported enabled"), Context.IsCriterionEnabled(EOutlinerAuditCriterion::TickEnabled));
	TestFalse(TEXT("Disabled criterion should be reported disabled"), Context.IsCriterionEnabled(EOutlinerAuditCriterion::NoFolder));

	Context.AddIssue(
		Actor,
		EOutlinerAuditSeverity::Warning,
		EOutlinerAuditFixAction::MoveToAuditFolder,
		Category,
		Issue,
		FText::FromString(TEXT("Details")));

	TestEqual(TEXT("Context should append one issue"), Issues.Num(), 1);
	TestTrue(TEXT("Issue pointer should be valid"), Issues[0].IsValid());
	TestEqual(TEXT("Issue key should be stable"), Issues[0]->Key, ExpectedKey);
	TestTrue(TEXT("Ignored keys should mark issue ignored"), Issues[0]->bIgnored);
	TestEqual(TEXT("Issue severity should be stored"), Issues[0]->Severity, EOutlinerAuditSeverity::Warning);
	TestEqual(TEXT("Fix action should be stored"), Issues[0]->FixAction, EOutlinerAuditFixAction::MoveToAuditFolder);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutlinerAuditRuleSetBuildsRulesTest,
	"OutlinerToolkit.Audit.Rules.BuildDefaultRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOutlinerAuditRuleSetBuildsRulesTest::RunTest(const FString& Parameters)
{
	TArray<TUniquePtr<IOutlinerAuditRule>> Rules;
	FOutlinerAuditRuleSet::BuildDefaultRules(Rules);

	TestTrue(TEXT("Default rule set should contain rules"), Rules.Num() > 0);
	for (const TUniquePtr<IOutlinerAuditRule>& Rule : Rules)
	{
		TestTrue(TEXT("Rule entry should be valid"), Rule.IsValid());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOutlinerAuditRunnerGroupsActorResultsTest,
	"OutlinerToolkit.Audit.Runner.RebuildActorResults.GroupsAndCountsIssues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOutlinerAuditRunnerGroupsActorResultsTest::RunTest(const FString& Parameters)
{
	AActor* FirstActor = NewAuditTestActor(TEXT("AuditRunnerActorA"));
	AActor* SecondActor = NewAuditTestActor(TEXT("AuditRunnerActorB"));

	TArray<FOutlinerAuditIssuePtr> Issues;

	FOutlinerAuditIssuePtr FirstWarning = MakeShared<FOutlinerAuditIssue>();
	FirstWarning->Actor = FirstActor;
	FirstWarning->Severity = EOutlinerAuditSeverity::Warning;
	FirstWarning->FixAction = EOutlinerAuditFixAction::MoveToAuditFolder;
	FirstWarning->Issue = FText::FromString(TEXT("Warning"));
	FirstWarning->Key = TEXT("FirstWarning");
	Issues.Add(FirstWarning);

	FOutlinerAuditIssuePtr FirstInfoIgnored = MakeShared<FOutlinerAuditIssue>();
	FirstInfoIgnored->Actor = FirstActor;
	FirstInfoIgnored->Severity = EOutlinerAuditSeverity::Info;
	FirstInfoIgnored->Issue = FText::FromString(TEXT("Info"));
	FirstInfoIgnored->Key = TEXT("FirstInfo");
	FirstInfoIgnored->bIgnored = true;
	Issues.Add(FirstInfoIgnored);

	FOutlinerAuditIssuePtr SecondError = MakeShared<FOutlinerAuditIssue>();
	SecondError->Actor = SecondActor;
	SecondError->Severity = EOutlinerAuditSeverity::Error;
	SecondError->Issue = FText::FromString(TEXT("Error"));
	SecondError->Key = TEXT("SecondError");
	Issues.Add(SecondError);

	TArray<FOutlinerAuditActorResultPtr> Results;
	FOutlinerAuditRunnerService::RebuildActorResults(Issues, Results);

	TestEqual(TEXT("Issues should be grouped by actor"), Results.Num(), 2);
	TestTrue(TEXT("Error actor should sort before warning actor"), Results[0]->Actor.Get() == SecondActor);
	TestEqual(TEXT("First actor should retain two issues"), Results[1]->Issues.Num(), 2);
	TestEqual(TEXT("First actor should count fixable issues"), Results[1]->FixableIssueCount, 1);
	TestEqual(TEXT("First actor should count ignored issues"), Results[1]->IgnoredIssueCount, 1);
	TestEqual(TEXT("First actor worst severity should be warning"), Results[1]->WorstSeverity, EOutlinerAuditSeverity::Warning);

	return true;
}

#endif
