// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum class EOutlinerAuditSeverityOverride : uint8
{
	UseDefault UMETA(DisplayName = "Default"),
	Info	   UMETA(DisplayName = "Info"),
	Warning	   UMETA(DisplayName = "Warning"),
	Error	   UMETA(DisplayName = "Error")
};

enum class EOutlinerAuditSeverity : uint8
{
	Info,
	Warning,
	Error
};

enum class EOutlinerAuditFixAction : uint8
{
	None,
	MoveToAuditFolder,
	SetSimulatedPhysicsMovable
};

enum class EOutlinerAuditCriterion : uint8
{
	TickEnabled,
	NoFolder,
	EditorOnly,
	TooManyComponents,
	BadActorScale,
	BadComponentScale,
	PhysicsEnabled,
	InvalidPhysicsMobility,
	OverlapEvents,
	MovableShadows,
	InvalidStaticMesh,
	InvalidMaterials,
	TooManyMaterials,
	Count
};

enum class EOutlinerAuditScope : uint8
{
	SelectedActors,
	CurrentLevel,
	VisibleActors,
	WholeWorld
};