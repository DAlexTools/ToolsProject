// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Engine/OutlinerAuditRules.h"
#include "Audit/Core/OutlinerAuditFormatting.h"
#include "Audit/Core/OutlinerAuditTypes.h"
#include "Audit/Core/OutlinerAuditUtils.h"
#include "Audit/Engine/IOutlinerAuditRule.h"
#include "Audit/Engine/OutlinerAuditActorHelpers.h"
#include "Columns/OutlinerColumnUtils.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Settings/OutlinerToolkitSettings.h"

#define LOCTEXT_NAMESPACE "OutlinerAuditRules"

namespace
{
	class FOutlinerActorAuditRule final : public IOutlinerAuditRule
	{
	public:
		virtual void Execute(AActor* Actor, FOutlinerAuditContext& Context) override
		{
			if (!IsValid(Actor) || !Context.Settings)
			{
				return;
			}

			const UOutlinerToolkitSettings* Settings = Context.Settings;

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::TickEnabled) && Actor->PrimaryActorTick.bCanEverTick && Actor->IsActorTickEnabled())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::TickEnabled, EOutlinerAuditSeverity::Warning),
					EOutlinerAuditFixAction::None,
					LOCTEXT("PerformanceCategory", "Performance"),
					LOCTEXT("TickEnabledIssue", "Tick Enabled"),
					LOCTEXT("TickEnabledDetails", "Actor tick is enabled. Review whether this actor needs per-frame updates."));
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::NoFolder) && Actor->GetFolderPath().IsNone())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::NoFolder, EOutlinerAuditSeverity::Info),
					EOutlinerAuditFixAction::MoveToAuditFolder,
					LOCTEXT("OrganizationCategory", "Organization"),
					LOCTEXT("NoFolderIssue", "No Folder"),
					LOCTEXT("NoFolderDetails", "Actor is not assigned to an Outliner folder."));
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::EditorOnly) && Actor->IsEditorOnly())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::EditorOnly, EOutlinerAuditSeverity::Info),
					EOutlinerAuditFixAction::None,
					LOCTEXT("PackagingCategory", "Packaging"),
					LOCTEXT("EditorOnlyIssue", "Editor Only"),
					LOCTEXT("EditorOnlyDetails", "Actor is marked as editor-only and will not be cooked into runtime builds."));
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::TooManyComponents))
			{
				TArray<UActorComponent*> ActorComponents;
				Actor->GetComponents(ActorComponents);
				if (ActorComponents.Num() > Settings->TooManyComponentsThreshold)
				{
					TArray<FOutlinerAuditDetailEntry> ComponentDetails;
					ComponentDetails.Reserve(ActorComponents.Num());
					for (const UActorComponent* ActorComponent : ActorComponents)
					{
						ComponentDetails.Add(OutlinerAuditUtils::MakeDetailEntry(
							FOutlinerAuditActorHelpers::GetComponentDisplayName(ActorComponent),
							ActorComponent ? FString::Printf(TEXT("Path: %s"), *ActorComponent->GetPathName()) : FString(TEXT("Invalid component"))));
					}

					Context.AddIssue(
						Actor,
						Context.GetIssueSeverity(EOutlinerAuditCriterion::TooManyComponents, EOutlinerAuditSeverity::Warning),
						EOutlinerAuditFixAction::None,
						LOCTEXT("ComplexityCategory", "Complexity"),
						LOCTEXT("TooManyComponentsIssue", "Too Many Components"),
						FText::Format(
							LOCTEXT("TooManyComponentsDetails", "{0} components found. Current threshold is {1}."),
							FText::AsNumber(ActorComponents.Num()),
							FText::AsNumber(Settings->TooManyComponentsThreshold)),
						ComponentDetails);
				}
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::BadActorScale) && OutlinerAuditUtils::IsNonUniformOrNegativeScale(Actor->GetActorScale3D(), Settings->ScaleUniformTolerance))
			{
				TArray<FOutlinerAuditDetailEntry> ActorScaleDetails;
				ActorScaleDetails.Add(OutlinerAuditUtils::MakeDetailEntry(Actor->GetActorLabel(), FOutlinerAuditReportFormatter::FormatScale(Actor->GetActorScale3D())));

				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::BadActorScale, EOutlinerAuditSeverity::Warning),
					EOutlinerAuditFixAction::None,
					LOCTEXT("TransformCategory", "Transform"),
					LOCTEXT("BadActorScaleIssue", "Bad Actor Scale"),
					LOCTEXT("BadActorScaleDetails", "Actor scale is non-uniform or negative."),
					ActorScaleDetails);
			}
		}
	};

	class FOutlinerComponentScaleAuditRule final : public IOutlinerAuditRule
	{
	public:
		virtual void Execute(AActor* Actor, FOutlinerAuditContext& Context) override
		{
			if (!IsValid(Actor) || !Context.Settings || !Context.IsCriterionEnabled(EOutlinerAuditCriterion::BadComponentScale))
			{
				return;
			}

			TArray<FOutlinerAuditDetailEntry> BadComponentScaleDetails;
			for (const USceneComponent* SceneComponent : OutlinerColumnUtils::GetActorComponents<USceneComponent>(Actor))
			{
				if (SceneComponent && OutlinerAuditUtils::IsNonUniformOrNegativeScale(SceneComponent->GetRelativeScale3D(), Context.Settings->ScaleUniformTolerance))
				{
					BadComponentScaleDetails.Add(OutlinerAuditUtils::MakeDetailEntry(
						FOutlinerAuditActorHelpers::GetComponentDisplayName(SceneComponent),
						FOutlinerAuditReportFormatter::FormatScale(SceneComponent->GetRelativeScale3D())));
				}
			}

			if (!BadComponentScaleDetails.IsEmpty())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::BadComponentScale, EOutlinerAuditSeverity::Warning),
					EOutlinerAuditFixAction::None,
					LOCTEXT("ComponentTransformCategory", "Transform"),
					LOCTEXT("BadComponentScaleIssue", "Bad Component Scale"),
					FText::Format(
						LOCTEXT("BadComponentScaleDetails", "{0} scene components have non-uniform or negative relative scale."),
						FText::AsNumber(BadComponentScaleDetails.Num())),
					BadComponentScaleDetails);
			}
		}
	};

	class FOutlinerPrimitiveComponentAuditRule final : public IOutlinerAuditRule
	{
	public:
		virtual void Execute(AActor* Actor, FOutlinerAuditContext& Context) override
		{
			if (!IsValid(Actor) || !Context.Settings)
			{
				return;
			}

			TArray<FOutlinerAuditDetailEntry> SimulatingPhysicsDetails;
			TArray<FOutlinerAuditDetailEntry> InvalidPhysicsMobilityDetails;
			TArray<FOutlinerAuditDetailEntry> OverlapEventsDetails;
			TArray<FOutlinerAuditDetailEntry> MovableShadowDetails;

			for (const UPrimitiveComponent* PrimitiveComponent : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
			{
				if (!PrimitiveComponent)
				{
					continue;
				}

				if (PrimitiveComponent->IsSimulatingPhysics())
				{
					SimulatingPhysicsDetails.Add(OutlinerAuditUtils::MakeDetailEntry(
						FOutlinerAuditActorHelpers::GetComponentDisplayName(PrimitiveComponent),
						TEXT("Simulates physics")));

					if (PrimitiveComponent->Mobility != EComponentMobility::Movable)
					{
						InvalidPhysicsMobilityDetails.Add(OutlinerAuditUtils::MakeDetailEntry(
							FOutlinerAuditActorHelpers::GetComponentDisplayName(PrimitiveComponent),
							FString::Printf(TEXT("Mobility: %s"), *FOutlinerAuditReportFormatter::GetMobilityName(PrimitiveComponent->Mobility))));
					}
				}

				if (PrimitiveComponent->GetGenerateOverlapEvents())
				{
					OverlapEventsDetails.Add(OutlinerAuditUtils::MakeDetailEntry(
						FOutlinerAuditActorHelpers::GetComponentDisplayName(PrimitiveComponent),
						TEXT("Generate overlap events: true")));
				}

				if (PrimitiveComponent->Mobility == EComponentMobility::Movable && PrimitiveComponent->CastShadow)
				{
					MovableShadowDetails.Add(OutlinerAuditUtils::MakeDetailEntry(
						FOutlinerAuditActorHelpers::GetComponentDisplayName(PrimitiveComponent),
						TEXT("Movable component casts shadows")));
				}
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::PhysicsEnabled) && !SimulatingPhysicsDetails.IsEmpty())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::PhysicsEnabled, EOutlinerAuditSeverity::Warning),
					EOutlinerAuditFixAction::None,
					LOCTEXT("PhysicsCategory", "Physics"),
					LOCTEXT("PhysicsEnabledIssue", "Physics Enabled"),
					FText::Format(LOCTEXT("PhysicsEnabledDetails", "{0} primitive components simulate physics."), FText::AsNumber(SimulatingPhysicsDetails.Num())),
					SimulatingPhysicsDetails);
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::InvalidPhysicsMobility) && !InvalidPhysicsMobilityDetails.IsEmpty())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::InvalidPhysicsMobility, EOutlinerAuditSeverity::Error),
					EOutlinerAuditFixAction::SetSimulatedPhysicsMovable,
					LOCTEXT("InvalidPhysicsCategory", "Physics"),
					LOCTEXT("InvalidPhysicsMobilityIssue", "Invalid Physics Mobility"),
					FText::Format(LOCTEXT("InvalidPhysicsMobilityDetails", "{0} simulating primitive components are not Movable."), FText::AsNumber(InvalidPhysicsMobilityDetails.Num())),
					InvalidPhysicsMobilityDetails);
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::OverlapEvents) && !OverlapEventsDetails.IsEmpty())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::OverlapEvents, EOutlinerAuditSeverity::Warning),
					EOutlinerAuditFixAction::None,
					LOCTEXT("CollisionCategory", "Collision"),
					LOCTEXT("OverlapEventsIssue", "Overlap Events Enabled"),
					FText::Format(LOCTEXT("OverlapEventsDetails", "{0} primitive components generate overlap events."), FText::AsNumber(OverlapEventsDetails.Num())),
					OverlapEventsDetails);
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::MovableShadows) && !MovableShadowDetails.IsEmpty())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::MovableShadows, EOutlinerAuditSeverity::Warning),
					EOutlinerAuditFixAction::None,
					LOCTEXT("RenderingCategory", "Rendering"),
					LOCTEXT("MovableShadowIssue", "Movable Shadows"),
					FText::Format(LOCTEXT("MovableShadowDetails", "{0} movable primitive components cast shadows."), FText::AsNumber(MovableShadowDetails.Num())),
					MovableShadowDetails);
			}
		}
	};

	class FOutlinerStaticMeshAuditRule final : public IOutlinerAuditRule
	{
	public:
		virtual void Execute(AActor* Actor, FOutlinerAuditContext& Context) override
		{
			if (!IsValid(Actor) || !Context.Settings)
			{
				return;
			}

			int32 MaterialSlotCount = 0;
			TArray<FOutlinerAuditDetailEntry> MissingMeshDetails;
			TArray<FOutlinerAuditDetailEntry> MissingMaterialSlotDetails;
			TArray<FOutlinerAuditDetailEntry> MaterialSlotDetails;

			for (const UStaticMeshComponent* StaticMeshComponent : OutlinerColumnUtils::GetActorComponents<UStaticMeshComponent>(Actor))
			{
				if (!StaticMeshComponent)
				{
					continue;
				}

				if (!StaticMeshComponent->GetStaticMesh())
				{
					MissingMeshDetails.Add(OutlinerAuditUtils::MakeDetailEntry(
						FOutlinerAuditActorHelpers::GetComponentDisplayName(StaticMeshComponent),
						TEXT("StaticMesh: <none>")));
					continue;
				}

				const int32 ComponentMaterialCount = StaticMeshComponent->GetNumMaterials();
				MaterialSlotCount += ComponentMaterialCount;

				for (int32 MaterialIndex = 0; MaterialIndex < ComponentMaterialCount; ++MaterialIndex)
				{
					const UMaterialInterface* Material = StaticMeshComponent->GetMaterial(MaterialIndex);
					const FString SlotSubject = FString::Printf(TEXT("%s - slot %d"), *FOutlinerAuditActorHelpers::GetComponentDisplayName(StaticMeshComponent), MaterialIndex);
					const FString MaterialName = Material ? Material->GetName() : FString(TEXT("<none>"));
					const FOutlinerAuditDetailEntry SlotDetail = OutlinerAuditUtils::MakeDetailEntry(SlotSubject, FString::Printf(TEXT("Material: %s"), *MaterialName));

					MaterialSlotDetails.Add(SlotDetail);
					if (!Material)
					{
						MissingMaterialSlotDetails.Add(SlotDetail);
					}
				}
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::InvalidStaticMesh) && !MissingMeshDetails.IsEmpty())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::InvalidStaticMesh, EOutlinerAuditSeverity::Error),
					EOutlinerAuditFixAction::None,
					LOCTEXT("MeshCategory", "Mesh"),
					LOCTEXT("InvalidStaticMeshIssue", "Invalid Static Mesh"),
					FText::Format(LOCTEXT("InvalidStaticMeshDetails", "{0} StaticMeshComponents have no StaticMesh assigned."), FText::AsNumber(MissingMeshDetails.Num())),
					MissingMeshDetails);
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::InvalidMaterials) && !MissingMaterialSlotDetails.IsEmpty())
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::InvalidMaterials, EOutlinerAuditSeverity::Error),
					EOutlinerAuditFixAction::None,
					LOCTEXT("MaterialsCategory", "Materials"),
					LOCTEXT("InvalidMaterialsIssue", "Invalid Materials"),
					FText::Format(LOCTEXT("InvalidMaterialsDetails", "{0} material slots resolve to no material."), FText::AsNumber(MissingMaterialSlotDetails.Num())),
					MissingMaterialSlotDetails);
			}

			if (Context.IsCriterionEnabled(EOutlinerAuditCriterion::TooManyMaterials) && MaterialSlotCount > Context.Settings->TooManyMaterialSlotsThreshold)
			{
				Context.AddIssue(
					Actor,
					Context.GetIssueSeverity(EOutlinerAuditCriterion::TooManyMaterials, EOutlinerAuditSeverity::Warning),
					EOutlinerAuditFixAction::None,
					LOCTEXT("MaterialComplexityCategory", "Materials"),
					LOCTEXT("TooManyMaterialsIssue", "Too Many Materials"),
					FText::Format(
						LOCTEXT("TooManyMaterialsDetails", "{0} material slots found. Current threshold is {1}."),
						FText::AsNumber(MaterialSlotCount),
						FText::AsNumber(Context.Settings->TooManyMaterialSlotsThreshold)),
					MaterialSlotDetails);
			}
		}
	};
}

void FOutlinerAuditRuleSet::BuildDefaultRules(TArray<TUniquePtr<IOutlinerAuditRule>>& OutRules)
{
	OutRules.Reset();
	OutRules.Add(MakeUnique<FOutlinerActorAuditRule>());
	OutRules.Add(MakeUnique<FOutlinerComponentScaleAuditRule>());
	OutRules.Add(MakeUnique<FOutlinerPrimitiveComponentAuditRule>());
	OutRules.Add(MakeUnique<FOutlinerStaticMeshAuditRule>());
}

#undef LOCTEXT_NAMESPACE
