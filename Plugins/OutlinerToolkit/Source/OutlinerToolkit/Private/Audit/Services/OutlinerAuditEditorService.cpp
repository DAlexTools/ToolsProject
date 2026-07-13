// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Services/OutlinerAuditEditorService.h"
#include "Settings/OutlinerToolkitSettings.h"
#include "Columns/OutlinerColumnUtils.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Audit/Core/OutlinerAuditConstants.h"

void FOutlinerAuditEditorService::SelectActor(AActor* Actor)
{
	if (!GEditor || !IsValid(Actor))
	{
		return;
	}

	GEditor->SelectNone(false, true, false);
	GEditor->SelectActor(Actor, true, false, true);
	GEditor->NoteSelectionChange();
}

void FOutlinerAuditEditorService::SelectActors(const TArray<AActor*>& Actors)
{
	if (!GEditor)
	{
		return;
	}

	GEditor->SelectNone(false, true, false);
	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			GEditor->SelectActor(Actor, true, false, true);
		}
	}

	GEditor->NoteSelectionChange();
}

void FOutlinerAuditEditorService::SelectAndFocusActor(AActor* Actor)
{
	SelectActor(Actor);
	if (!GEditor || !IsValid(Actor))
	{
		return;
	}
	
	GEditor->MoveViewportCamerasToActor(*Actor, true);
}

void FOutlinerAuditEditorService::LoadPersistentIgnoredIssueKeys(TSet<FString>& IgnoredIssueKeys)
{
	IgnoredIssueKeys.Reset();

	const UOutlinerToolkitSettings* Settings = GetDefault<UOutlinerToolkitSettings>();
	if (!Settings)
	{
		return;
	}

	for (const FString& IgnoredIssueKey : Settings->IgnoredAuditIssueKeys)
	{
		if (!IgnoredIssueKey.IsEmpty())
		{
			IgnoredIssueKeys.Add(IgnoredIssueKey);
		}
	}
}

void FOutlinerAuditEditorService::SavePersistentIgnoredIssueKeys(const TSet<FString>& IgnoredIssueKeys)
{
	UOutlinerToolkitSettings* Settings = GetMutableDefault<UOutlinerToolkitSettings>();
	if (!Settings)
	{
		return;
	}

	Settings->IgnoredAuditIssueKeys = IgnoredIssueKeys.Array();
	Settings->IgnoredAuditIssueKeys.Sort();
	Settings->SaveConfig();
}

UObject* FOutlinerAuditEditorService::GetBlueprintAsset(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	const UClass* ActorClass = Actor->GetClass();
	return ActorClass ? ActorClass->ClassGeneratedBy : nullptr;
}

void FOutlinerAuditEditorService::GatherActorStaticMeshes(AActor* Actor, TArray<UStaticMesh*>& OutStaticMeshes)
{
	OutStaticMeshes.Reset();

	if (!IsValid(Actor))
	{
		return;
	}

	for (const UStaticMeshComponent* StaticMeshComponent : OutlinerColumnUtils::GetActorComponents<UStaticMeshComponent>(Actor))
	{
		if (!StaticMeshComponent)
		{
			continue;
		}

		if (UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
		{
			OutStaticMeshes.AddUnique(StaticMesh);
		}
	}
}

void FOutlinerAuditEditorService::OpenActorStaticMeshes(AActor* Actor)
{
	if (!IsValid(Actor) || !GEditor)
	{
		return;
	}

	TArray<UStaticMesh*> StaticMeshes;
	GatherActorStaticMeshes(Actor, StaticMeshes);

	if (StaticMeshes.IsEmpty())
	{
		return;
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();

	for (UStaticMesh* StaticMesh : StaticMeshes)
	{
		if (!StaticMesh)
		{
			continue;
		}

		if (AssetEditorSubsystem)
		{
			AssetEditorSubsystem->OpenEditorForAsset(StaticMesh);
		}
		else
		{
			GEditor->EditObject(StaticMesh);
		}
	}
}

void FOutlinerAuditEditorService::CopyToClipboard(const FString& Text)
{
	FPlatformApplicationMisc::ClipboardCopy(*Text);
}

void FOutlinerAuditEditorService::EnsureAuditFolderExists(UWorld* World, AActor* Actor)
{
	if (!World)
	{
		return;
	}

	FActorFolders::Get().CreateFolder(*World, FFolder(FFolder::GetInvalidRootObject(), OutlinerAudit::AuditFolderName));
	Actor->Modify();
	Actor->SetFolderPath(OutlinerAudit::AuditFolderName);
}

void FOutlinerAuditEditorService::RefreshOutliners(bool bFullRefresh)
{
	OutlinerColumnUtils::RefreshLevelEditorOutliners(bFullRefresh);
}
