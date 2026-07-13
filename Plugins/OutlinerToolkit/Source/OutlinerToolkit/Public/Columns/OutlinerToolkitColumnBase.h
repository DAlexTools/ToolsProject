// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISceneOutlinerColumn.h"
#include "ISceneOutliner.h"

/**
 * Base class for custom Outliner Toolkit columns.
 *
 * Provides shared functionality and stores a weak reference
 * to the owning Scene Outliner instance.
 */
class OUTLINERTOOLKIT_API FOutlinerToolkitColumnBase : public ISceneOutlinerColumn
{
public:
	/**
	 * Constructs the base outliner column.
	 *
	 * @param InSceneOutliner Reference to the owning Scene Outliner.
	 */
	FOutlinerToolkitColumnBase(ISceneOutliner& InSceneOutliner)
		: WeakSceneOutliner(StaticCastSharedRef<ISceneOutliner>(InSceneOutliner.AsShared()))
	{
	}

protected:
	/** Weak reference to the outliner widget that owns our list */
	TWeakPtr<ISceneOutliner> WeakSceneOutliner;
};
