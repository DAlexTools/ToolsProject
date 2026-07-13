// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerCustomDepthColumn
 * @brief Scene Outliner column used to display and control the
 *        Render Custom Depth state for actor primitive components.
 *
 * This column provides an interactive button inside the Scene Outliner
 * allowing users to toggle Custom Depth rendering for actors and visualize
 * the current state through dynamic color feedback.
 */
class OUTLINERTOOLKIT_API FOutlinerCustomDepthColumn : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the custom depth Scene Outliner column.
	 *
	 * @param SceneOutliner Reference to the owning Scene Outliner instance.
	 */
	FOutlinerCustomDepthColumn(ISceneOutliner& SceneOutliner);
		
	/**
	 * @brief Returns the unique identifier of this column.
	 *
	 * @return Column identifier name.
	 */
	virtual FName GetColumnID() override;

	/**
	 * @brief Returns the static identifier for this column type.
	 *
	 * @return Column identifier name.
	 */
	static FName GetID() { return FName("RenderCustomDepth"); }

	/**
	 * @brief Constructs the header row widget for the column.
	 *
	 * @return Slate column arguments used by the Scene Outliner.
	 */
	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;

	/**
	 * @brief Indicates whether this column supports sorting.
	 *
	 * @return Always false since sorting is not supported.
	 */
	virtual bool SupportsSorting() const override;

	/**
	 * @brief Constructs the per-row widget for the specified tree item.
	 *
	 * @param TreeItem Scene Outliner tree item associated with the row.
	 * @param Row Slate table row widget.
	 *
	 * @return Slate widget representing the row content.
	 */
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

private:
	/**
	 * @brief Handles click events for the custom depth toggle button.
	 *
	 * Toggles the Render Custom Depth state for all primitive components
	 * associated with the specified actor.
	 *
	 * @param WeakActor Target actor weak pointer.
	 *
	 * @return Slate reply describing the handled event state.
	 */
	FReply GetOnClicked(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Returns the display color representing the current
	 *        Custom Depth state of the actor.
	 *
	 * Colors are used to visually indicate enabled, disabled,
	 * or mixed component states.
	 *
	 * @param WeakActor Target actor weak pointer.
	 *
	 * @return Slate color used by the UI widget.
	 */
	FSlateColor GetColorAndOpacity(TWeakObjectPtr<AActor> WeakActor) const;
};
