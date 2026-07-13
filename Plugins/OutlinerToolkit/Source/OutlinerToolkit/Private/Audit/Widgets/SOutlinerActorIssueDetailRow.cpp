// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Widgets/SOutlinerActorIssueDetailRow.h"
#include "Audit/Core/OutlinerAuditUtils.h"
#include "Audit/Core/OutlinerAuditFormatting.h"

#define LOCTEXT_NAMESPACE "SOutlinerAuditActorIssueDetailRow"

void SOutlinerAuditActorIssueDetailRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Issue = InArgs._Issue;

	STableRow<FOutlinerAuditIssuePtr>::Construct(STableRow<FOutlinerAuditIssuePtr>::FArguments().Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow")
		[
			BuildContent()
		],
		InOwnerTable);
}

TSharedRef<SWidget> SOutlinerAuditActorIssueDetailRow::BuildContent() const
{
	TSharedRef<SVerticalBox> RowContent = SNew(SVerticalBox);

	RowContent->AddSlot()
		.AutoHeight()
		.Padding(FMargin(6.0f, 4.0f, 6.0f, 1.0f))
		[
			SNew(STextBlock)
				.Text(this, &SOutlinerAuditActorIssueDetailRow::GetIssueTitleText)
				.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
				.ColorAndOpacity(this, &SOutlinerAuditActorIssueDetailRow::GetIssueColor)
				.AutoWrapText(true)
		];

	RowContent->AddSlot()
		.AutoHeight()
		.Padding(FMargin(6.0f, 1.0f, 6.0f, 4.0f))
		[
			SNew(STextBlock)
				.Text(this, &SOutlinerAuditActorIssueDetailRow::GetIssueDetailsText)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.AutoWrapText(true)
		];

	if (Issue.IsValid())
	{
		for (const FOutlinerAuditDetailEntry& DetailEntry : Issue->DetailEntries)
		{
			RowContent->AddSlot()
				.AutoHeight()
				.Padding(FMargin(12.0f, 1.0f, 6.0f, 1.0f))
				[
					SNew(STextBlock)
						.Text(FText::Format(LOCTEXT("ActorIssueDetailEntryFormat", "{0}: {1}"), DetailEntry.Subject, DetailEntry.Detail))
						.AutoWrapText(true)
				];
		}
	}

	return RowContent;
}

FText SOutlinerAuditActorIssueDetailRow::GetIssueTitleText() const
{
	if (!Issue.IsValid())
	{
		return FText::GetEmpty();
	}

	return Issue->bIgnored ?
		FText::Format(
			LOCTEXT("ActorIgnoredIssueDetailTitle", "{0} - {1} / {2} (ignored)"),
			::FOutlinerAuditReportFormatter::GetSeverityText(Issue->Severity), 
			Issue->Category, Issue->Issue)
		: FText::Format(
			LOCTEXT("ActorIssueDetailTitle", "{0} - {1} / {2}"),
			FOutlinerAuditReportFormatter::GetSeverityText(Issue->Severity),
			Issue->Category, Issue->Issue);
}

FSlateColor SOutlinerAuditActorIssueDetailRow::GetIssueColor() const
{
	return Issue.IsValid()
		? FOutlinerAuditReportFormatter::GetSeverityColor(Issue->Severity)
		: FSlateColor::UseForeground();
}

FText SOutlinerAuditActorIssueDetailRow::GetIssueDetailsText() const
{
	return Issue.IsValid() ? Issue->Details : FText::GetEmpty();
}


#undef LOCTEXT_NAMESPACE 