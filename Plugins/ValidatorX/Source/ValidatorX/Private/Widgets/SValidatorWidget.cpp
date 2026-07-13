// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/SValidatorWidget.h"

#include "BaseClasses/ValidatorXBase.h"
#include "ValidatorXManager.h"
#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"

namespace ValidatorListColumns
{
	static const FName ColumnID_Type("Type");
	static const FName ColumnID_Name("Name");
	static const FName ColumnID_State("State");
	static const FName ColumnID_Enabled("Enabled");
}

namespace ValidatorWidgetUtils
{
	FString AddSpacesBeforeUppercase(const FString& Input)
	{
		FString Result;
		Result.Reserve(Input.Len() * 2);

		for(int32 i = 0; i < Input.Len(); ++i)
		{
			const TCHAR Char = Input[i];
			if(i > 0 && FChar::IsUpper(Char) && !FChar::IsWhitespace(Input[i - 1]))
			{
				Result += TEXT(" ");
			}
			Result += Char;
		}

		return Result;
	}

	FString GetValidatorDisplayName(const UValidatorXBase* Validator)
	{
		if(!Validator)
		{
			return FString();
		}

		FString CleanName = Validator->GetName();
		int32 UnderscoreIndex;
		if(CleanName.FindLastChar('_', UnderscoreIndex))
		{
			const FString Suffix = CleanName.Mid(UnderscoreIndex + 1);
			if(Suffix.IsNumeric())
			{
				CleanName = CleanName.Left(UnderscoreIndex);
			}
		}

		if(CleanName.StartsWith(TEXT("Default__")))
		{
			CleanName.RightChopInline(9);
		}

		return AddSpacesBeforeUppercase(CleanName);
	}

	bool MatchesSearch(const UValidatorXBase* Validator, const FString& Search)
	{
		if(Search.IsEmpty())
		{
			return true;
		}

		if(!Validator)
		{
			return false;
		}

		return GetValidatorDisplayName(Validator).Contains(Search, ESearchCase::IgnoreCase)
			|| Validator->GetTypeValidator().Contains(Search, ESearchCase::IgnoreCase);
	}
}

class SValidatorTableRow : public SMultiColumnTableRow<TWeakObjectPtr<UValidatorXBase>>
{
public:
	SLATE_BEGIN_ARGS(SValidatorTableRow) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UValidatorXBase>, Validator)
		SLATE_ARGUMENT(FSlateFontInfo, Font)
		SLATE_EVENT(FSimpleDelegate, OnValidatorChanged)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable)
	{
		Validator = InArgs._Validator;
		LocalFont = InArgs._Font;
		OnValidatorChanged = InArgs._OnValidatorChanged;

		SMultiColumnTableRow::Construct(
			FSuperRowType::FArguments()
			.Style(FAppStyle::Get(), "ContentBrowser.AssetListView.ColumnListTableRow"),
			InOwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override
	{
		if(ColumnId == ValidatorListColumns::ColumnID_Type)
		{
			return MakeTextCell(Validator.IsValid() ? Validator->GetTypeValidator() : FString(), ETextJustify::Center);
		}

		if(ColumnId == ValidatorListColumns::ColumnID_Name)
		{
			return MakeTextCell(ValidatorWidgetUtils::GetValidatorDisplayName(Validator.Get()), ETextJustify::Left);
		}

		if(ColumnId == ValidatorListColumns::ColumnID_State)
		{
			return MakeStateCell();
		}

		if(ColumnId == ValidatorListColumns::ColumnID_Enabled)
		{
			return SNew(SBox)
				.Padding(4.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SCheckBox)
						.IsChecked_Lambda([this]
							{
								return Validator.IsValid() && Validator->IsEnabled()
									? ECheckBoxState::Checked
									: ECheckBoxState::Unchecked;
							})
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
							{
								if(Validator.IsValid())
								{
									Validator->SetValidationEnabled(NewState == ECheckBoxState::Checked);
									Invalidate(EInvalidateWidgetReason::Paint);
									OnValidatorChanged.ExecuteIfBound();
								}
							})
				];
		}

		return SNullWidget::NullWidget;
	}

private:
	TSharedRef<SWidget> MakeTextCell(const FString& Text, ETextJustify::Type Justification) const
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

	TSharedRef<SWidget> MakeStateCell() const
	{
		return SNew(SBox)
			.Padding(6.0f, 4.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
					.Text_Lambda([this]
						{
							const bool bEnabled = Validator.IsValid() && Validator->IsEnabled();
							return bEnabled ? FText::FromString(TEXT("Enabled")) : FText::FromString(TEXT("Disabled"));
						})
					.Font(LocalFont)
					.Justification(ETextJustify::Center)
			];
	}

	TWeakObjectPtr<UValidatorXBase> Validator;
	FSlateFontInfo LocalFont;
	FSimpleDelegate OnValidatorChanged;
};

