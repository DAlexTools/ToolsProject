// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Widgets/SOutlinerAuditActorRow.h"
#include "Columns/OutlinerColumnUtils.h"
#include "Audit/Core/OutlinerAuditUtils.h"
#include "Audit/Core/OutlinerAuditConstants.h"
#include "Audit/Core/OutlinerAuditFormatting.h"

#define LOCTEXT_NAMESPACE "SOutlinerAuditActorRow"

void SOutlinerAuditActorRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	ActorResult = InArgs._ActorResult;

	SMultiColumnTableRow<FOutlinerAuditActorResultPtr>::Construct(
		FSuperRowType::FArguments().Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow"),
		InOwnerTable);
}

TSharedRef<SWidget> SOutlinerAuditActorRow::GenerateWidgetForColumn(const FName& ColumnName) 
{
	if (!ActorResult.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	if (ColumnName == OutlinerAuditColumns::Severity)
	{
		return WrapText(FOutlinerAuditReportFormatter::GetSeverityText(
			ActorResult->WorstSeverity),
			FOutlinerAuditReportFormatter::GetSeverityColor(ActorResult->WorstSeverity));
	}

	if (ColumnName == OutlinerAuditColumns::Actor)
	{
		return WrapText(FOutlinerAuditReportFormatter::GetActorLabelText(ActorResult->Actor), FSlateColor::UseForeground());
	}

	if (ColumnName == OutlinerAuditColumns::Category)
	{
		return WrapText(GetCategorySummaryText(), FSlateColor::UseForeground());
	}

	if (ColumnName == OutlinerAuditColumns::Issue)
	{
		return WrapText(GetIssueCountText(), FSlateColor::UseForeground());
	}

	if (ColumnName == OutlinerAuditColumns::Details)
	{
		return WrapText(GetIssueSummaryText(), FSlateColor::UseSubduedForeground());
	}

	if (ColumnName == OutlinerAuditColumns::Fix)
	{
		return WrapText(
			ActorResult->FixableIssueCount > 0
			? FText::Format(LOCTEXT("ActorFixableCountText", "{0} fixable"), FText::AsNumber(ActorResult->FixableIssueCount))
			: LOCTEXT("NoFixText", "-"),
			ActorResult->FixableIssueCount > 0
			? FSlateColor(FLinearColor(0.35f, 0.75f, 0.45f))
			: FSlateColor::UseSubduedForeground());
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SOutlinerAuditActorRow::WrapText(const FText& Text, const FSlateColor& Color) const
{
	return SNew(SBox)
		.Padding(FMargin(6.0f, 2.0f))
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(Text)
				.ColorAndOpacity(Color)
				.AutoWrapText(true)
		];
}

FText SOutlinerAuditActorRow::GetCategorySummaryText() const
{
	TArray<FString> Categories;
	for (const FOutlinerAuditIssuePtr& Issue : ActorResult->Issues)
	{
		if (Issue.IsValid())
		{
			Categories.AddUnique(Issue->Category.ToString());
		}
	}

	return Categories.IsEmpty()
		? LOCTEXT("NoCategoriesText", "-")
		: FText::FromString(FString::Join(Categories, TEXT(", ")));
}

FText SOutlinerAuditActorRow::GetIssueSummaryText() const
{
	TArray<FString> IssueNames;
	for (const FOutlinerAuditIssuePtr& Issue : ActorResult->Issues)
	{
		if (Issue.IsValid())
		{
			IssueNames.AddUnique(Issue->Issue.ToString());
		}
	}

	if (IssueNames.IsEmpty())
	{
		return FText::GetEmpty();
	}

	constexpr int32 MaxInlineIssues = 3;
	TArray<FString> VisibleIssueNames;
	for (int32 Index = 0; Index < IssueNames.Num() && Index < MaxInlineIssues; ++Index)
	{
		VisibleIssueNames.Add(IssueNames[Index]);
	}

	FString Summary = FString::Join(VisibleIssueNames, TEXT(", "));
	if (IssueNames.Num() > MaxInlineIssues)
	{
		Summary += FString::Printf(TEXT(" (+%d)"), IssueNames.Num() - MaxInlineIssues);
	}

	return FText::FromString(Summary);
}

FText SOutlinerAuditActorRow::GetIssueCountText() const
{
	if (ActorResult->IgnoredIssueCount > 0)
	{
		return FText::Format(LOCTEXT("ActorIssueCountWithIgnoredText",
			"{0} issues ({1} ignored)"),
			FText::AsNumber(ActorResult->Issues.Num()),
			FText::AsNumber(ActorResult->IgnoredIssueCount));
	}

	return FText::Format(LOCTEXT("ActorIssueCountText", "{0} issues"), FText::AsNumber(ActorResult->Issues.Num()));
}

#undef LOCTEXT_NAMESPACE
