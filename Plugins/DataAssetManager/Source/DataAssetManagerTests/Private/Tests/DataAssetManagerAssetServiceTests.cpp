// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Services/DataAssetManagerAssetService.h"
#include "Tests/DataAssetManagerTestHelpers.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace DataAssetManagerServiceTests
{
	TSharedPtr<FAssetData> MakeAssetDataPtr(const FAssetData& AssetData)
	{
		return AssetData.IsValid() ? MakeShared<FAssetData>(AssetData) : TSharedPtr<FAssetData>();
	}

	bool ContainsAssetName(const TArray<TSharedPtr<FAssetData>>& Assets, FName AssetName)
	{
		return Assets.ContainsByPredicate(
			[AssetName](const TSharedPtr<FAssetData>& AssetData)
			{
				return AssetData.IsValid() && AssetData->AssetName == AssetName;
			});
	}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FDataAssetManagerAssetServiceTest,
	"DataAssetManager.Services.AssetService",
	DataAssetManagerFlags::Flags)

void FDataAssetManagerAssetServiceTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TEXT("InvalidInputs"));
	OutTestCommands.Add(TEXT("InvalidInputs"));

	OutBeautifiedNames.Add(TEXT("LoadDataAssetsHonorsSettingsAndExclusions"));
	OutTestCommands.Add(TEXT("LoadDataAssetsHonorsSettingsAndExclusions"));

	OutBeautifiedNames.Add(TEXT("SaveRenameDuplicateMoveDelete"));
	OutTestCommands.Add(TEXT("SaveRenameDuplicateMoveDelete"));

	OutBeautifiedNames.Add(TEXT("ValidateInspectAndDiff"));
	OutTestCommands.Add(TEXT("ValidateInspectAndDiff"));

	OutBeautifiedNames.Add(TEXT("ClipboardAndArrayConversion"));
	OutTestCommands.Add(TEXT("ClipboardAndArrayConversion"));
}

