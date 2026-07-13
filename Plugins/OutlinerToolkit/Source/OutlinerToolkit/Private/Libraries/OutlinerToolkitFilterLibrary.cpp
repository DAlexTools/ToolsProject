// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Libraries/OutlinerToolkitFilterLibrary.h"
#include "Columns/OutlinerColumnUtils.h"
#include "ActorTreeItem.h"
#include "Columns/OutlinerColumnUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "ISceneOutlinerTreeItem.h"
#include "Settings/OutlinerToolkitSettings.h"
#include "Audit/Core/OutlinerAuditUtils.h"

namespace OutlinerToolkit
{
	/**
	 * @brief Checks whether an item contains at least one component simulating physics.
	 *
	 * Resolves the actor associated with the outliner item and scans all primitive
	 * components for physics simulation.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if any primitive component has physics simulation enabled.
	 * @return false if no simulated physics components are found or the actor is invalid.
	 */
	[[nodiscard]] bool DoesItemPassSimulatedPhysicsFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (Component && Component->IsSimulatingPhysics())
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether actor ticking is enabled.
	 *
	 * The actor must support ticking and have ticking currently enabled.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if actor ticking is enabled.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassTickEnabledFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		return Actor->PrimaryActorTick.bCanEverTick && Actor->IsActorTickEnabled();
	}

	/**
	 * @brief Checks whether an actor is not assigned to any folder.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if the actor has no folder assigned.
	 * @return false if the actor belongs to a folder or is invalid.
	 */
	[[nodiscard]] bool DoesItemPassNoFolderFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}
		return Actor && Actor->GetFolderPath().IsNone();
	}

	/**
	 * @brief Detects static mesh components with missing mesh assets.
	 *
	 * Useful for identifying broken actor setups where a static mesh component
	 * exists but no mesh asset is assigned.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if at least one static mesh component has no assigned mesh.
	 * @return false otherwise.
	 *
	 * @warning Actors with missing mesh references may indicate corrupted or incomplete content.
	 */
	[[nodiscard]] bool DoesItemPassInvalidStaticMeshFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UStaticMeshComponent* Component : OutlinerColumnUtils::GetActorComponents<UStaticMeshComponent>(Actor))
		{
			if (Component && !Component->GetStaticMesh())
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether any component has collision enabled.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if at least one primitive component uses collision.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassCollisionEnabledFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (Component && Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether all primitive components have collision disabled.
	 *
	 * Actors without primitive components are excluded from this filter.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if every primitive component uses NoCollision.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassNoCollisionFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		const TArray<UPrimitiveComponent*> PrimitiveComponents = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor);
		if (PrimitiveComponents.IsEmpty())
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : PrimitiveComponents)
		{
			if (Component && Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				return false;
			}
		}

		return true;
	}

	/**
	 * @brief Detects missing material assignments on static mesh components.
	 *
	 * Iterates through all material slots and verifies that each slot contains
	 * a valid material interface.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if any material slot is empty.
	 * @return false otherwise.
	 *
	 * @warning Empty material slots may result in rendering issues.
	 */
	[[nodiscard]] bool DoesItemPassInvalidMaterialsFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UStaticMeshComponent* Component : OutlinerColumnUtils::GetActorComponents<UStaticMeshComponent>(Actor))
		{
			if (!Component || !Component->GetStaticMesh())
			{
				continue;
			}

			const int32 MaterialCount = Component->GetNumMaterials();
			for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
			{
				if (!Component->GetMaterial(MaterialIndex))
				{
					return true;
				}
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether any scene component is movable.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if at least one component has Movable mobility.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassMovableFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const USceneComponent* Component : OutlinerColumnUtils::GetActorComponents<USceneComponent>(Actor))
		{
			if (Component && Component->Mobility == EComponentMobility::Movable)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether any component is hidden during gameplay.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if at least one component has Hidden In Game enabled.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassHiddenInGameFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const USceneComponent* Component : OutlinerColumnUtils::GetActorComponents<USceneComponent>(Actor))
		{
			if (Component && Component->bHiddenInGame)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether any primitive component casts shadows.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if shadow casting is enabled on at least one component.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassCastShadowsFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (Component && Component->CastShadow)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether overlap event generation is enabled.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if any primitive component generates overlap events.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassGenerateOverlapEventsFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (Component && Component->GetGenerateOverlapEvents())
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether custom depth rendering is enabled.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if any primitive component renders into the custom depth buffer.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassCustomDepthFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (Component && Component->bRenderCustomDepth)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether a custom stencil value is assigned.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if any primitive component uses a non-zero stencil value.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassCustomStencilFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (Component && Component->CustomDepthStencilValue != 0)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether an actor has no tags assigned.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if the actor tag array is empty.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassUntaggedFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		return Actor && Actor->Tags.IsEmpty();
	}

	/**
	 * @brief Checks whether an actor contains at least one tag.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if the actor has one or more tags.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassHasTagsFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}
		return Actor && !Actor->Tags.IsEmpty();
	}

	/**
	 * @brief Detects movable components that also cast shadows.
	 *
	 * Such components are generally more expensive to render and may affect
	 * runtime performance.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if a movable shadow-casting component exists.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassMovableCastShadowsFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (Component && Component->Mobility == EComponentMobility::Movable && Component->CastShadow)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Identifies actors that may negatively impact performance.
	 *
	 * Checks for ticking actors, physics simulation, overlap events,
	 * and movable shadow-casting components.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if any performance risk condition is detected.
	 * @return false otherwise.
	 *
	 * @note This filter provides heuristic detection and does not represent an actual profiler result.
	 */
	[[nodiscard]] bool DoesItemPassPerformanceRiskFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		if (Actor->PrimaryActorTick.bCanEverTick && Actor->IsActorTickEnabled())
		{
			return true;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (!Component)
			{
				continue;
			}

			if (Component->IsSimulatingPhysics()
				|| Component->GetGenerateOverlapEvents()
				|| (Component->Mobility == EComponentMobility::Movable && Component->CastShadow))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Detects invalid physics mobility configurations.
	 *
	 * Physics simulation typically requires Movable mobility.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if a simulated physics component is not movable.
	 * @return false otherwise.
	 *
	 * @warning Non-movable physics components may produce unexpected behavior.
	 */
	[[nodiscard]] bool DoesItemPassInvalidPhysicsMobilityFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		for (const UPrimitiveComponent* Component : OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor))
		{
			if (Component && Component->IsSimulatingPhysics() && Component->Mobility != EComponentMobility::Movable)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether an actor exceeds the configured material slot threshold.
	 *
	 * The threshold value is defined in plugin developer settings.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if the total material slot count exceeds the configured limit.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassTooManyMaterialsFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}
		const UOutlinerToolkitSettings* DeveloperSettings = GetDefault<UOutlinerToolkitSettings>();
		check(DeveloperSettings);

		int32 MaterialSlotCount = 0;
		for (const UStaticMeshComponent* Component : OutlinerColumnUtils::GetActorComponents<UStaticMeshComponent>(Actor))
		{
			if (Component)
			{
				MaterialSlotCount += Component->GetNumMaterials();
			}
		}

		return MaterialSlotCount > DeveloperSettings->TooManyMaterialSlotsThreshold;
	}

	/**
	 * @brief Checks whether an actor contains too many components.
	 *
	 * The maximum allowed component count is defined in developer settings.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if the component count exceeds the configured threshold.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassTooManyComponentsFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		const UOutlinerToolkitSettings* DeveloperSettings = GetDefault<UOutlinerToolkitSettings>();
		check(DeveloperSettings);

		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		return Components.Num() > DeveloperSettings->TooManyComponentsThreshold;
	}

	/**
	 * @brief Detects non-uniform or negative scale values.
	 *
	 * Evaluates both actor scale and all scene component relative scales.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if non-uniform or negative scaling is detected.
	 * @return false otherwise.
	 *
	 * @note Scale comparison uses the tolerance value configured in developer settings.
	 */
	[[nodiscard]] bool DoesItemPassNonUniformOrNegativeScaleFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		const UOutlinerToolkitSettings* DeveloperSettings = GetDefault< UOutlinerToolkitSettings>();
		if (OutlinerAuditUtils::IsNonUniformOrNegativeScale(Actor->GetActorScale3D(), DeveloperSettings->ScaleUniformTolerance))
		{
			return true;
		}

		for (const USceneComponent* Component : OutlinerColumnUtils::GetActorComponents<USceneComponent>(Actor))
		{
			if (Component && OutlinerAuditUtils::IsNonUniformOrNegativeScale(Component->GetRelativeScale3D(), DeveloperSettings->ScaleUniformTolerance))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Checks whether an actor is marked as editor-only.
	 *
	 * Editor-only actors are excluded from packaged builds.
	 *
	 * @param Item Scene outliner item to evaluate.
	 * @return true if the actor is editor-only.
	 * @return false otherwise.
	 */
	[[nodiscard]] bool DoesItemPassEditorOnlyFilter(const ISceneOutlinerTreeItem& Item)
	{
		AActor* Actor = OutlinerColumnUtils::ResolveActor(Item);
		if (!Actor)
		{
			return false;
		}

		return Actor && Actor->IsEditorOnly();
	}
} // namespace OutlinerToolkit
