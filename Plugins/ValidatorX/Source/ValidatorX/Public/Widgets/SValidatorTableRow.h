// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "BaseClasses/BlueprintValidatorBase.h"
#include "Widgets/SValidatorWidget.h"
#include "Styling/SlateStyleRegistry.h"

/**
 * @brief Represents a single row in the validator list table.
 *
 * Each row displays a validator's type, name, and an enable/disable button.
 * Inherits from SMultiColumnTableRow to support multiple columns.
 */
class VALIDATORX_API SValidatorTableRow : public SMultiColumnTableRow<TWeakObjectPtr<UBlueprintValidatorBase>>
{
public:
	SLATE_BEGIN_ARGS(SValidatorTableRow) {}
	/** The validator object associated with this row. */
	SLATE_ARGUMENT(TWeakObjectPtr<UBlueprintValidatorBase>, Validator)

	/** The font to use for text in this row. */
	SLATE_ARGUMENT(FSlateFontInfo, Font)
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the row widget.
	 *
	 * Initializes the row with the specified validator and associates it with the owner table.
	 *
	 * @param InArgs The Slate arguments for constructing the row.
	 * @param InOwnerTable The table view that owns this row.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);

	/**
	 * @brief Generates the widget for a given column in this row.
	 *
	 * Called by the multi-column table to populate each column with the appropriate widget.
	 *
	 * @param ColumnId The identifier of the column for which to generate a widget.
	 * @return A shared reference to the widget for the specified column.
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;

	/**
	 * @brief Handles a mouse button press event.
	 *
	 * @param MyGeometry The geometry of this widget.
	 * @param MouseEvent The mouse event.
	 * @return A reply indicating how the event was handled.
	 */
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles a mouse button double-click event.
	 *
	 * @param InMyGeometry The geometry of this widget.
	 * @param InMouseEvent The mouse event.
	 * @return A reply indicating how the event was handled.
	 */
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

	/**
	 * @brief Handles a mouse button release event.
	 *
	 * @param MyGeometry The geometry of this widget.
	 * @param MouseEvent The mouse event.
	 * @return A reply indicating how the event was handled.
	 */
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/**
	 * @brief Handles a key press event while this row has focus.
	 *
	 * @param MyGeometry The geometry of this widget.
	 * @param InKeyEvent The key event.
	 * @return A reply indicating how the event was handled.
	 */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/**
	 * @brief Handles changes to the row's checkbox state.
	 *
	 * Updates the validator's enabled state when the checkbox is toggled.
	 *
	 * @param NewState The new state of the checkbox.
	 */
	void GetButtonCheckBoxStateChange(ECheckBoxState NewState);

	/**
	 * @brief Creates the widget for the "Type" column.
	 *
	 * @return A shared reference to the widget displaying the validator type.
	 */
	[[nodiscard]] TSharedRef<SBox> GetTypeBox();

	/**
	 * @brief Creates the widget for the "Name" column.
	 *
	 * @return A shared reference to the widget displaying the validator name.
	 */
	[[nodiscard]] TSharedRef<SBox> GetNameBox();

	/**
	 * @brief Creates the widget for the "Button" column.
	 *
	 * @return A shared reference to the widget containing the enable/disable checkbox.
	 */
	[[nodiscard]] TSharedRef<SBox> GetButtonBox();

	/**
	 * @brief Retrieves the current state of the row's checkbox.
	 *
	 * @return ECheckBoxState::Checked if the validator is enabled, otherwise ECheckBoxState::Unchecked.
	 */
	ECheckBoxState GetBoxButtonState() const;

private:
	/** The font used for text widgets in this row. */
	FSlateFontInfo LocalFont;

	/** The validator object associated with this row. */
	TWeakObjectPtr<UBlueprintValidatorBase> Validator;

	/**
	 * @brief Helper function to wrap a widget in a centered SBox with padding.
	 *
	 * @tparam WidgetType The type of the widget to wrap.
	 * @param Widget The widget to wrap.
	 * @return The widget wrapped in a padded, centered SBox.
	 */
	template <typename WidgetType>
	TSharedRef<SBox> WrapBox(TSharedRef<WidgetType> Widget)
	{
		return SNew(SBox)
			.Padding(4.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
				[Widget];
	}
};
