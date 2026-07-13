// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerMobilityColumn
 * @brief Scene Outliner column for viewing and editing actor mobility settings.
 *
 * Provides a UI column in the Scene Outliner that allows inspection and modification
 * of actor/component mobility (Static, Stationary, Movable) via a dropdown interface.
 */
class OUTLINERTOOLKIT_API FOutlinerMobilityColumn : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the Mobility column.
	 *
	 * @param SceneOutliner Reference to the Scene Outliner instance this column belongs to.
	 */
	explicit FOutlinerMobilityColumn(ISceneOutliner& SceneOutliner);

	/**
	 * @brief Returns the static column identifier used for registration.
	 *
	 * @return Column ID name ("SetMobility").
	 */
	static FName GetID() { return FName("SetMobility"); }

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
	virtual bool SupportsSorting() const override
	{
		return false;
	}

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

	/** Cached list of mobility options (Static / Stationary / Movable). */
	TArray<TSharedPtr<FString>> MobilityOptions;
};