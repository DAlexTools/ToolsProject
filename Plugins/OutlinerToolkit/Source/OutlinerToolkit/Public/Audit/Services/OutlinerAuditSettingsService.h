// Copyright (c) 2026 DimAlek. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditEnums.h"
#include "Settings/OutlinerToolkitSettings.h"

/**
 * 
 */
class OUTLINERTOOLKIT_API FOutlinerAuditSettingsService final 
{
public:
	static EOutlinerAuditSeverity GetIssueSeverity(const UOutlinerToolkitSettings* Settings, EOutlinerAuditCriterion Criterion, EOutlinerAuditSeverity DefaultSeverity);

private:
	static EOutlinerAuditSeverity ApplySeverityOverride(EOutlinerAuditSeverity DefaultSeverity, EOutlinerAuditSeverityOverride Override);
};
