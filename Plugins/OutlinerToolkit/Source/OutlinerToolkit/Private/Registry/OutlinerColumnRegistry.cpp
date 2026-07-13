// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Registry/OutlinerColumnRegistry.h"
#include "SceneOutlinerModule.h"
#include "Columns/OutlinerCastShadowsColumn.h"
#include "Columns/OutlinerCollisionPresetColumn.h"
#include "Columns/OutlinerColumnUtils.h"
#include "Columns/OutlinerCustomDepthColumn.h"
#include "Columns/OutlinerCustomDepthStencilColumn.h"
#include "Columns/OutlinerGenerateOverlapEventsColumn.h"
#include "Columns/OutlinerHiddenInGameColumn.h"
#include "Columns/OutlinerMobilityColumn.h"
#include "Columns/OutlinerSimulatePhysicsColumn.h"
#include "Columns/OutlinerTagsColumn.h"
#include "Columns/OutlinerTickColumn.h"
#include "Columns/OutlinerNaniteColumn.h"
#include "Columns/OutlinerNaniteKeepTrianglePercentColumn.h"
#include "Columns/OutlinerActorLockColumn.h"

namespace OutlinerColumnPriority
{
	constexpr int32 HiddenInGame = 2;
	constexpr int32 ActorLock = 3;
	constexpr int32 SimulatePhysics = 4;
	constexpr int32 Mobility = 5;
	constexpr int32 Tick = 6;
	constexpr int32 CastShadow = 7;
	constexpr int32 Nanite = 8;
	constexpr int32 NaniteKeepTrianglePercent = 9;
	constexpr int32 CollisionPreset = 10;
	constexpr int32 GenerateOverlap = 11;
	constexpr int32 CustomDepth = 12;
	constexpr int32 CustomDepthStencil = 13;
	constexpr int32 Tags = 14;
}

void FOutlinerColumnRegistry::RegisterColumns()
{
	FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::LoadModuleChecked<FSceneOutlinerModule>("SceneOutliner");
	{
		RegisterColumn<FOutlinerHiddenInGameColumn>(SceneOutlinerModule, OutlinerColumnPriority::HiddenInGame);
		RegisterColumn<FOutlinerSimulatePhysicsColumn>(SceneOutlinerModule, OutlinerColumnPriority::SimulatePhysics);
		RegisterColumn<FOutlinerMobilityColumn>(SceneOutlinerModule, OutlinerColumnPriority::Mobility);
		RegisterColumn<FOutlinerTickColumn>(SceneOutlinerModule, OutlinerColumnPriority::Tick);
		RegisterColumn<FOutlinerCastShadowsColumn>(SceneOutlinerModule, OutlinerColumnPriority::CastShadow);
		RegisterColumn<FOutlinerCollisionPresetColumn>(SceneOutlinerModule, OutlinerColumnPriority::CollisionPreset);
		RegisterColumn<FOutlinerGenerateOverlapEventsColumn>(SceneOutlinerModule, OutlinerColumnPriority::GenerateOverlap);
		RegisterColumn<FOutlinerCustomDepthColumn>(SceneOutlinerModule, OutlinerColumnPriority::CustomDepth);
		RegisterColumn<FOutlinerCustomDepthStencilColumn>(SceneOutlinerModule, OutlinerColumnPriority::CustomDepthStencil);
		RegisterColumn<FOutlinerTagsColumn>(SceneOutlinerModule, OutlinerColumnPriority::Tags);
		RegisterColumn<FOutlinerNaniteColumn>(SceneOutlinerModule, OutlinerColumnPriority::Nanite);
		RegisterColumn<FOutlinerNaniteKeepTrianglePercentColumn>(SceneOutlinerModule, OutlinerColumnPriority::NaniteKeepTrianglePercent);
		RegisterColumn<FOutlinerActorLockColumn>(SceneOutlinerModule, OutlinerColumnPriority::ActorLock);
	}
}

template<typename TColumn>
void FOutlinerColumnRegistry::RegisterColumn(FSceneOutlinerModule& Module, int32 Priority)
{
	Module.RegisterDefaultColumnType<TColumn>(FSceneOutlinerColumnInfo(
		ESceneOutlinerColumnVisibility::Visible,
		Priority,
		FCreateSceneOutlinerColumn::CreateLambda(
			[](ISceneOutliner& SceneOutliner) ->TSharedRef<ISceneOutlinerColumn>
			{
				return MakeShared<TColumn>(SceneOutliner);
			})));

}
