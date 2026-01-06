// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SValidatorTableRow.h"
#include "Library/UtilsFunctionLibrary.h"
#include "ValidatorXTypes.h"

void SValidatorTableRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
{
	Validator = InArgs._Validator;
	LocalFont = InArgs._Font;

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
	else if (ColumnId == ValidatorListColumns::ColumnID_Button)
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

/* clang-format on */
TSharedRef<SBox> SValidatorTableRow::GetTypeBox()
{
	FString Type;
	if (Validator.IsValid())
	{
		Type = Validator->GetTypeValidator();
	}

	TSharedRef<SBox> TypeBox = WrapBox(SNew(STextBlock)
			.Text(FText::FromString(Type))
			.Font(LocalFont)
			.Justification(ETextJustify::Center));

	return TypeBox;
}

TSharedRef<SBox> SValidatorTableRow::GetNameBox()
{
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

	CleanName = FUtilsFunctionLibrary::AddSpacesBeforeUppercase(CleanName);

	TSharedRef<SBox> NameBox = WrapBox(SNew(STextBlock)
			.Text(FText::FromString(CleanName))
			.Font(LocalFont)
			.Justification(ETextJustify::Center));

	return NameBox;
}

TSharedRef<SBox> SValidatorTableRow::GetButtonBox()
{
	TSharedRef<SBox> ButtonBox = WrapBox(SNew(SCheckBox)
			.Style(&FSlateStyleRegistry::FindSlateStyle("TakeRecorderStyle")->GetWidgetStyle<FCheckBoxStyle>("TakeRecorder.Source.Switch"))
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
	}
}
