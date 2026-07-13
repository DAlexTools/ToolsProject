// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerCastShadowsColumn
 * @brief Scene Outliner column for viewing and toggling Cast Shadows property of actors.
 *
 * Provides a UI column in the Scene Outliner that indicates whether an actor casts shadows
 * and allows toggling this property directly from the outliner.
 */
class OUTLINERTOOLKIT_API FOutlinerCastShadowsColumn : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the Cast Shadows column.
	 *
	 * @param SceneOutliner Reference to the Scene Outliner instance this column is added to.
	 */
	FOutlinerCastShadowsColumn(ISceneOutliner& SceneOutliner);

	/**
	 * @brief Returns the unique column identifier.
	 *
	 * @return Column ID name.
	 */
	virtual FName GetColumnID() override;

	/**
	 * @brief Returns the static column identifier.
	 *
	 * @return Column ID name used for registration and lookup.
	 */
	static FName GetID() { return FName("CastShadows"); }

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
	virtual bool SupportsSorting() const override;
	

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
	 * @brief Handles button click for toggling Cast Shadows property.
	 *
	 * @param WeakActor Actor being modified.
	 *
	 * @return Slate reply indicating UI event result.
	 */
	FReply GetCastShadowColumnButtonClicked(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Returns the icon color based on Cast Shadows state.
	 *
	 * @param WeakActor Actor to evaluate.
	 *
	 * @return Slate color for UI representation.
	 */
	FSlateColor GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const;
};
