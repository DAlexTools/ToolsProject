// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditTypes.h"

/**
 * 
 */
class OUTLINERTOOLKIT_API FOutlinerAuditActorHelpers final
{
public:
	static UWorld* GetEditorWorld();
	static FString GetComponentDisplayName(const UActorComponent* Component);
	static TArray<AActor*> ExtractActors(const TArray<FOutlinerAuditActorResultPtr>& Results);
};
	