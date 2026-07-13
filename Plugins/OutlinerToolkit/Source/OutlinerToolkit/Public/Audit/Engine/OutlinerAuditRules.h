// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class IOutlinerAuditRule;

class OUTLINERTOOLKIT_API FOutlinerAuditRuleSet final
{
public:
	static void BuildDefaultRules(TArray<TUniquePtr<IOutlinerAuditRule>>& OutRules);
};
