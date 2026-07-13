// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SValidatorTableRow.h"
#include "Library/UtilsFunctionLibrary.h"
#include "ValidatorXTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

void SValidatorTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Validator = InArgs._Validator;
	LocalFont = InArgs._Font;
	OnValidatorChanged = InArgs._OnValidatorChanged;

	SMultiColumnTableRow::Construct(
		FSuperRowType::FArguments()														 //
			.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow"), //
		InOwnerTable);																	 //
}

TSharedRef<SWidget> SValidatorTableRow::GenerateWidgetForColumn(const FName& ColumnId)
{
	if (ColumnId == ValidatorListColumns::ColumnID_Type)
	{
		return GetTypeBox();
	}
	else if (ColumnId == ValidatorListColumns::ColumnID_Name)
	{
		return GetNameBox();
	}
	else if (ColumnId == ValidatorListColumns::ColumnID_State)
	{
		return GetStateBox();
	}
	else if (ColumnId == ValidatorListColumns::ColumnID_Enabled)
	{
		return GetButtonBox();
	}

	return SNullWidget::NullWidget;
}

FReply SValidatorTableRow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Unhandled();
}

FReply SValidatorTableRow::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Unhandled();
}

FReply SValidatorTableRow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Unhandled();
}

FReply SValidatorTableRow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	return FReply::Unhandled();
}

TSharedRef<SBox> SValidatorTableRow::GetTypeBox()
{
	return MakeTextCell(Validator.IsValid() ? Validator->GetTypeValidator() : FString(), ETextJustify::Center);
}

TSharedRef<SBox> SValidatorTableRow::GetNameBox()
{
	if (!Validator.IsValid())
	{
		return MakeTextCell(FString(), ETextJustify::Left);
	}

	FString CleanName = Validator->GetName();
	int32	UnderscoreIndex;
	if (CleanName.FindLastChar('_', UnderscoreIndex))
	{
		const FString Suffix = CleanName.Mid(UnderscoreIndex + 1);
		if (Suffix.IsNumeric())
		{
			CleanName = CleanName.Left(UnderscoreIndex);
		}
	}

	if (CleanName.StartsWith(TEXT("Default__")))
	{
		CleanName.RightChopInline(9);
	}

	CleanName = FUtilsFunctionLibrary::AddSpacesBeforeUppercase(CleanName);

	return MakeTextCell(CleanName, ETextJustify::Left);
}

TSharedRef<SBox> SValidatorTableRow::GetStateBox()
{
	return SNew(SBox)
		.Padding(6.0f, 4.0f)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
				.Text(this, &SValidatorTableRow::GetStateText)
				.Font(LocalFont)
				.Justification(ETextJustify::Center)
		];
}

FText SValidatorTableRow::GetStateText() const
{
	const bool bEnabled = Validator.IsValid() && Validator->IsEnabled();
	return bEnabled ? INVTEXT("Enabled") : INVTEXT("Disabled");
}

TSharedRef<SBox> SValidatorTableRow::GetButtonBox()
{
	TSharedRef<SBox> ButtonBox = WrapBox(SNew(SCheckBox)
			.IsChecked(SharedThis(this), &SValidatorTableRow::GetBoxButtonState)
			.OnCheckStateChanged(SharedThis(this), &SValidatorTableRow::GetButtonCheckBoxStateChange));

	return ButtonBox;
}
ECheckBoxState SValidatorTableRow::GetBoxButtonState() const
{
	if (Validator.IsValid())
	{
		return Validator->IsEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}
	return ECheckBoxState::Unchecked;
}

void SValidatorTableRow::GetButtonCheckBoxStateChange(ECheckBoxState NewState)
{
	if (Validator.IsValid())
	{
		switch (NewState)
		{
			case ECheckBoxState::Unchecked:
				Validator->SetValidationEnabled(false);
				break;

			case ECheckBoxState::Checked:
				Validator->SetValidationEnabled(true);
				break;

			default:
				break;
		}
		Invalidate(EInvalidateWidgetReason::Layout);
		OnValidatorChanged.ExecuteIfBound();
	}
}

TSharedRef<SBox> SValidatorTableRow::MakeTextCell(const FString& Text, ETextJustify::Type Justification) const
{
	return SNew(SBox)
		.Padding(6.0f, 4.0f)
		.VAlign(VAlign_Center)
		.HAlign(Justification == ETextJustify::Left ? HAlign_Left : HAlign_Center)
		[
			SNew(STextBlock)
				.Text(FText::FromString(Text))
				.Font(LocalFont)
				.Justification(Justification)
		];
}
