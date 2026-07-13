// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerActorLockColumn
 * @brief Scene Outliner column for viewing and toggling actor lock state.
 *
 * Provides a UI column in the Scene Outliner that indicates whether an actor
 * is locked for selection or editing and allows toggling this state directly
 * from the outliner.
 */
class OUTLINERTOOLKIT_API FOutlinerActorLockColumn final : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the Actor Lock column.
	 *
	 * @param SceneOutliner Reference to the Scene Outliner instance this column belongs to.
	 */
	FOutlinerActorLockColumn(ISceneOutliner& SceneOutliner)
		: FOutlinerToolkitColumnBase(SceneOutliner)
	{
	}

	/**
	 * @brief Returns the runtime column identifier.
	 *
	 * @return Column ID name used by the Scene Outliner.
	 */
	virtual FName GetColumnID() override;

	/**
	 * @brief Returns the static column identifier used for registration.
	 *
	 * @return Column ID name ("ActorLock").
	 */
	static FName GetID() { return FName("ActorLock"); }

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
	 * @brief Returns the icon brush representing the actor lock state.
	 *
	 * @param WeakActor Actor to evaluate.
	 *
	 * @return Slate brush used for UI icon representation.
	 */
	const FSlateBrush* GetIconImageBrush(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Handles click action for toggling actor lock state.
	 *
	 * @param WeakActor Actor being modified.
	 *
	 * @return Slate reply indicating event handling result.
	 */
	FReply GetLockColumnButtonClicked(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Returns the button color based on the actor lock state.
	 *
	 * @param WeakActor Actor to evaluate.
	 *
	 * @return Slate color used for UI representation.
	 */
	FSlateColor GetColorAndOpacityButtonImage(TWeakObjectPtr<AActor> WeakActor) const;
};
