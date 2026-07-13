// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerHiddenInGameColumn
 * @brief Scene Outliner column for controlling and visualizing "Hidden In Game" state of actors.
 *
 * Provides a UI column in the Scene Outliner that reflects whether an actor is hidden in-game
 * and allows toggling this state directly from the outliner.
 */
class OUTLINERTOOLKIT_API FOutlinerHiddenInGameColumn final : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the Hidden In Game column.
	 *
	 * @param SceneOutliner Reference to the Scene Outliner instance this column belongs to.
	 */
	FOutlinerHiddenInGameColumn(ISceneOutliner& SceneOutliner);
	
	/**
	 * @brief Returns the static column identifier.
	 *
	 * @return Column ID name used for registration and lookup.
	 */
	static FName GetID() { return FName("HiddenInGame"); }

	/**
	 * @brief Returns the runtime column identifier.
	 *
	 * @return Column ID name.
	 */
	virtual FName GetColumnID() override;

	/**
	 * @brief Indicates whether sorting is supported for this column.
	 *
	 * @return True if sorting is supported, otherwise false.
	 */
	virtual bool SupportsSorting() const override;

	/**
	 * @brief Creates the header row UI definition for this column.
	 *
	 * @return Slate header column arguments.
	 */
	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;

	/**
	 * @brief Creates the widget used for each row in this column.
	 *
	 * @param TreeItem Scene Outliner tree item.
	 * @param Row Slate table row containing the item.
	 *
	 * @return Widget displayed in the column cell.
	 */
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

private:
	/**
	 * @brief Returns the icon brush representing Hidden In Game state.
	 *
	 * @param WeakActor Actor to evaluate.
	 *
	 * @return Slate brush for UI icon.
	 */
	const FSlateBrush* GetIconImageBrush(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Returns the color used for the button icon based on actor state.
	 *
	 * @param WeakActor Actor to evaluate.
	 *
	 * @return Slate color for UI representation.
	 */
	FSlateColor GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Handles click action for toggling Hidden In Game state.
	 *
	 * @param WeakActor Actor being modified.
	 *
	 * @return Slate reply indicating event handling result.
	 */
	FReply GetHiddenInGameColumnButtonClicked(TWeakObjectPtr<AActor> WeakActor) const;
};
