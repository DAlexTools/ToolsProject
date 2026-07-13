// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Engine/OutlinerAuditActorHelpers.h"

UWorld* FOutlinerAuditActorHelpers::GetEditorWorld()
{
	if (GEditor)
	{
		if (UWorld* EditorWorld = GEditor->GetEditorWorldContext().World())
		{
			return EditorWorld;
		}
	}

	if (GEngine)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Editor)
			{
				return WorldContext.World();
			}
		}
	}

	return nullptr;
}

TArray<AActor*> FOutlinerAuditActorHelpers::ExtractActors(const TArray<FOutlinerAuditActorResultPtr>& Results)
{
	TArray<AActor*> Actors;
	Actors.Reserve(Results.Num());

	for (const FOutlinerAuditActorResultPtr& Result : Results)
	{
		if (Result.IsValid() && Result->Actor.IsValid())
		{
			Actors.Add(Result->Actor.Get());
		}
	}

	return Actors;
}


FString FOutlinerAuditActorHelpers::GetComponentDisplayName(const UActorComponent* Component)
{
	if (!Component)
	{
		return TEXT("<invalid component>");
	}

	const UClass* ComponentClass = Component->GetClass();

	return FString::Printf(TEXT("%s (%s)"), *Component->GetName(),ComponentClass 
		? *ComponentClass->GetName()
		: TEXT("UnknownClass"));
}