// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UBlueprintValidatorBase;

/**
 * @brief Main widget for displaying and managing Blueprint validators.
 *
 * SValidatorWidget provides a UI for listing validators and interacting with them
 * via Slate widgets, including custom styles for checkboxes.
 *
 * @ingroup ValidatorX
 */
class VALIDATORX_API SValidatorWidget : public SCompoundWidget
{
public:
	/**
	 * @brief Slate constructor arguments for SValidatorWidget.
	 *
	 * @param Validators Array of validators to display in the list.
	 */
	SLATE_BEGIN_ARGS(SValidatorWidget) {}
	SLATE_ARGUMENT(TArray<TWeakObjectPtr<UBlueprintValidatorBase>>, Validators)
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the Slate widget.
	 *
	 * Initializes local validator data, builds the widget layout, and sets up the list view.
	 *
	 * @param InArgs Constructor arguments passed via SLATE_BEGIN_ARGS.
	 */
	void Construct(const FArguments& InArgs);

private:
	/**
	 * @brief Generates a row for each validator in the list.
	 *
	 * Creates a table row for the given item and associates it with the OwnerTable.
	 *
	 * @param InItem The validator item for which to generate the row.
	 * @param OwnerTable The table view in which the row will be displayed.
	 * @return The generated table row.
	 * @see SListView
	 */
	TSharedRef<ITableRow> OnGenerateRowForList(TWeakObjectPtr<UBlueprintValidatorBase> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	 * @brief Handles changes to the "Enable/Disable All Validators" checkbox in the header.
	 *
	 * Updates the enabled state of all validators in the list based on the new checkbox state
	 * and refreshes the list view to reflect changes.
	 *
	 * @param NewState The new state of the checkbox (Checked or Unchecked).
	 */
	void OnCheckValidatorStateChange(ECheckBoxState NewState);

	/**
	 * @brief Creates and returns the header row for the validator list.
	 *
	 * Defines the columns for Type, Validator Name, and the Enable/Disable button.
	 * Each column contains a widget for its header, including a checkbox in the button column
	 * to toggle all validators.
	 *
	 * @return A shared reference to the constructed SHeaderRow.
	 */
	TSharedRef<SHeaderRow> GetValidatorHeaderRow();

	/** @brief Local copy of validators for internal widget use. */
	TArray<TWeakObjectPtr<UBlueprintValidatorBase>> LocalValidators;

	/** @brief List widget for displaying validators. */
	TSharedPtr<SListView<TWeakObjectPtr<UBlueprintValidatorBase>>> ListViewWidget;

	/** @brief Custom style set used for checkboxes in the widget. */
	TSharedPtr<FSlateStyleSet> CheckBoxStyleSet;

	/** @brief Font info for displaying text in the widget. */
	FSlateFontInfo FontInfo;
};
