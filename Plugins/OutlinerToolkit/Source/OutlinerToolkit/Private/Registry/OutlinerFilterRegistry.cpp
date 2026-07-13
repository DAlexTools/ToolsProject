// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Registry/OutlinerFilterRegistry.h"
#include "Libraries/OutlinerToolkitFilterLibrary.h"
#include "Filters/GenericFilter.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "FOutlinerFilterRegistry"

void FOutlinerFilterRegistry::RegisterOutlinerFilters()
{
	FLevelEditorModule&	LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FFilterCategory> Category = MakeShared<FFilterCategory>(LOCTEXT("OutlinerToolkitFilterCategory", "Outliner Toolkit"), FText::GetEmpty());

	using FOutlinerGenericFilter = FGenericFilter<const ISceneOutlinerTreeItem&>;

	auto CreateFilter = [Category](
							const FString&							Name,
							const FText&							DisplayName,
							const FText&							ToolTipText,
							FOutlinerGenericFilter::FOnItemFiltered Predicate,
							FName									IconName,
							const FLinearColor&						Color) {
		TSharedRef<FOutlinerGenericFilter> Filter = MakeShared<FOutlinerGenericFilter>(
			Category,
			Name,
			DisplayName,
			Predicate);

		Filter->SetToolTipText(ToolTipText);
		Filter->SetIconName(IconName);
		Filter->SetColor(Color);

		return Filter;
	};

	TArray<TSharedRef<FOutlinerGenericFilter>> Filters;
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_Movable"), LOCTEXT("MovableFilterName", "Movable Actors"), LOCTEXT("MovableFilterTooltip", "Only show actors with at least one Movable scene component."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassMovableFilter), TEXT("ClassIcon.Actor"), FLinearColor(0.35f, 0.65f, 1.0f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_HiddenInGame"), LOCTEXT("HiddenInGameFilterName", "Hidden In Game"), LOCTEXT("HiddenInGameFilterTooltip", "Only show actors with at least one scene component hidden in game."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassHiddenInGameFilter), TEXT("Level.VisibleIcon16x"), FLinearColor(0.55f, 0.55f, 0.65f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_CastShadows"), LOCTEXT("CastShadowsFilterName", "Cast Shadows"), LOCTEXT("CastShadowsFilterTooltip", "Only show actors with at least one primitive component casting shadows."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassCastShadowsFilter), TEXT("EditorViewport.LightingOnlyMode"), FLinearColor(0.95f, 0.85f, 0.35f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_OverlapEvents"), LOCTEXT("OverlapEventsFilterName", "Overlap Events"), LOCTEXT("OverlapEventsFilterTooltip", "Only show actors with at least one primitive component generating overlap events."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassGenerateOverlapEventsFilter), TEXT("GraphEditor.Event_16x"), FLinearColor(0.6f, 0.85f, 0.35f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_CustomDepth"), LOCTEXT("CustomDepthFilterName", "Custom Depth"), LOCTEXT("CustomDepthFilterTooltip", "Only show actors with at least one primitive component rendering custom depth."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassCustomDepthFilter), TEXT("EditorViewport.VisualizeBufferMode"), FLinearColor(0.55f, 0.45f, 0.95f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_CustomStencil"), LOCTEXT("CustomStencilFilterName", "Custom Stencil Non-Zero"), LOCTEXT("CustomStencilFilterTooltip", "Only show actors with at least one primitive component whose custom depth stencil value is not 0."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassCustomStencilFilter), TEXT("EditorViewport.VisualizeBufferMode"), FLinearColor(0.7f, 0.45f, 0.95f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_Untagged"), LOCTEXT("UntaggedFilterName", "Untagged Actors"), LOCTEXT("UntaggedFilterTooltip", "Only show actors with no actor tags."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassUntaggedFilter), TEXT("ClassIcon.Actor"), FLinearColor(0.7f, 0.7f, 0.7f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_HasTags"), LOCTEXT("HasTagsFilterName", "Actor Has Tags"), LOCTEXT("HasTagsFilterTooltip", "Only show actors with at least one actor tag."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassHasTagsFilter), TEXT("ClassIcon.Actor"), FLinearColor(0.4f, 0.8f, 0.7f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_MovableCastShadows"), LOCTEXT("MovableCastShadowsFilterName", "Movable + Shadows"), LOCTEXT("MovableCastShadowsFilterTooltip", "Only show actors with a Movable primitive component that casts shadows."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassMovableCastShadowsFilter), TEXT("EditorViewport.LitMode"), FLinearColor(1.0f, 0.58f, 0.25f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_PerformanceRisk"), LOCTEXT("PerformanceRiskFilterName", "Performance Risk"), LOCTEXT("PerformanceRiskFilterTooltip", "Only show actors with tick, physics, overlap events, or Movable shadow-casting components."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassPerformanceRiskFilter), TEXT("Icons.Warning"), FLinearColor(1.0f, 0.35f, 0.22f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_InvalidPhysicsMobility"), LOCTEXT("InvalidPhysicsMobilityFilterName", "Invalid Physics Mobility"), LOCTEXT("InvalidPhysicsMobilityFilterTooltip", "Only show actors with simulating primitive components that are not Movable."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassInvalidPhysicsMobilityFilter), TEXT("PhysicsAssetEditor.Tabs.Tools"), FLinearColor(1.0f, 0.25f, 0.25f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_TooManyMaterials"), LOCTEXT("TooManyMaterialsFilterName", "Too Many Materials "), LOCTEXT("TooManyMaterialsFilterTooltip", "Only show actors whose StaticMeshComponents have more than 5 material slots in total."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassTooManyMaterialsFilter), TEXT("ClassIcon.Material"), FLinearColor(0.9f, 0.45f, 0.7f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_TooManyComponents"), LOCTEXT("TooManyComponentsFilterName", "Too Many Components "), LOCTEXT("TooManyComponentsFilterTooltip", "Only show actors with more than 20 components."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassTooManyComponentsFilter), TEXT("ClassIcon.ActorComponent"), FLinearColor(0.95f, 0.65f, 0.25f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_NonUniformOrNegativeScale"), LOCTEXT("NonUniformOrNegativeScaleFilterName", "Bad Scale"), LOCTEXT("NonUniformOrNegativeScaleFilterTooltip", "Only show actors with non-uniform or negative actor/component scale."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassNonUniformOrNegativeScaleFilter), TEXT("EditorViewport.ScaleMode"), FLinearColor(0.95f, 0.55f, 0.25f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_EditorOnly"), LOCTEXT("EditorOnlyFilterName", "Editor Only"), LOCTEXT("EditorOnlyFilterTooltip", "Only show actors marked as editor-only."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassEditorOnlyFilter), TEXT("LevelEditor.Tabs.Details"), FLinearColor(0.55f, 0.7f, 0.95f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_NoFolder"), LOCTEXT("NoFolderFilterName", "No Folder"), LOCTEXT("NoFolderFilterTooltip", "Only show actors that are not assigned to a folder."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassNoFolderFilter), TEXT("SceneOutliner.FolderClosed"), FLinearColor(0.95f, 0.72f, 0.25f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_InvalidStaticMesh"), LOCTEXT("InvalidStaticMeshFilterName", "Invalid Static Mesh"), LOCTEXT("InvalidStaticMeshFilterTooltip", "Only show actors with at least one StaticMeshComponent that has no StaticMesh assigned."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassInvalidStaticMeshFilter), TEXT("ClassIcon.StaticMeshActor"), FLinearColor(0.95f, 0.32f, 0.24f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_CollisionEnabled"), LOCTEXT("CollisionEnabledFilterName", "Collision Enabled"), LOCTEXT("CollisionEnabledFilterTooltip", "Only show actors with at least one primitive component that has collision enabled."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassCollisionEnabledFilter), TEXT("CollisionAnalyzer.TabIcon"), FLinearColor(0.32f, 0.78f, 0.95f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_NoCollision"), LOCTEXT("NoCollisionFilterName", "No Collision"), LOCTEXT("NoCollisionFilterTooltip", "Only show actors whose primitive components all have collision disabled."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassNoCollisionFilter), TEXT("CollisionAnalyzer.TabIcon"), FLinearColor(0.55f, 0.55f, 0.55f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_InvalidMaterials"), LOCTEXT("InvalidMaterialsFilterName", "Invalid Materials"), LOCTEXT("InvalidMaterialsFilterTooltip", "Only show actors with at least one StaticMeshComponent material slot that resolves to no material."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassInvalidMaterialsFilter), TEXT("ClassIcon.Material"), FLinearColor(0.9f, 0.25f, 0.6f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_TickEnabled"), LOCTEXT("TickEnabledFilterName", "Tick Enabled"), LOCTEXT("TickEnabledFilterTooltip", "Only show actors with Actor Tick enabled."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassTickEnabledFilter), TEXT("GraphEditor.Event_16x"), FLinearColor(0.35f, 0.85f, 0.35f, 1.0f)));
	Filters.Add(CreateFilter(TEXT("OutlinerToolkit_PhysicsEnabled"), LOCTEXT("PhysicsEnabledFilterName", "Physics Enabled"), LOCTEXT("PhysicsEnabledFilterTooltip", "Only show actors with at least one primitive component simulating physics."), FOutlinerGenericFilter::FOnItemFiltered::CreateStatic(&OutlinerToolkit::DoesItemPassSimulatedPhysicsFilter), TEXT("PhysicsAssetEditor.Tabs.Tools"), FLinearColor(0.25f, 0.65f, 1.0f, 1.0f)));

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	for (const TSharedRef<FOutlinerGenericFilter>& Filter : Filters)
	{
		LevelEditorModule.AddCustomFilterToOutliner(Filter);
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

#undef LOCTEXT_NAMESPACE