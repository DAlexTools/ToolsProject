// Copyright (c) 2026 DimAlek. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Audit/Core/OutlinerAuditTypes.h"
#include "OutlinerToolkitSettings.generated.h"



/**
 *
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Outliner Toolkit"))
class OUTLINERTOOLKIT_API UOutlinerToolkitSettings final : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	static const UOutlinerToolkitSettings* Get();

	static UOutlinerToolkitSettings* GetMutable();
public:

	UPROPERTY(EditAnywhere, config, Category = "Settings", meta = (DisplayName = "Material Slot Count Parameter"))
	int32 TooManyMaterialSlotsThreshold = 5;

	UPROPERTY(EditAnywhere, config, Category = "Settings", meta = (DisplayName = "Component Slot Count Parameter"))
	int32 TooManyComponentsThreshold = 20;

	UPROPERTY(EditAnywhere, config, Category = "Settings", meta = (DisplayName = "Scale Tolerance Parameter"))
	float ScaleUniformTolerance = 0.01f;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Tick Enabled"))
	EOutlinerAuditSeverityOverride TickEnabledSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "No Folder"))
	EOutlinerAuditSeverityOverride NoFolderSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Editor Only"))
	EOutlinerAuditSeverityOverride EditorOnlySeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Too Many Components"))
	EOutlinerAuditSeverityOverride TooManyComponentsSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Bad Actor Scale"))
	EOutlinerAuditSeverityOverride BadActorScaleSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Bad Component Scale"))
	EOutlinerAuditSeverityOverride BadComponentScaleSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Physics Enabled"))
	EOutlinerAuditSeverityOverride PhysicsEnabledSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Invalid Physics Mobility"))
	EOutlinerAuditSeverityOverride InvalidPhysicsMobilitySeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Overlap Events"))
	EOutlinerAuditSeverityOverride OverlapEventsSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Movable Shadows"))
	EOutlinerAuditSeverityOverride MovableShadowsSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Invalid Static Mesh"))
	EOutlinerAuditSeverityOverride InvalidStaticMeshSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Invalid Materials"))
	EOutlinerAuditSeverityOverride InvalidMaterialsSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(EditAnywhere, config, Category = "Audit Severity Overrides", meta = (DisplayName = "Too Many Materials"))
	EOutlinerAuditSeverityOverride TooManyMaterialsSeverity = EOutlinerAuditSeverityOverride::UseDefault;

	UPROPERTY(config)
	TArray<FString> IgnoredAuditIssueKeys;
};
