// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Core/OutlinerAuditUtils.h"
#include "Audit/Core/OutlinerAuditEnums.h"
#include "Audit/Core/OutlinerAuditConstants.h"
#include "Audit/Core/OutlinerAuditTypes.h"
#include "Settings/OutlinerToolkitSettings.h"

#define LOCTEXT_NAMESPACE "OutlinerAuditUtils"

namespace OutlinerAuditUtils
{
	FString MakeIssueKey(const AActor* Actor, const FText& Category, const FText& Issue)
	{
		return FString::Printf(TEXT("%s|%s|%s"), Actor ? *Actor->GetPathName() : TEXT("<invalid>"), *Category.ToString(), *Issue.ToString());
	}

	int32 CountEnabledStringFilterOptions(const TArray<TSharedPtr<FString>>& Options, const TSet<FString>& EnabledValues)
	{
		int32 EnabledCount = 0;
		for (const TSharedPtr<FString>& Option : Options)
		{
			if (Option.IsValid() && EnabledValues.Contains(*Option))
			{
				++EnabledCount;
			}
		}

		return EnabledCount;
	}



	bool IsNonUniformOrNegativeScale(const FVector& Scale, float ScaleUniformTolerance)
	{
		return Scale.X < 0.0f
			|| Scale.Y < 0.0f
			|| Scale.Z < 0.0f
			|| !FMath::IsNearlyEqual(Scale.X, Scale.Y, ScaleUniformTolerance)
			|| !FMath::IsNearlyEqual(Scale.X, Scale.Z, ScaleUniformTolerance);
	}


	FOutlinerAuditDetailEntry MakeDetailEntry(const FString& Subject, const FString& Detail)
	{
		FOutlinerAuditDetailEntry Entry;
		Entry.Subject = FText::FromString(Subject);
		Entry.Detail = FText::FromString(Detail);
		return Entry;
	}
	

}

#undef LOCTEXT_NAMESPACE