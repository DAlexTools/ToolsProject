// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace
{
	const FName OutlinerToolkitAuditTabId(TEXT("OutlinerToolkitAudit"));

	/**
	 * @brief Clipboard used to store actor settings copied from the Scene Outliner.
	 *
	 * Stores copied actor and component properties that can later be pasted
	 * onto one or more selected actors.
	 *
	 * Properties are copied selectively using the corresponding copy flags.
	 * Optional values are used to represent mixed component states or
	 * unavailable settings.
	 */
	struct FOutlinerActorSettingsClipboard
	{
		/** Indicates whether the clipboard currently contains any copied data. */
		bool bHasData = false;

		/** Whether actor tick settings should be pasted. */
		bool bCopyTick = false;

		/** Whether Hidden In Game settings should be pasted. */
		bool bCopyHiddenInGame = false;

		/** Whether component mobility settings should be pasted. */
		bool bCopyMobility = false;

		/** Whether shadow casting settings should be pasted. */
		bool bCopyCastShadows = false;

		/** Whether overlap event settings should be pasted. */
		bool bCopyGenerateOverlapEvents = false;

		/** Whether Custom Depth rendering settings should be pasted. */
		bool bCopyRenderCustomDepth = false;

		/** Whether Custom Depth stencil values should be pasted. */
		bool bCopyCustomDepthStencilValue = false;

		/** Whether physics simulation settings should be pasted. */
		bool bCopySimulatePhysics = false;

		/** Whether actor tags should be pasted. */
		bool bCopyTags = false;

		/** Whether actor folder assignment should be pasted. */
		bool bCopyFolder = false;

		/** Copied actor tick enabled state. */
		TOptional<bool> bTickEnabled;

		/** Copied Hidden In Game state shared by all scene components. */
		TOptional<bool> bHiddenInGame;

		/** Copied component mobility value shared by all scene components. */
		TOptional<EComponentMobility::Type> Mobility;

		/** Copied Cast Shadow state shared by all primitive components. */
		TOptional<bool> bCastShadows;

		/** Copied Generate Overlap Events state shared by all primitive components. */
		TOptional<bool> bGenerateOverlapEvents;

		/** Copied Render Custom Depth state shared by all primitive components. */
		TOptional<bool> bRenderCustomDepth;

		/** Copied Custom Depth stencil value shared by all primitive components. */
		TOptional<int32> CustomDepthStencilValue;

		/** Copied Simulate Physics state shared by all primitive components. */
		TOptional<bool> bSimulatePhysics;

		/** Copied actor tags. */
		TArray<FName> Tags;

		/** Copied actor folder path. */
		FName FolderPath = NAME_None;
	};
}


