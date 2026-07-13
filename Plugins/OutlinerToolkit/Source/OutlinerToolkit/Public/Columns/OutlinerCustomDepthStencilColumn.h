// Copyright (c) 2026 DimAlek. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"

/**
 * @class FOutlinerCustomDepthStencilColumn
 * @brief Scene Outliner column that displays and edits Custom Depth Stencil values for actors.
 *
 * Adds an additional column to the Scene Outliner allowing inspection and modification
 * of Custom Depth Stencil values for actor components that use the Custom Depth rendering pass.
 */
class OUTLINERTOOLKIT_API FOutlinerCustomDepthStencilColumn : public FOutlinerToolkitColumnBase
{
public:
	/**
	 * @brief Constructs the column.
	 *
	 * @param SceneOutliner Reference to the Scene Outliner instance this column is added to.
	 */
	FOutlinerCustomDepthStencilColumn(ISceneOutliner& SceneOutliner);

	/**
	 * @brief Returns the unique column identifier.
	 *
	 * @return Column identifier name.
	 */
	virtual FName GetColumnID() override;
	
	/**
	 * @brief Returns the static column identifier.
	 *
	 * Used for registration and lookup.
	 *
	 * @return Column identifier name.
	 */
	static FName GetID() { return FName("CustomDepthStencil"); }

	/**
	 * @brief Constructs the header row arguments for this column.
	 *
	 * @return Header row column arguments.
	 */
	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;
	
	/**
	 * @brief Indicates whether sorting is supported for this column.
	 *
	 * @return Always false, sorting is not supported.
	 */
	virtual bool SupportsSorting() const override;

	/**
	 * @brief Constructs the widget used for a row in this column.
	 *
	 * @param TreeItem Scene Outliner tree item.
	 * @param Row Table row containing the item.
	 *
	 * @return Widget displayed in the column cell.
	 */
	virtual const TSharedRef<SWidget> ConstructRowWidget(FSceneOutlinerTreeItemRef TreeItem, const STableRow<FSceneOutlinerTreeItemPtr>& Row) override;

private:
	/**
	 * @brief Retrieves the Custom Depth Stencil value for an actor.
	 *
	 * @param WeakActor Weak pointer to the actor.
	 *
	 * @return Stencil value, or empty optional if unavailable.
	 */
	TOptional<int32> GetCustomDepthStencilValue(TWeakObjectPtr<AActor> WeakActor) const;
	
	/**
	 * @brief Checks whether Custom Depth rendering is enabled for the actor.
	 *
	 * @param WeakActor Weak pointer to the actor.
	 *
	 * @return True if any component of the actor uses Custom Depth rendering.
	 */
	bool IsCustomDepthEnable(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Callback invoked when Custom Depth Stencil value is committed in UI.
	 *
	 * @param NewValue New stencil value entered by the user.
	 * @param CommitType Type of text commit action.
	 * @param WeakActor Actor being modified.
	 */
	void OnCustomDepthStencilCommited(int32 NewValue, ETextCommit::Type CommitType, TWeakObjectPtr<AActor> WeakActor);
};
