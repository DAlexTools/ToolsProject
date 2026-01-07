// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SValidatorWidget.h"
#include "BaseClasses/BlueprintValidatorBase.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/SValidatorTableRow.h"
#include "ValidatorXTypes.h"

/* clang-format off */
void SValidatorWidget::Construct(const FArguments& InArgs)
{
	LocalValidators = InArgs._Validators;

	FontInfo = FAppStyle::GetFontStyle("NormalFont");
	FontInfo.Size = 15.0f;

	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		SNew(SSeparator)
		.Thickness(1.0f)
	]

	+ SVerticalBox::Slot()
	.Padding(4)
	[
		SNew(SExpandableArea)
		.InitiallyCollapsed(false)
		.AreaTitle(FText::FromString("Blueprint Validators"))
		.AreaTitleFont(FontInfo)
		.BodyContent()
		[
			SAssignNew(ListViewWidget, SListView<TWeakObjectPtr<UBlueprintValidatorBase>>)
			.ListItemsSource(&LocalValidators)
			.OnGenerateRow(this, &SValidatorWidget::OnGenerateRowForList)
			.SelectionMode(ESelectionMode::None)
			.HeaderRow(GetValidatorHeaderRow())
		]
	];

	ChildSlot
	[
		VerticalBox
	];
}

TSharedRef<ITableRow> SValidatorWidget::OnGenerateRowForList(TWeakObjectPtr<UBlueprintValidatorBase> InItem,const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SValidatorTableRow, OwnerTable)
		.Validator(InItem)
		.Font(FontInfo);
}

void SValidatorWidget::OnCheckValidatorStateChange(ECheckBoxState NewState)
{
	const bool bEnableAll = (NewState == ECheckBoxState::Checked);
	for (const TWeakObjectPtr<UBlueprintValidatorBase>& Validator : LocalValidators)
	{
		if (Validator.IsValid())
		{
			Validator->SetValidationEnabled(bEnableAll);
		}
	}

	if (ListViewWidget.IsValid())
	{
		ListViewWidget->RequestListRefresh();
	}
}

TSharedRef<SHeaderRow> SValidatorWidget::GetValidatorHeaderRow()
{
	TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow)

	+ SHeaderRow::Column(ValidatorListColumns::ColumnID_Type)
	.FillWidth(0.4f)
	.FixedWidth(200.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString("Type"))
		.Justification(ETextJustify::Center).Font(FontInfo)
	]
	
	+ SHeaderRow::Column(ValidatorListColumns::ColumnID_Name)
	.FillWidth(0.4f)
	[
		SNew(STextBlock)
		.Text(FText::FromString("Validator Name"))
		.Justification(ETextJustify::Center).Font(FontInfo)
	]
	
	+ SHeaderRow::Column(ValidatorListColumns::ColumnID_Button)
	.FixedWidth(50.0f)
	[
		SNew(SBox)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SCheckBox)
			.Style(&FSlateStyleRegistry::FindSlateStyle("TakeRecorderStyle")->GetWidgetStyle<FCheckBoxStyle>("TakeRecorder.Source.Switch"))
			.HAlign(HAlign_Center)
			.ToolTipText(FText::FromString("Enable/Disable all validators"))
			.OnCheckStateChanged(SharedThis(this), &SValidatorWidget::OnCheckValidatorStateChange)
		]
	];

	return HeaderRow;
}
