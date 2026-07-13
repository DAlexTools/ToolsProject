// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerTagsColumn
 * @brief Scene Outliner column used to display, edit, and sort actor tags.
 *
 * This column provides inline tag editing functionality for actors
 * inside the Scene Outliner and supports alphabetical sorting based
 * on actor tag values.
 */
class OUTLINERTOOLKIT_API FOutlinerTagsColumn : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the actor tags Scene Outliner column.
	 *
	 * @param SceneOutliner Reference to the owning Scene Outliner instance.
	 */
	FOutlinerTagsColumn(ISceneOutliner& SceneOutliner)
		: FOutlinerToolkitColumnBase(SceneOutliner)
	{

	}

	/**
	 * @brief Returns the unique identifier of this column.
	 *
	 * @return Column identifier name.
	 */
	virtual FName GetColumnID() override { return FName("ActorTags"); }

	/**
	 * @brief Returns the static identifier for this column type.
	 *
	 * @return Column identifier name.
	 */
	static FName GetID() { return FName("ActorTags"); }

	/**
	 * @brief Constructs the header row widget for the column.
	 *
	 * @return Slate column arguments used by the Scene Outliner.
	 */
	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;

	/**
	 * @brief Indicates whether this column supports sorting.
	 *
	 * @return Always true since tag sorting is supported.
	 */
	virtual bool SupportsSorting() const override { return true; }

	/**
	 * @brief Constructs the per-row widget for the specified tree item.
	 *
	 * @param TreeItem Scene Outliner tree item associated with the row.
	 * @param Row Slate table row widget.
	 *
	 * @return Slate widget representing the row content.
	 */
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

	/**
	 * @brief Sorts Scene Outliner items using actor tag values.
	 *
	 * @param InOutItems Array of Scene Outliner items to sort.
	 * @param InSortMode Requested sort mode.
	 */
	virtual void SortItems(TArray<FSceneOutlinerTreeItemPtr>& InOutItems, const EColumnSortMode::Type InSortMode) const override;

private:
	/**
	 * @brief Builds a string representation of actor tags
	 *        for the specified tree item.
	 *
	 * @param TreeItem Target Scene Outliner item.
	 *
	 * @return Concatenated tag string.
	 */
	FString GetTagsString(FSceneOutlinerTreeItemRef TreeItem) const;

	/**
	 * @brief Returns the current sort mode used by the column.
	 *
	 * @return Active column sort mode.
	 */
	EColumnSortMode::Type GetColumnSortMode() const;
	
	/**
	 * @brief Handles sort mode changes initiated by the Scene Outliner.
	 *
	 * @param PriorityType Sort priority level.
	 * @param Name Column identifier.
	 * @param SortMode Newly selected sort mode.
	 */
	void OnColumnSortModeChanged(EColumnSortPriority::Type PriorityType, const FName& Name, EColumnSortMode::Type SortMode);

	/**
	 * @brief Returns formatted tag text for the specified actor.
	 *
	 * @param WeakActor Target actor weak pointer.
	 *
	 * @return Localized text representation of actor tags.
	 */
	FText GetTagText(TWeakObjectPtr<AActor> WeakActor) const;
	
	/**
	 * @brief Handles tag text commit events from the inline editor.
	 *
	 * Applies edited tags to the target actor.
	 *
	 * @param NewText Newly committed text value.
	 * @param CommitType Slate text commit type.
	 * @param WeakActor Target actor weak pointer.
	 */
	void OnTextCommited(const FText& NewText, ETextCommit::Type CommitType, TWeakObjectPtr<AActor> WeakActor);

	/** Current sorting mode for the column. */
	EColumnSortMode::Type CurrentSortMode = EColumnSortMode::None;
};
