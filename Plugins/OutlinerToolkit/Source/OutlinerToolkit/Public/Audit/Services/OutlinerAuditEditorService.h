// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class OUTLINERTOOLKIT_API FOutlinerAuditEditorService final 
{
public:
	static void SelectActor(AActor* Actor);
	static void SelectActors(const TArray<AActor*>& Actors);
	static void SelectAndFocusActor(AActor* Actor);
	static void SavePersistentIgnoredIssueKeys(const TSet<FString>& IgnoredIssueKeys);
	static void LoadPersistentIgnoredIssueKeys(TSet<FString>& IgnoredIssueKeys);
	static UObject* GetBlueprintAsset(const AActor* Actor);
	static void GatherActorStaticMeshes(AActor* Actor, TArray<UStaticMesh*>& OutStaticMeshes);
	static void OpenActorStaticMeshes(AActor* Actor);
	static void CopyToClipboard(const FString& Text);
	static void EnsureAuditFolderExists(UWorld* World, AActor* Actor);
	static void RefreshOutliners(bool bFullRefresh);

};
