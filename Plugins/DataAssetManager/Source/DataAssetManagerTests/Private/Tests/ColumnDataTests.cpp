// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "DataAssetManagerTypes.h"
#include "Tests/DataAssetManagerTestTypes.h"

#include "Misc/AutomationTest.h"
#include "UI/SDataAssetManagerWidget.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FColumnDataTests,
	"DataAssetManager.Data.FColumnDataTests",
	DataAssetManagerFlags::Flags)

void FColumnDataTests::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("InitializeColumnOrderTest"));
	OutTestCommands.Add(TEXT("InitializeColumnOrderTest"));

	OutBeautifiedNames.Add(TEXT("InitializeColumnDataTest"));
	OutTestCommands.Add(TEXT("InitializeColumnDataTest"));
}

bool FColumnDataTests::RunTest(const FString& Parameters)
{
	FColumnData ColumnData;

	if (Parameters == TEXT("InitializeColumnOrderTest"))
	{
		ColumnData.InitializeColumnOrder();
		TestEqual(TEXT("ColumnOrder should have 6 columns"), ColumnData.ColumnOrder.Num(), 6);
		TestTrue(TEXT("First column is RC"), ColumnData.ColumnOrder[0] == DataAssetListColumns::ColumnID_RC);
		TestTrue(TEXT("Second column is Validation"), ColumnData.ColumnOrder[1] == DataAssetListColumns::ColumnID_Validation);
		TestTrue(TEXT("Third column is Name"), ColumnData.ColumnOrder[2] == DataAssetListColumns::ColumnID_Name);
		TestTrue(TEXT("Fourth column is Type"), ColumnData.ColumnOrder[3] == DataAssetListColumns::ColumnID_Type);
		TestTrue(TEXT("Fifth column is DiskSize"), ColumnData.ColumnOrder[4] == DataAssetListColumns::ColumnID_DiskSize);
		TestTrue(TEXT("Sixth column is Path"), ColumnData.ColumnOrder[5] == DataAssetListColumns::ColumnID_Path);
	}

	if (Parameters == TEXT("InitializeColumnDataTest"))
	{
		ColumnData.ColumnVisibility.bShowDiskSizeColumn = true;
		ColumnData.ColumnVisibility.bShowPathColumn = true;
		ColumnData.ColumnVisibility.bShowTypeColumn = true;
		ColumnData.ColumnVisibility.bShowRevisionColumn = true;
		ColumnData.ColumnVisibility.bShowValidationColumn = true;

		TSharedRef<STestDataAssetManagerWidget> Widget = SNew(STestDataAssetManagerWidget);
		TestNotNull(TEXT("Widget pointer should not be null"), &Widget.Get());

		SHeaderRow::FColumn::FArguments RevisionControlColumn = Widget->CreateRevisionControlColumn_Test();
		TestNotNull(TEXT("Revision Control column arguments should not be null"), &RevisionControlColumn);

		ColumnData.InitializeColumnAdders(
			[Widget](TSharedPtr<SHeaderRow> HeaderRow, FName ColumnId, const TCHAR* Label, float Width)
			{
				if (HeaderRow.IsValid())
				{
					Widget->AddColumnToHeader_Test(HeaderRow.ToSharedRef(), ColumnId, Label, Width);
				}
			},
			[RevisionControlColumn]()
			{
				return RevisionControlColumn;
			});

		TSharedRef<SHeaderRow> HeaderRow = ColumnData.BuildHeaderRow();
		TestNotNull(TEXT("HeaderRow should not be null"), &HeaderRow.Get());

		ColumnData.ToggleAllColumnsVisibility();
		TestTrue(TEXT("All columns should be hidden after toggle"), ColumnData.AreAllColumnsHidden());

		ColumnData.ToggleAllColumnsVisibility();
		TestFalse(TEXT("All columns should be visible after second toggle"), ColumnData.AreAllColumnsHidden());
	}

	return true;
}

#endif
