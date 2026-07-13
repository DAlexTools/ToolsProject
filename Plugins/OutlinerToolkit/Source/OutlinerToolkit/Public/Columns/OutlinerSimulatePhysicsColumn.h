// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"


/**
 * @class FOutlinerSimulatePhysicsColumn
 * @brief Scene Outliner column for viewing and toggling physics simulation on actors.
 *
 * Provides a UI column in the Scene Outliner that displays the current physics simulation
 * state and allows enabling or disabling simulation directly from the outliner.
 */
class OUTLINERTOOLKIT_API FOutlinerSimulatePhysicsColumn : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the Simulate Physics column.
	 *
	 * @param SceneOutliner Reference to the Scene Outliner instance this column belongs to.
	 */
	explicit FOutlinerSimulatePhysicsColumn(ISceneOutliner& SceneOutliner);

	/**
	 * @brief Returns the static column identifier used for registration.
	 *
	 * @return Column ID name ("SimulatePhysics").
	 */
	static FName GetID() noexcept { return FName("SimulatePhysics"); }

	/**
	 * @brief Returns the runtime column identifier.
	 *
	 * @return Column ID name used by the Scene Outliner.
	 */
	virtual FName GetColumnID() override;

	/**
	 * @brief Creates the header row UI definition for this column.
	 *
	 * @return Slate header column arguments.
	 */
	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;

	/**
	 * @brief Indicates whether sorting is supported for this column.
	 *
	 * @return Always false, sorting is not supported.
	 */
	virtual bool SupportsSorting() const override { return false; }

	/**
	 * @brief Creates the widget used for a row in this column.
	 *
	 * @param TreeItem Scene Outliner tree item.
	 * @param Row Slate table row containing the item.
	 *
	 * @return Widget displayed in the column cell.
	 */
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

private:
	/**
	 * @brief Returns the tooltip text for the simulation toggle button.
	 *
	 * @param bIsStaticMeshActor Whether the actor is a StaticMeshActor.
	 * @param bIsSkySphereActor Whether the actor is a SkySphere actor.
	 *
	 * @return Localized tooltip text.
	 */
	FText GetButtonToolTipText(bool bIsStaticMeshActor, bool bIsSkySphereActor) const;

	/**
	 * @brief Handles click action for toggling physics simulation.
	 *
	 * @param WeakActor Actor being modified.
	 * @param bIsEnabled Desired simulation state.
	 *
	 * @return Slate reply indicating event handling result.
	 */
	FReply GetOnButtonClicked(TWeakObjectPtr<AActor> WeakActor, bool bIsEnabled) const;

	/**
	 * @brief Returns the button color based on the actor's simulation state.
	 *
	 * @param WeakActor Actor to evaluate.
	 * @param bIsEnabled Current simulation state.
	 *
	 * @return Slate color used for UI representation.
	 */
	FSlateColor GetButtonColorAndOpacity(TWeakObjectPtr<AActor> WeakActor, bool bIsEnabled) const;

};