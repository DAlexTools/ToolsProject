// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UValidatorXBase;

/**
 * 
 */
class VALIDATORX_API SValidatorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SValidatorWidget) {}
		SLATE_ARGUMENT(TArray<TWeakObjectPtr<UValidatorXBase>>, Validators)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TArray<TWeakObjectPtr<UValidatorXBase>> AllValidators;
	TArray<TWeakObjectPtr<UValidatorXBase>> FilteredValidators;
	
	TSharedPtr<SListView<TWeakObjectPtr<UValidatorXBase>>> ListViewWidget;

	FSlateFontInfo FontInfo;
	FText SearchText;

	void RefreshFilteredValidators();
	void RefreshList();
	void OnSearchTextChanged(const FText& NewSearchText);
	FReply EnableAllValidators();
	FReply DisableAllValidators();
	FText GetSummaryText() const;

	TSharedRef<ITableRow> OnGenerateRowForList(TWeakObjectPtr<UValidatorXBase> InItem, const TSharedRef<STableViewBase>& OwnerTable);
};
