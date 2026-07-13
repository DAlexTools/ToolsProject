// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditEnums.h"	
/**
 * 
 */
class OUTLINERTOOLKIT_API FOutlinerAuditBitmask final
{
public:
	[[nodiscard]] static uint32 CriterionToMask(EOutlinerAuditCriterion Criterion);
	[[nodiscard]] static uint8 SeverityToMask(EOutlinerAuditSeverity Severity);
};
