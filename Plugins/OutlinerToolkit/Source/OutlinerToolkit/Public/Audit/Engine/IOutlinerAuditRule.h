// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FOutlinerAuditIssue;
struct FOutlinerAuditContext;

/**
 *
 */
class OUTLINERTOOLKIT_API IOutlinerAuditRule
{
public:
	virtual ~IOutlinerAuditRule() = default;

	virtual void Execute(AActor* Actor, FOutlinerAuditContext& Context) = 0;
};
