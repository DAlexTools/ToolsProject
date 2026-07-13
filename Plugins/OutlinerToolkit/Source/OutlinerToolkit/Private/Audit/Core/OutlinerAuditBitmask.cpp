// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Core/OutlinerAuditBitmask.h"

uint32 FOutlinerAuditBitmask::CriterionToMask(EOutlinerAuditCriterion Criterion)
{
	return 1u << static_cast<uint8>(Criterion);
}

uint8 FOutlinerAuditBitmask::SeverityToMask(EOutlinerAuditSeverity Severity)
{
	return static_cast<uint8>(1u << static_cast<uint8>(Severity));
}