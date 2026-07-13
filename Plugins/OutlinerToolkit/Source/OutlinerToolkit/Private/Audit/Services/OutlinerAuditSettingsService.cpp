// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Services/OutlinerAuditSettingsService.h"

EOutlinerAuditSeverity FOutlinerAuditSettingsService::GetIssueSeverity(const UOutlinerToolkitSettings* Settings, EOutlinerAuditCriterion Criterion, EOutlinerAuditSeverity DefaultSeverity)
{
	if (!Settings)
	{
		return DefaultSeverity;
	}

	switch (Criterion)
	{
	case EOutlinerAuditCriterion::TickEnabled:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->TickEnabledSeverity);
	}

	case EOutlinerAuditCriterion::NoFolder:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->NoFolderSeverity);
	}

	case EOutlinerAuditCriterion::EditorOnly:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->EditorOnlySeverity);
	}

	case EOutlinerAuditCriterion::TooManyComponents:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->TooManyComponentsSeverity);
	}

	case EOutlinerAuditCriterion::BadActorScale:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->BadActorScaleSeverity);
	}

	case EOutlinerAuditCriterion::BadComponentScale:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->BadComponentScaleSeverity);
	}

	case EOutlinerAuditCriterion::PhysicsEnabled:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->PhysicsEnabledSeverity);
	}

	case EOutlinerAuditCriterion::InvalidPhysicsMobility:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->InvalidPhysicsMobilitySeverity);
	}

	case EOutlinerAuditCriterion::OverlapEvents:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->OverlapEventsSeverity);
	}

	case EOutlinerAuditCriterion::MovableShadows:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->MovableShadowsSeverity);
	}

	case EOutlinerAuditCriterion::InvalidStaticMesh:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->InvalidStaticMeshSeverity);
	}

	case EOutlinerAuditCriterion::InvalidMaterials:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->InvalidMaterialsSeverity);
	}

	case EOutlinerAuditCriterion::TooManyMaterials:
	{
		return ApplySeverityOverride(DefaultSeverity, Settings->TooManyMaterialsSeverity);
	}

	case EOutlinerAuditCriterion::Count:
	default:
	{
		return DefaultSeverity;
	}
	}
}

EOutlinerAuditSeverity FOutlinerAuditSettingsService::ApplySeverityOverride(EOutlinerAuditSeverity DefaultSeverity, EOutlinerAuditSeverityOverride Override)
{
	switch (Override)
	{
		case EOutlinerAuditSeverityOverride::Info:
		{
			return EOutlinerAuditSeverity::Info;
		}

		case EOutlinerAuditSeverityOverride::Warning:
		{
			return EOutlinerAuditSeverity::Warning;
		}

		case EOutlinerAuditSeverityOverride::Error:
		{
			return EOutlinerAuditSeverity::Error;
		}

		case EOutlinerAuditSeverityOverride::UseDefault:
		default:
		{
			return DefaultSeverity;
		}
	}
}