bool FDataAssetManagerAssetServiceTest::RunTest(const FString& Parameters)
{
	if (Parameters == TEXT("InvalidInputs"))
	{
		TArray<TSharedPtr<FAssetData>> DataAssets = { MakeShared<FAssetData>() };
		TArray<TSharedPtr<FString>> PluginFilters = { MakeShared<FString>(TEXT("/ShouldBeReset")) };

		FDataAssetManagerAssetService::LoadDataAssets(nullptr, DataAssets, PluginFilters);
		TestEqual(TEXT("LoadDataAssets resets output assets for null settings"), DataAssets.Num(), 0);
		TestEqual(TEXT("LoadDataAssets resets plugin filters for null settings"), PluginFilters.Num(), 0);

		TestFalse(TEXT("SaveAsset rejects null asset data"), FDataAssetManagerAssetService::SaveAsset(nullptr));
		TestEqual(TEXT("SaveAssets ignores null asset data"), FDataAssetManagerAssetService::SaveAssets({ nullptr }), 0);
		TestFalse(TEXT("RenameAsset rejects null asset data"), FDataAssetManagerAssetService::RenameAsset(nullptr, TEXT("Name")));
		TestEqual(TEXT("DuplicateAssets ignores null asset data"), FDataAssetManagerAssetService::DuplicateAssets({ nullptr }), 0);
		TestEqual(TEXT("MoveAssets rejects invalid destination paths"), FDataAssetManagerAssetService::MoveAssets({}, TEXT("Relative/Path")), 0);
		TestFalse(TEXT("DeleteAssets rejects empty lists"), FDataAssetManagerAssetService::DeleteAssets({}));

		const FDataAssetValidationResults EmptyValidation = FDataAssetManagerAssetService::ValidateAssets({}, false);
		TestEqual(TEXT("ValidateAssets returns no states for empty input"), EmptyValidation.StatesByPackage.Num(), 0);
		TestEqual(TEXT("ValidateAssets returns no invalid packages for empty input"), EmptyValidation.InvalidPackages.Num(), 0);

		const FDataAssetReferenceInspectionResult EmptyReferences = FDataAssetManagerAssetService::InspectReferences(nullptr);
		TestFalse(TEXT("InspectReferences keeps invalid source for null input"), EmptyReferences.IsValidSourceAsset());
		TestTrue(TEXT("InspectReferences with null input is potentially unused"), EmptyReferences.IsPotentiallyUnused());

		const FDataAssetDiffResult EmptyDiff = FDataAssetManagerAssetService::DiffAssets(nullptr, nullptr);
		TestFalse(TEXT("DiffAssets rejects null inputs"), EmptyDiff.bComparable);
		TestFalse(TEXT("DiffAssets reports an error for null inputs"), EmptyDiff.IsEmptyErrorText());
	}

	if (Parameters == TEXT("LoadDataAssetsHonorsSettingsAndExclusions"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::DeleteTestRootAssets();
		DataAssetManagerTests::FScopedTestDataAsset IncludedAsset(
			TEXT("LoadIncludedAsset"),
			DataAssetManagerTests::TestRootPath,
			UTestDataAsset::StaticClass());
		DataAssetManagerTests::FScopedTestDataAsset ExcludedAsset(
			TEXT("LoadExcludedAsset"),
			DataAssetManagerTests::TestRootPath,
			UAlternateTestDataAsset::StaticClass());
		DataAssetManagerTests::ScanTestPaths();

		UDataAssetManagerSettings* Settings = NewObject<UDataAssetManagerSettings>(GetTransientPackage(), TEXT("LoadDataAssetsSettings"));
		TestNotNull(TEXT("Settings object should be created"), Settings);
		Settings->ScannedAssetDirectories = { FDirectoryPath{ DataAssetManagerTests::TestRootPath } };
		Settings->ExcludedScanAssetTypes = { UAlternateTestDataAsset::StaticClass() };

		TArray<TSharedPtr<FAssetData>> DataAssets;
		TArray<TSharedPtr<FString>> PluginFilters;
		FDataAssetManagerAssetService::LoadDataAssets(Settings, DataAssets, PluginFilters);

		TestTrue(
			TEXT("LoadDataAssets includes DataAssets from configured scan directories"),
			DataAssetManagerServiceTests::ContainsAssetName(DataAssets, FName(TEXT("LoadIncludedAsset"))));
		TestFalse(
			TEXT("LoadDataAssets excludes configured DataAsset classes"),
			DataAssetManagerServiceTests::ContainsAssetName(DataAssets, FName(TEXT("LoadExcludedAsset"))));

		DataAssetManagerTests::DeleteTestRootAssets();
#endif
	}

	if (Parameters == TEXT("SaveRenameDuplicateMoveDelete"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::DeleteTestRootAssets();
		DataAssetManagerTests::FScopedTestDataAsset TempAsset(DataAssetManagerTests::MakeUniqueAssetName(TEXT("ServiceAsset")));
		UTestDataAsset* Asset = TempAsset.GetAssetAs<UTestDataAsset>();
		TestNotNull(TEXT("Service test asset should be created"), Asset);

		Asset->TestProperty = 11;
		Asset->MarkPackageDirty();
		TestTrue(TEXT("SaveAsset saves valid DataAssets"), FDataAssetManagerAssetService::SaveAsset(TempAsset.MakeSharedAssetData()));
		TestEqual(TEXT("SaveAssets counts only saved assets"), FDataAssetManagerAssetService::SaveAssets({ nullptr, TempAsset.MakeSharedAssetData() }), 1);

		const FString RenamedAssetName = DataAssetManagerTests::MakeUniqueAssetName(TEXT("ServiceRenamedAsset"));
		TestFalse(TEXT("RenameAsset rejects empty names"), FDataAssetManagerAssetService::RenameAsset(TempAsset.MakeSharedAssetData(), TEXT("")));
		TestFalse(TEXT("RenameAsset rejects identical names"), FDataAssetManagerAssetService::RenameAsset(TempAsset.MakeSharedAssetData(), Asset->GetName()));
		TestTrue(TEXT("RenameAsset renames valid DataAssets"), FDataAssetManagerAssetService::RenameAsset(TempAsset.MakeSharedAssetData(), RenamedAssetName));
		TempAsset.RefreshAssetData();
		TestEqual(TEXT("Renamed asset data should expose the new name"), TempAsset.GetAssetData().AssetName, FName(*RenamedAssetName));

		TArray<FAssetData> DuplicatedAssets;
		TestEqual(TEXT("DuplicateAssets duplicates valid DataAssets"), FDataAssetManagerAssetService::DuplicateAssets({ TempAsset.MakeSharedAssetData() }, &DuplicatedAssets), 1);
		TestEqual(TEXT("DuplicateAssets returns duplicated asset data"), DuplicatedAssets.Num(), 1);
		TestTrue(TEXT("Duplicated asset data should be valid"), DuplicatedAssets[0].IsValid());

		TArray<FAssetData> MovedAssets;
		TestEqual(
			TEXT("MoveAssets moves valid DataAssets"),
			FDataAssetManagerAssetService::MoveAssets({ TempAsset.MakeSharedAssetData() }, DataAssetManagerTests::TestMoveRootPath, &MovedAssets),
			1);
		TestEqual(TEXT("MoveAssets returns moved asset data"), MovedAssets.Num(), 1);
		TestEqual(TEXT("Moved asset should have destination package path"), MovedAssets[0].PackagePath, FName(*DataAssetManagerTests::TestMoveRootPath));

		TestEqual(
			TEXT("MoveAssets skips assets already in destination"),
			FDataAssetManagerAssetService::MoveAssets({ DataAssetManagerServiceTests::MakeAssetDataPtr(MovedAssets[0]) }, DataAssetManagerTests::TestMoveRootPath),
			0);
		TestTrue(
			TEXT("DeleteAssets deletes valid DataAssets"),
			FDataAssetManagerAssetService::DeleteAssets({ DataAssetManagerServiceTests::MakeAssetDataPtr(MovedAssets[0]) }));

		DataAssetManagerTests::DeleteTestRootAssets();
#endif
	}

	if (Parameters == TEXT("ValidateInspectAndDiff"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::DeleteTestRootAssets();
		DataAssetManagerTests::FScopedTestDataAsset LeftAsset(DataAssetManagerTests::MakeUniqueAssetName(TEXT("DiffLeft")));
		DataAssetManagerTests::FScopedTestDataAsset RightAsset(DataAssetManagerTests::MakeUniqueAssetName(TEXT("DiffRight")));
		DataAssetManagerTests::FScopedTestDataAsset InvalidAsset(
			DataAssetManagerTests::MakeUniqueAssetName(TEXT("InvalidAsset")),
			DataAssetManagerTests::TestRootPath,
			UInvalidTestDataAsset::StaticClass());
		DataAssetManagerTests::FScopedTestDataAsset AlternateAsset(
			DataAssetManagerTests::MakeUniqueAssetName(TEXT("AlternateAsset")),
			DataAssetManagerTests::TestRootPath,
			UAlternateTestDataAsset::StaticClass());

		UTestDataAsset* Left = LeftAsset.GetAssetAs<UTestDataAsset>();
		UTestDataAsset* Right = RightAsset.GetAssetAs<UTestDataAsset>();
		TestNotNull(TEXT("Left diff asset should be created"), Left);
		TestNotNull(TEXT("Right diff asset should be created"), Right);
		Left->TestProperty = 10;
		Right->TestProperty = 20;

		const FDataAssetValidationResults Validation = FDataAssetManagerAssetService::ValidateAssets(
			{ LeftAsset.MakeSharedAssetData(), InvalidAsset.MakeSharedAssetData() },
			false);
		TestTrue(TEXT("ValidateAssets records valid asset state"), Validation.StatesByPackage.Contains(LeftAsset.GetAssetData().PackageName));
		TestFalse(TEXT("ValidateAssets does not mark valid assets invalid"), Validation.InvalidPackages.Contains(LeftAsset.GetAssetData().PackageName));
		TestTrue(TEXT("ValidateAssets records invalid asset state"), Validation.StatesByPackage.Contains(InvalidAsset.GetAssetData().PackageName));
		TestTrue(TEXT("ValidateAssets marks invalid assets invalid"), Validation.InvalidPackages.Contains(InvalidAsset.GetAssetData().PackageName));

		const FDataAssetReferenceInspectionResult References = FDataAssetManagerAssetService::InspectReferences(LeftAsset.MakeSharedAssetData());
		TestTrue(TEXT("InspectReferences preserves valid source asset data"), References.IsValidSourceAsset());

		const FDataAssetDiffResult Diff = FDataAssetManagerAssetService::DiffAssets(LeftAsset.MakeSharedAssetData(), RightAsset.MakeSharedAssetData());
		TestTrue(TEXT("DiffAssets accepts assets of the same class"), Diff.bComparable);
		TestTrue(TEXT("DiffAssets reports property differences"), Diff.HasDifferences());
		TestTrue(
			TEXT("DiffAssets includes TestProperty changes"),
			Diff.Entries.ContainsByPredicate(
				[](const TSharedPtr<FDataAssetDiffEntry>& Entry)
				{
					return Entry.IsValid() && Entry->PropertyName == GET_MEMBER_NAME_CHECKED(UTestDataAsset, TestProperty);
				}));

		TestTrue(
			TEXT("CopyDiffPropertyValue copies editable property values"),
			FDataAssetManagerAssetService::CopyDiffPropertyValue(
				LeftAsset.MakeSharedAssetData(),
				RightAsset.MakeSharedAssetData(),
				GET_MEMBER_NAME_CHECKED(UTestDataAsset, TestProperty)));
		TestEqual(TEXT("Copied diff value should update the target asset"), Right->TestProperty, Left->TestProperty);

		FText CopyError;
		TestFalse(
			TEXT("CopyDiffPropertyValue rejects missing properties"),
			FDataAssetManagerAssetService::CopyDiffPropertyValue(
				LeftAsset.MakeSharedAssetData(),
				RightAsset.MakeSharedAssetData(),
				FName(TEXT("MissingProperty")),
				&CopyError));
		TestFalse(TEXT("CopyDiffPropertyValue reports an error for missing properties"), CopyError.IsEmpty());

		const FDataAssetDiffResult ClassMismatchDiff = FDataAssetManagerAssetService::DiffAssets(
			LeftAsset.MakeSharedAssetData(),
			AlternateAsset.MakeSharedAssetData());
		TestFalse(TEXT("DiffAssets rejects class mismatches"), ClassMismatchDiff.bComparable);
		TestFalse(TEXT("DiffAssets reports class mismatch errors"), ClassMismatchDiff.IsEmptyErrorText());

		DataAssetManagerTests::DeleteTestRootAssets();
#endif
	}

	if (Parameters == TEXT("ClipboardAndArrayConversion"))
	{
#if WITH_EDITOR
		DataAssetManagerTests::DeleteTestRootAssets();
		DataAssetManagerTests::FScopedTestDataAsset FirstAsset(TEXT("ClipboardA"));
		DataAssetManagerTests::FScopedTestDataAsset SecondAsset(TEXT("ClipboardB"));
		FirstAsset.Save();
		SecondAsset.Save();

		const TArray<TSharedPtr<FAssetData>> AssetDataList = { nullptr, SecondAsset.MakeSharedAssetData(), FirstAsset.MakeSharedAssetData() };
		const TArray<FAssetData> ConvertedAssets = FDataAssetManagerAssetService::ToAssetDataArray(AssetDataList);
		TestEqual(TEXT("ToAssetDataArray filters invalid shared pointers"), ConvertedAssets.Num(), 2);

		const FString ClipboardReferences = FDataAssetManagerAssetService::BuildClipboardText(AssetDataList, false);
		TestTrue(TEXT("BuildClipboardText includes first export reference"), ClipboardReferences.Contains(FirstAsset.GetAssetData().GetExportTextName()));
		TestTrue(TEXT("BuildClipboardText includes second export reference"), ClipboardReferences.Contains(SecondAsset.GetAssetData().GetExportTextName()));
		TestTrue(TEXT("BuildClipboardText joins multiple entries with line terminators"), ClipboardReferences.Contains(LINE_TERMINATOR));

		const FString ClipboardPaths = FDataAssetManagerAssetService::BuildClipboardText(AssetDataList, true);
		TestTrue(TEXT("BuildClipboardText can copy filesystem paths"), ClipboardPaths.Contains(TEXT(".uasset")));

		bool bProcessCallbackCalled = false;
		FDataAssetManagerAssetService::ProcessAssetData(ConvertedAssets,
			[&bProcessCallbackCalled, this](const TArray<FAssetIdentifier>& Identifiers)
			{
				bProcessCallbackCalled = true;
				TestTrue(TEXT("AssetService ProcessAssetData forwards identifiers"), Identifiers.Num() >= 2);
			});
		TestTrue(TEXT("AssetService ProcessAssetData should call callback"), bProcessCallbackCalled);

		DataAssetManagerTests::DeleteTestRootAssets();
#endif
	}

	return true;
}

#endif
