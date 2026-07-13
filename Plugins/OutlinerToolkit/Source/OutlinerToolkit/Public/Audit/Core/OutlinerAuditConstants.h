// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


namespace OutlinerAuditColumns
{
	static const FName Severity(TEXT("Severity"));
	static const FName Actor(TEXT("Actor"));
	static const FName Category(TEXT("Category"));
	static const FName Issue(TEXT("Issue"));
	static const FName Details(TEXT("Details"));
	static const FName Fix(TEXT("Fix"));
}

namespace OutlinerAudit
{
	const FName AuditFolderName(TEXT("Audit_Unsorted"));

	static const FLinearColor ErrorColor(0.95f, 0.25f, 0.22f);
	static const FLinearColor WarningColor(1.0f, 0.65f, 0.18f);
	static const FLinearColor InfoColor(0.45f, 0.68f, 1.0f);
}
