// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ISceneOutlinerColumn.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerTickColumn
 * @brief Custom Scene Outliner column used to display and control Actor Tick state.
 *
 * This column adds a clickable Tick icon to the Unreal Engine Scene Outliner.
 * It allows enabling and disabling Actor ticking directly from the editor UI.
 *
 * Features:
 * - Displays current Actor Tick state.
 * - Supports visual state feedback using icon tinting.
 * - Handles Actors that do not support ticking.
 * - Integrates with Unreal Engine undo/redo transaction system.
 * - Refreshes the Scene Outliner after Tick state changes.
 *
 * The column is intended for editor-only tooling and integrates with
 * the Scene Outliner framework through ISceneOutlinerColumn.
 */
class OUTLINERTOOLKIT_API FOutlinerTickColumn final : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the Tick column instance.
	 *
	 * @param SceneOutliner Reference to the owning Scene Outliner instance.
	 */
	FOutlinerTickColumn(ISceneOutliner& SceneOutliner)
		: FOutlinerToolkitColumnBase(SceneOutliner)
	{
	}

	/**
	 * @brief Returns the unique identifier for this column.
	 *
	 * @return Unique column FName identifier.
	 */
	virtual FName GetColumnID() override 
	{ 
		return FName("ActorTick"); 
	}

	/**
	 * @brief Static accessor for the column identifier.
	 *
	 * Can be used during column registration.
	 *
	 * @return Unique column FName identifier.
	 */
	static FName GetID() 
	{ 
		return FName("ActorTick"); 
	}

	/**
	 * @brief Builds the Scene Outliner header cell widget.
	 *
	 * Creates the header icon displayed above the Tick column.
	 *
	 * @return Slate header row column arguments.
	 */
	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;

	/**
	 * @brief Indicates whether this column supports sorting.
	 *
	 * @return Always false.
	 */
	virtual bool SupportsSorting() const override 
	{ 
		return false; 
	}
	
	/**
	 * @brief Constructs the row widget for a specific Outliner item.
	 *
	 * Creates the Tick toggle button for Actor rows.
	 *
	 * @param TreeItem Target Scene Outliner item.
	 * @param Row Parent table row widget.
	 *
	 * @return Slate widget representing the Tick control.
	 */
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

private:
	/**
	 * @brief Returns tooltip text for the Tick button.
	 *
	 * @param bCanEverTick Whether the Actor supports ticking.
	 *
	 * @return Localized tooltip text.
	 */
	FText GetTickTooltipText(bool bCanEverTick) const;

	/**
	 * @brief Determines icon tint color based on Tick state.
	 *
	 * Provides visual feedback for enabled, disabled,
	 * and unsupported Tick states.
	 *
	 * @param WeakActor Weak pointer to the target Actor.
	 * @param bCanEverTick Whether the Actor supports ticking.
	 *
	 * @return Slate color used for icon rendering.
	 */
	[[nodiscard]] FSlateColor GetColorAndOpacity(TWeakObjectPtr<AActor> WeakActor, bool bCanEverTick) const;
	
	/**
	 * @brief Handles Tick button click events.
	 *
	 * Toggles Actor Tick state and refreshes the Scene Outliner.
	 *
	 * @param WeakActor Weak pointer to the target Actor.
	 * @param bCanEverTick Whether the Actor supports ticking.
	 *
	 * @return Slate reply object.
	 */
	[[nodiscard]] FReply OnTickColumnClicked(TWeakObjectPtr<AActor> WeakActor, bool bCanEverTick) const;
};