void SValidatorWidget::Construct(const FArguments& InArgs)
{
	AllValidators = InArgs._Validators;
	RefreshFilteredValidators();

	FontInfo = FAppStyle::GetFontStyle("NormalFont");
	FontInfo.Size = 11.0f;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(this, &SValidatorWidget::GetSummaryText)
					.Font(FAppStyle::GetFontStyle("NormalFontBold"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
					.Text(FText::FromString("Enable All"))
					.OnClicked(this, &SValidatorWidget::EnableAllValidators)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
					.Text(FText::FromString("Disable All"))
					.OnClicked(this, &SValidatorWidget::DisableAllValidators)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 6.0f)
		[
			SNew(SSearchBox)
				.HintText(FText::FromString("Filter by validator or asset type"))
				.OnTextChanged(this, &SValidatorWidget::OnSearchTextChanged)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator).Thickness(1.0f)
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(4.0f)
		[
			SAssignNew(ListViewWidget, SListView<TWeakObjectPtr<UValidatorXBase>>)
				.ListItemsSource(&FilteredValidators)
				.OnGenerateRow(this, &SValidatorWidget::OnGenerateRowForList)
				.SelectionMode(ESelectionMode::None)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column(ValidatorListColumns::ColumnID_Type)
					.FixedWidth(140.0f)
					[
						SNew(STextBlock).Text(FText::FromString("Type")).Justification(ETextJustify::Center).Font(FontInfo)
					]

					+ SHeaderRow::Column(ValidatorListColumns::ColumnID_Name)
					.FillWidth(1.0f)
					[
						SNew(STextBlock).Text(FText::FromString("Validator")).Font(FontInfo)
					]

					+ SHeaderRow::Column(ValidatorListColumns::ColumnID_State)
					.FixedWidth(90.0f)
					[
						SNew(STextBlock).Text(FText::FromString("State")).Justification(ETextJustify::Center).Font(FontInfo)
					]

					+ SHeaderRow::Column(ValidatorListColumns::ColumnID_Enabled)
					.FixedWidth(72.0f)
					[
						SNew(STextBlock).Text(FText::FromString("Enabled")).Justification(ETextJustify::Center).Font(FontInfo)
					]
				)
		]
	];
}

void SValidatorWidget::RefreshFilteredValidators()
{
	FilteredValidators.Reset();

	const FString Search = SearchText.ToString();
	for(const TWeakObjectPtr<UValidatorXBase>& Validator : AllValidators)
	{
		if(ValidatorWidgetUtils::MatchesSearch(Validator.Get(), Search))
		{
			FilteredValidators.Add(Validator);
		}
	}
}

void SValidatorWidget::RefreshList()
{
	AllValidators = FValidatorXManager::Get().GetValidators();
	RefreshFilteredValidators();

	if(ListViewWidget.IsValid())
	{
		ListViewWidget->RequestListRefresh();
		ListViewWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SValidatorWidget::OnSearchTextChanged(const FText& NewSearchText)
{
	SearchText = NewSearchText;
	RefreshList();
}

FReply SValidatorWidget::EnableAllValidators()
{
	FValidatorXManager::Get().SetAllValidatorsEnabled(true);
	RefreshList();
	return FReply::Handled();
}

FReply SValidatorWidget::DisableAllValidators()
{
	FValidatorXManager::Get().SetAllValidatorsEnabled(false);
	RefreshList();
	return FReply::Handled();
}

FText SValidatorWidget::GetSummaryText() const
{
	int32 EnabledCount = 0;
	for(const TWeakObjectPtr<UValidatorXBase>& Validator : AllValidators)
	{
		if(Validator.IsValid() && Validator->IsEnabled())
		{
			++EnabledCount;
		}
	}

	return FText::Format(
		INVTEXT("ValidatorX: {0} enabled / {1} total ({2} shown)"),
		FText::AsNumber(EnabledCount),
		FText::AsNumber(AllValidators.Num()),
		FText::AsNumber(FilteredValidators.Num()));
}

TSharedRef<ITableRow> SValidatorWidget::OnGenerateRowForList(TWeakObjectPtr<UValidatorXBase> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SValidatorTableRow, OwnerTable)
		.Validator(InItem)
		.Font(FontInfo)
		.OnValidatorChanged(FSimpleDelegate::CreateSP(this, &SValidatorWidget::RefreshList));
}
