// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerGenerateOverlapEventsColumn
 * @brief Scene Outliner column for viewing and toggling Generate Overlap Events for actors.
 *
 * Adds a custom column to the Scene Outliner that provides UI controls and status indication
 * for the "Generate Overlap Events" property of actors/components.
 */
class OUTLINERTOOLKIT_API FOutlinerGenerateOverlapEventsColumn : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the column and registers it with the Scene Outliner.
	 *
	 * @param SceneOutliner Reference to the Scene Outliner instance this column is added to.
	 */
	explicit FOutlinerGenerateOverlapEventsColumn(ISceneOutliner& SceneOutliner);

	/**
	 * @brief Returns the unique identifier for this column.
	 *
	 * @return Column name identifier.
	 */
	virtual FName GetColumnID() override;

	/**
	 * @brief Static identifier used for registration and lookup.
	 *
	 * @return Column name identifier.
	 */
	static FName GetID() { return FName("GenerateOverlapEvents"); }

	/**
	 * @brief Indicates whether sorting is supported for this column.
	 *
	 * @return Always false, sorting is not supported.
	 */
	virtual bool SupportsSorting() const override;

	/**
	 * @brief Creates the header row UI definition for this column.
	 *
	 * @return Slate header column arguments.
	 */
	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;

	/**
	 * @brief Creates the widget used to represent this column in a row.
	 *
	 * @param TreeItem Scene Outliner tree item.
	 * @param Row Slate table row containing the item.
	 *
	 * @return Widget displayed in the column cell.
	 */
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

private:
	/**
	 * @brief Handles click interaction for toggling Generate Overlap Events.
	 *
	 * @param WeakActor Actor affected by the button click.
	 *
	 * @return Slate reply indicating UI event handling result.
	 */
	FReply GetGenerateOverlapButtonClicked(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Determines the color used for the button icon based on actor state.
	 *
	 * @param WeakActor Actor used to evaluate visual state.
	 *
	 * @return Slate color for UI representation.
	 */
	FSlateColor GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const;
};
