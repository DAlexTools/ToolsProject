// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "DataAssetManagerTypes.h"
#include "Tests/DataAssetManagerTestTypes.h"

#include "AssetRegistry/AssetData.h"
#include "Misc/AutomationTest.h"

#if !UE_BUILD_SHIPPING && WITH_AUTOMATION_TESTS

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FEditableWidget_AddWidgetTest,
	"DataAssetManager.UI.FEditableWidgets.AddWidget",
	DataAssetManagerFlags::Flags);

void FEditableWidget_AddWidgetTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("AddEditableWidget"));
	OutTestCommands.Add(TEXT("AddEditableWidget"));
}

bool FEditableWidget_AddWidgetTest::RunTest(const FString& Parameters)
{
	if (Parameters == TEXT("AddEditableWidget"))
	{
		FEditableWidgets Widgets;

		const FName TestPackageName = TEXT("/Game/Test/MyAsset");
		const FName TestPackagePath = TEXT("/Game/Test");
		const FName TestAssetName = TEXT("MyAsset");

		const FTopLevelAssetPath DummyClassPath(TEXT("/Script/Engine"), TEXT("Texture2D"));
		const FAssetData AssetData(
			TestPackageName,
			TestPackagePath,
			TestAssetName,
			DummyClassPath,
			{},
			{},
			0);

		TSharedPtr<SEditableText> EditableText = SNew(SEditableText).Text(FText::FromString(TEXT("Test")));
		Widgets.AddEditableTextWidget(&AssetData, EditableText);

		TestEqual(TEXT("EditableTextWidgets size must be 1"), Widgets.EditableTextWidgets.Num(), 1);

		const TPair<FName, FName> Key(TestPackagePath, TestAssetName);
		TestTrue(TEXT("Key must exist after AddEditableTextWidget"), Widgets.EditableTextWidgets.Contains(Key));
		TestTrue(TEXT("Stored widget must be valid"), Widgets.EditableTextWidgets[Key].IsValid());
		TestTrue(TEXT("Stored widget pointer must be same as inserted one"), Widgets.EditableTextWidgets[Key] == EditableText);

		Widgets.AddEditableTextWidget(nullptr, EditableText);
		TestEqual(TEXT("Widgets count must stay unchanged if AssetData is null"), Widgets.EditableTextWidgets.Num(), 1);

		Widgets.AddEditableTextWidget(&AssetData, nullptr);
		TestEqual(TEXT("Widgets count must stay unchanged if EditableText is null"), Widgets.EditableTextWidgets.Num(), 1);
	}

	return true;
}

#endif
