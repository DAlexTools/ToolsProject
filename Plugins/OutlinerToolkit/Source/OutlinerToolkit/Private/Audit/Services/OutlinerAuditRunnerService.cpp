// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Services/OutlinerAuditRunnerService.h"
#include "Audit/Core/OutlinerAuditFormatting.h"
#include "Audit/Engine/IOutlinerAuditRule.h"
#include "Audit/Engine/OutlinerAuditActorHelpers.h"
#include "Audit/Engine/OutlinerAuditRules.h"
#include "Audit/Services/OutlinerAuditEditorService.h"
#include "Columns/OutlinerColumnUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Settings/OutlinerToolkitSettings.h"

FOutlinerAuditRunResult FOutlinerAuditRunnerService::RunAudit(const FOutlinerAuditRunRequest& Request)
{
	FOutlinerAuditRunResult Result;
	if ((Request.EnabledCriteriaMask & OutlinerAuditAllCriteriaMask) == 0)
	{
		return Result;
	}

	TArray<AActor*> ActorsToAudit;
	GatherActorsForScope(Request.Scope, ActorsToAudit);
	Result.AuditedActorCount = ActorsToAudit.Num();

	const UOutlinerToolkitSettings* Settings = Request.Settings ? Request.Settings : UOutlinerToolkitSettings::Get();
	if (!Settings)
	{
		return Result;
	}

	FOutlinerAuditContext Context;
	Context.Settings = Settings;
	Context.IgnoredIssueKeys = Request.IgnoredIssueKeys;
	Context.OutIssues = &Result.Issues;
	Context.EnabledCriteriaMask = Request.EnabledCriteriaMask;

	TArray<TUniquePtr<IOutlinerAuditRule>> Rules;
	FOutlinerAuditRuleSet::BuildDefaultRules(Rules);

	for (AActor* Actor : ActorsToAudit)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		for (const TUniquePtr<IOutlinerAuditRule>& Rule : Rules)
		{
			if (Rule.IsValid())
			{
				Rule->Execute(Actor, Context);
			}
		}
	}

	SortIssues(Result.Issues);
	return Result;
}

void FOutlinerAuditRunnerService::GatherActorsForScope(EOutlinerAuditScope Scope, TArray<AActor*>& OutActors)
{
	OutActors.Reset();

	const UWorld* EditorWorld = FOutlinerAuditActorHelpers::GetEditorWorld();
	if (Scope == EOutlinerAuditScope::SelectedActors)
	{
		if (!GEditor)
		{
			return;
		}

		for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
		{
			AActor* Actor = Cast<AActor>(*It);
			if (IsValid(Actor) && (!EditorWorld || Actor->GetWorld() == EditorWorld))
			{
				OutActors.AddUnique(Actor);
			}
		}

		return;
	}

	if (!EditorWorld)
	{
		return;
	}

	for (TActorIterator<AActor> ActorIt(EditorWorld); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Scope == EOutlinerAuditScope::CurrentLevel && Actor->GetLevel() != EditorWorld->GetCurrentLevel())
		{
			continue;
		}

		if (Scope == EOutlinerAuditScope::VisibleActors && (Actor->IsHiddenEd() || Actor->IsTemporarilyHiddenInEditor()))
		{
			continue;
		}

		OutActors.Add(Actor);
	}
}

void FOutlinerAuditRunnerService::SortIssues(TArray<FOutlinerAuditIssuePtr>& Issues)
{
	Issues.StableSort([](const FOutlinerAuditIssuePtr& Left, const FOutlinerAuditIssuePtr& Right)
	{
		if (!Left.IsValid() || !Right.IsValid())
		{
			return Left.IsValid();
		}

		const int32 LeftSeverity = FOutlinerAuditReportFormatter::GetSeveritySortRank(Left->Severity);
		const int32 RightSeverity = FOutlinerAuditReportFormatter::GetSeveritySortRank(Right->Severity);
		if (LeftSeverity != RightSeverity)
		{
			return LeftSeverity < RightSeverity;
		}

		const FString LeftActor = Left->Actor.IsValid() ? Left->Actor->GetActorLabel() : FString();
		const FString RightActor = Right->Actor.IsValid() ? Right->Actor->GetActorLabel() : FString();
		if (LeftActor != RightActor)
		{
			return LeftActor < RightActor;
		}

		return Left->Issue.ToString() < Right->Issue.ToString();
	});
}

