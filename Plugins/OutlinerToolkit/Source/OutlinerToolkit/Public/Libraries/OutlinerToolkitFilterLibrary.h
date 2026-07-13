// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct ISceneOutlinerTreeItem;

namespace OutlinerToolkit
{
	bool DoesItemPassSimulatedPhysicsFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassTickEnabledFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassNoFolderFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassInvalidStaticMeshFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassCollisionEnabledFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassNoCollisionFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassInvalidMaterialsFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassMovableFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassHiddenInGameFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassCastShadowsFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassGenerateOverlapEventsFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassCustomDepthFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassCustomStencilFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassUntaggedFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassHasTagsFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassMovableCastShadowsFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassPerformanceRiskFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassInvalidPhysicsMobilityFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassTooManyMaterialsFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassTooManyComponentsFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassNonUniformOrNegativeScaleFilter(const ISceneOutlinerTreeItem& Item);
	bool DoesItemPassEditorOnlyFilter(const ISceneOutlinerTreeItem& Item);
} // namespace OutlinerToolkit
