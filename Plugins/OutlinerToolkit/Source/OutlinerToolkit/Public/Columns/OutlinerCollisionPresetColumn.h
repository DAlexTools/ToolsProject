// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OutlinerToolkitColumnBase.h"
#include "Types/SlateEnums.h"

class AActor;
class UPrimitiveComponent;

/**
 * @class FOutlinerCollisionPresetColumn
 * @brief Scene Outliner column for viewing and editing collision presets of actors.
 *
 * Provides a UI column in the Scene Outliner that displays the current collision preset
 * of an actor (based on its primitive components) and allows changing it via a dropdown.
 */
class OUTLINERTOOLKIT_API FOutlinerCollisionPresetColumn : public FOutlinerToolkitColumnBase
{
public:

	/**
	 * @brief Constructs the Collision Preset column.
	 *
	 * @param SceneOutliner Reference to the Scene Outliner instance this column belongs to.
	 */
	FOutlinerCollisionPresetColumn(ISceneOutliner& SceneOutliner);

	/**
	 * @brief Returns the unique column identifier.
	 *
	 * @return Column ID name used for registration and lookup.
	 */
	virtual FName GetColumnID() override;

	/**
	 * @brief Returns the static column identifier.
	 *
	 * @return Column ID name used for registration.
	 */
	static FName GetID() { return FName("CollisionPreset"); }

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
	 * @brief Initializes available collision profile options if not already created.
	 */
	void EnsureCollisionProfileOptions();

	/**
	 * @brief Finds a collision profile option by name.
	 *
	 * @param ProfileName Name of the collision profile.
	 * @return Shared pointer to option string if found.
	 */
	TSharedPtr<FString> FindProfileOption(FName ProfileName) const;

	/**
	 * @brief Returns a uniform collision profile name if all primitive components share the same value.
	 *
	 * @param PrimitiveComponents Array of primitive components to evaluate.
	 * @return Profile name if uniform, otherwise empty optional.
	 */
	TOptional<FName> GetUniformCollisionProfileName(const TArray<UPrimitiveComponent*>& PrimitiveComponents) const;

	/**
	 * @brief Returns formatted text representation of actor collision preset.
	 *
	 * @param WeakActor Actor to evaluate.
	 * @return Localized text for UI display.
	 */
	FText GetCollisionPresetText(TWeakObjectPtr<AActor> WeakActor) const;

	/**
	 * @brief Creates a widget for a single collision preset option in the combo box.
	 *
	 * @param Option Option data.
	 * @return Slate widget representing the option.
	 */
	TSharedRef<SWidget> MakeCollisionPresetOptionWidget(TSharedPtr<FString> Option) const;

	/**
	 * @brief Callback invoked when a new collision preset is selected.
	 *
	 * @param NewValue Newly selected preset option.
	 * @param SelectInfo Selection type information.
	 * @param WeakActor Actor being modified.
	 */
	void OnCollisionPresetChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo, TWeakObjectPtr<AActor> WeakActor);

private:

	/** Cached list of available collision profile options. */
	TArray<TSharedPtr<FString>> CollisionProfileOptions;
};