void FOutlinerAuditRunnerService::RebuildActorResults(const TArray<FOutlinerAuditIssuePtr>& Issues, TArray<FOutlinerAuditActorResultPtr>& OutActorResults)
{
	OutActorResults.Reset();

	TMap<FString, FOutlinerAuditActorResultPtr> ResultsByActorPath;
	for (const FOutlinerAuditIssuePtr& Issue : Issues)
	{
		if (!Issue.IsValid())
		{
			continue;
		}

		const FString ActorPath = Issue->Actor.IsValid()
			? Issue->Actor->GetPathName()
			: FString::Printf(TEXT("<invalid>|%s"), *Issue->Key);

		FOutlinerAuditActorResultPtr* ExistingResult = ResultsByActorPath.Find(ActorPath);
		FOutlinerAuditActorResultPtr ActorResult;
		if (ExistingResult)
		{
			ActorResult = *ExistingResult;
		}
		else
		{
			ActorResult = MakeShared<FOutlinerAuditActorResult>();
			ActorResult->Actor = Issue->Actor;
			ResultsByActorPath.Add(ActorPath, ActorResult);
			OutActorResults.Add(ActorResult);
		}

		ActorResult->Issues.Add(Issue);
		if (FOutlinerAuditReportFormatter::GetSeveritySortRank(Issue->Severity) < FOutlinerAuditReportFormatter::GetSeveritySortRank(ActorResult->WorstSeverity))
		{
			ActorResult->WorstSeverity = Issue->Severity;
		}

		if (Issue->FixAction != EOutlinerAuditFixAction::None)
		{
			++ActorResult->FixableIssueCount;
		}

		if (Issue->bIgnored)
		{
			++ActorResult->IgnoredIssueCount;
		}
	}

	OutActorResults.StableSort([](const FOutlinerAuditActorResultPtr& Left, const FOutlinerAuditActorResultPtr& Right)
	{
		if (!Left.IsValid() || !Right.IsValid())
		{
			return Left.IsValid();
		}

		const int32 LeftSeverity = FOutlinerAuditReportFormatter::GetSeveritySortRank(Left->WorstSeverity);
		const int32 RightSeverity = FOutlinerAuditReportFormatter::GetSeveritySortRank(Right->WorstSeverity);
		if (LeftSeverity != RightSeverity)
		{
			return LeftSeverity < RightSeverity;
		}

		const FString LeftActor = Left->Actor.IsValid() ? Left->Actor->GetActorLabel() : FString();
		const FString RightActor = Right->Actor.IsValid() ? Right->Actor->GetActorLabel() : FString();
		return LeftActor < RightActor;
	});
}

bool FOutlinerAuditRunnerService::ApplyFix(const FOutlinerAuditIssue& Issue)
{
	AActor* Actor = Issue.Actor.Get();
	if (!IsValid(Actor))
	{
		return false;
	}

	switch (Issue.FixAction)
	{
		case EOutlinerAuditFixAction::MoveToAuditFolder:
		{
			UWorld* World = Actor->GetWorld();
			FOutlinerAuditEditorService::EnsureAuditFolderExists(World, Actor);
			return true;
		}

		case EOutlinerAuditFixAction::SetSimulatedPhysicsMovable:
		{
			bool bChanged = false;
			for (UPrimitiveComponent* PrimitiveComponent : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
			{
				if (PrimitiveComponent && PrimitiveComponent->IsSimulatingPhysics() && PrimitiveComponent->Mobility != EComponentMobility::Movable)
				{
					PrimitiveComponent->Modify();
					PrimitiveComponent->SetMobility(EComponentMobility::Movable);
					bChanged = true;
				}
			}
			return bChanged;
		}

		case EOutlinerAuditFixAction::None:
		default:
			break;
	}

	return false;
}
