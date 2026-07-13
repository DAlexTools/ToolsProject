// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Audit/Core/OutlinerAuditTypes.h"

/**
 * Slate table row widget used to display a single actor audit issue in the Outliner Audit toolkit.
 * Inherits from STableRow and binds to FOutlinerAuditIssuePtr as the row data source.
 */
class OUTLINERTOOLKIT_API SOutlinerAuditActorIssueDetailRow final : public STableRow<FOutlinerAuditIssuePtr>
{
public:
	SLATE_BEGIN_ARGS(SOutlinerAuditActorIssueDetailRow) {}
		/** The audit issue instance associated with this row */
		SLATE_ARGUMENT(FOutlinerAuditIssuePtr, Issue)
	SLATE_END_ARGS()
	
	/**
	 * Constructs the widget.
	 *
	 * @param InArgs Slate construction arguments, including the Issue to display.
	 * @param InOwnerTable The owning table view that contains this row.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);

private:
	/**
	 * Builds the visual content of the row widget.
	 *
	 * @return A Slate widget representing the row content.
	 */
	TSharedRef<SWidget> BuildContent() const;

	/**
	 * Retrieves the display title text for the audit issue.
	 *
	 * @return Localized text representing the issue title.
	 */
	FText GetIssueTitleText() const;

	/**
	 * Determines the visual color associated with the issue severity/state.
	 *
	 * @return Slate color used to visually represent the issue.
	 */
	FSlateColor GetIssueColor() const;

	/**
	 * Retrieves detailed description text for the audit issue.
	 *
	 * @return Localized text containing issue details.
	 */
	FText GetIssueDetailsText() const;

private:
	/** Pointer to the audit issue data represented by this row */
	FOutlinerAuditIssuePtr Issue;
};

