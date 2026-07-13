// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Tests/DataAssetManagerTestTypes.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FDataAssetManagerWidgetTest,
	"DataAssetManager.UI.SDataAssetManagerWidget",
	DataAssetManagerFlags::Flags)

void FDataAssetManagerWidgetTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("IsSelectedAssetValid"));
	OutTestCommands.Add(TEXT("IsSelectedAssetValid"));
}

bool FDataAssetManagerWidgetTest::RunTest(const FString& Parameters)
{
	if (Parameters == TEXT("IsSelectedAssetValid"))
	{
		TSharedRef<STestDataAssetManagerWidget> Widget = SNew(STestDataAssetManagerWidget);

		const bool bValid = Widget->IsSelectedAssetValid_Test(TEXT(""));
		TestTrue(TEXT("Selected asset should be valid"), bValid);

		const bool bValidWithMsg = Widget->IsSelectedAssetValid_Test(TEXT("Custom error message"));
		TestTrue(TEXT("Selected asset should be valid with custom message"), bValidWithMsg);
	}

	return true;
}

#endif
