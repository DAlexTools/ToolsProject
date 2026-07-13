// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditTypes.h"


/**
 * Represents a single row widget in the Outliner Audit table.
 *
 * This Slate widget is responsible for displaying audit information
 * for a specific actor and generating widgets for each table column.
 */
class OUTLINERTOOLKIT_API SOutlinerAuditActorRow final : public SMultiColumnTableRow<FOutlinerAuditActorResultPtr>
{
public:
	SLATE_BEGIN_ARGS(SOutlinerAuditActorRow) {}

		/** Audit result associated with the actor displayed in this row. */
		SLATE_ARGUMENT(FOutlinerAuditActorResultPtr, ActorResult)
	
	SLATE_END_ARGS()

	/**
	 * Constructs the row widget.
	 *
	 * @param InArgs Arguments passed during widget construction.
	 * @param InOwnerTable Reference to the parent table view.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);
	
	/**
	 * Generates a widget for the specified column.
	 *
	 * Called automatically by the Slate table system to populate
	 * individual cells of the row.
	 *
	 * @param ColumnName Name of the column being generated.
	 * @return Widget representing the column content.
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	/**
	 * Wraps text inside a styled Slate text widget.
	 *
	 * @param Text Text content to display.
	 * @param Color Text color.
	 * @return Configured Slate widget containing the text.
	 */
	TSharedRef<SWidget> WrapText(const FText& Text, const FSlateColor& Color) const;

	/**
	 * Builds a summary string describing actor categories.
	 *
	 * @return Formatted category summary text.
	 */
	FText GetCategorySummaryText() const;

	/**
	 * Builds a summary string describing detected issues.
	 *
	 * @return Formatted issue summary text.
	 */
	FText GetIssueSummaryText() const;

	/**
	 * Returns the total number of detected issues for the actor.
	 *
	 * @return Text representation of issue count.
	 */
	FText GetIssueCountText() const;

private:
	/** Audit result data associated with this actor row. */
	FOutlinerAuditActorResultPtr ActorResult;
};
