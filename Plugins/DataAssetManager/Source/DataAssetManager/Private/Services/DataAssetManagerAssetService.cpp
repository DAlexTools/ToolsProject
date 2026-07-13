// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Services/DataAssetManagerAssetService.h"

#include "Algo/AnyOf.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "Engine/DataAsset.h"
#include "FileHelpers.h"
#include "FunctionLibraries/DataAssetManagerFunctionLibrary.h"
#include "IAssetTools.h"
#include "Interfaces/IPluginManager.h"
#include "Logging/MessageLog.h"
#include "Logging/DataAssetManagerLog.h"
#include "Misc/DataValidation.h"
#include "Models/DataAssetListModel.h"
#include "ScopedTransaction.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Types/DataAssetManagerConstants.h"

#define LOCTEXT_NAMESPACE "DataAssetManagerAssetService"

namespace
{
	FString NormalizeLongPackagePath(FString InPath)
	{
		FPaths::NormalizeDirectoryName(InPath);
		return InPath;
	}

	bool ResolveDataAsset(const TSharedPtr<FAssetData>& AssetData, UObject*& OutAsset)
	{
		OutAsset = nullptr;
		if (!AssetData.IsValid())
		{
			return false;
		}

		OutAsset = AssetData->GetAsset();
		return IsValid(OutAsset) && OutAsset->IsA<UDataAsset>();
	}

	FText GetValidationResultText(EDataValidationResult Result)
	{
		switch (Result)
		{
		case EDataValidationResult::Valid:
			return LOCTEXT("ValidationResultValid", "valid");
		case EDataValidationResult::Invalid:
			return LOCTEXT("ValidationResultInvalid", "invalid");
		default:
			return LOCTEXT("ValidationResultNotValidated", "not validated");
		}
	}

	FDataAssetValidationState MakeValidationState(EDataValidationResult Result, int32 NumWarnings, int32 NumErrors)
	{
		FDataAssetValidationState State;
		State.NumWarnings = NumWarnings;
		State.NumErrors = NumErrors;

		if (Result == EDataValidationResult::Invalid || NumErrors > 0)
		{
			State.Status = EDataAssetValidationStatus::Invalid;
			State.Summary = FText::Format(
				LOCTEXT("ValidationInvalidSummary", "Invalid: {0} error(s), {1} warning(s)."),
				FText::AsNumber(NumErrors),
				FText::AsNumber(NumWarnings));
		}
		else if (NumWarnings > 0)
		{
			State.Status = EDataAssetValidationStatus::Warning;
			State.Summary = FText::Format(
				LOCTEXT("ValidationWarningSummary", "Valid with {0} warning(s)."),
				FText::AsNumber(NumWarnings));
		}
		else if (Result == EDataValidationResult::Valid)
		{
			State.Status = EDataAssetValidationStatus::Valid;
			State.Summary = LOCTEXT("ValidationValidSummary", "Valid.");
		}
		else
		{
			State.Status = EDataAssetValidationStatus::Unknown;
			State.Summary = GetValidationResultText(Result);
		}

		return State;
	}

	bool ShouldShowUnresolvedPackage(FName PackageName)
	{
		const FString PackageString = PackageName.ToString();
		return !PackageString.StartsWith(TEXT("/Script/"));
	}

	void SortReferenceEntries(TArray<TSharedPtr<FDataAssetReferenceEntry>>& Entries)
	{
		Entries.Sort(
			[](const TSharedPtr<FDataAssetReferenceEntry>& Left, const TSharedPtr<FDataAssetReferenceEntry>& Right)
			{
				if (!Left.IsValid())
				{
					return false;
				}

				if (!Right.IsValid())
				{
					return true;
				}

				return Left->GetSortKey().Compare(Right->GetSortKey(), ESearchCase::IgnoreCase) < 0;
			});
	}

	void AppendReferenceEntries(
		IAssetRegistry& AssetRegistry,
		const TArray<FName>& PackageNames,
		FName SourcePackageName,
		TArray<TSharedPtr<FDataAssetReferenceEntry>>& OutEntries,
		TArray<FName>& OutUnresolvedPackages)
	{
		TSet<FName> AddedPackages;
		for (const FName PackageName : PackageNames)
		{
			if (PackageName.IsNone() || PackageName == SourcePackageName || AddedPackages.Contains(PackageName))
			{
				continue;
			}

			AddedPackages.Add(PackageName);

			TArray<FAssetData> PackageAssets;
			if (AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets, true) && PackageAssets.Num() > 0)
			{
				PackageAssets.Sort(
					[](const FAssetData& Left, const FAssetData& Right)
					{
						return Left.AssetName.LexicalLess(Right.AssetName);
					});

				for (const FAssetData& PackageAsset : PackageAssets)
				{
					TSharedPtr<FDataAssetReferenceEntry> Entry = MakeShared<FDataAssetReferenceEntry>();
					Entry->PackageName = PackageName;
					Entry->AssetData = PackageAsset;
					Entry->bAssetDataResolved = true;

					if (const UClass* AssetClass = PackageAsset.GetClass(EResolveClass::No))
					{
						Entry->bIsDataAsset = AssetClass->IsChildOf(UDataAsset::StaticClass());
					}

					OutEntries.Add(Entry);
				}

				continue;
			}

			if (!ShouldShowUnresolvedPackage(PackageName))
			{
				continue;
			}

			TSharedPtr<FDataAssetReferenceEntry> Entry = MakeShared<FDataAssetReferenceEntry>();
			Entry->PackageName = PackageName;
			OutEntries.Add(Entry);
			OutUnresolvedPackages.AddUnique(PackageName);
		}

		SortReferenceEntries(OutEntries);
	}

	bool ShouldDiffProperty(const FProperty* Property)
	{
		if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit))
		{
			return false;
		}

		constexpr EPropertyFlags IgnoredFlags =
			CPF_Transient |
			CPF_Deprecated |
			CPF_DuplicateTransient |
			CPF_NonPIEDuplicateTransient;

		return !Property->HasAnyPropertyFlags(IgnoredFlags);
	}

	bool ArePropertyValuesIdentical(const FProperty* Property, const UObject* LeftAsset, const UObject* RightAsset)
	{
		for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
		{
			if (!Property->Identical_InContainer(LeftAsset, RightAsset, Index))
			{
				return false;
			}
		}

		return true;
	}

	FString ExportPropertyValue(const FProperty* Property, UObject* Asset)
	{
		FString Value;
		for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
		{
			FString ItemValue;
			Property->ExportText_InContainer(Index, ItemValue, Asset, nullptr, Asset, PPF_None);

			if (Property->ArrayDim > 1)
			{
				if (!Value.IsEmpty())
				{
					Value += TEXT(", ");
				}

				Value += FString::Printf(TEXT("[%d]=%s"), Index, *ItemValue);
				continue;
			}

			Value = MoveTemp(ItemValue);
		}

		return Value;
	}

	void SortDiffEntries(TArray<TSharedPtr<FDataAssetDiffEntry>>& Entries)
	{
		Entries.Sort(
			[](const TSharedPtr<FDataAssetDiffEntry>& Left, const TSharedPtr<FDataAssetDiffEntry>& Right)
			{
				if (!Left.IsValid())
				{
					return false;
				}

				if (!Right.IsValid())
				{
					return true;
				}

				return Left->GetSortKey().Compare(Right->GetSortKey(), ESearchCase::IgnoreCase) < 0;
			});
	}
}

void FDataAssetManagerAssetService::LoadDataAssets( const UDataAssetManagerSettings* PluginSettings, TArray<TSharedPtr<FAssetData>>& OutDataAssets, TArray<TSharedPtr<FString>>& OutPluginFilterItems)
{
	OutDataAssets.Reset();
	OutPluginFilterItems.Reset();

	if (!IsValid(PluginSettings))
	{
		return;
	}

	TArray<FString> AssetDirectories;
	AssetDirectories.Reserve(PluginSettings->ScannedAssetDirectories.Num());

	for (const FDirectoryPath& Dir : PluginSettings->ScannedAssetDirectories)
	{
		FString NormalizedPath = Dir.Path;
		FPaths::NormalizeDirectoryName(NormalizedPath);
		AssetDirectories.Add(NormalizedPath);
	}

	for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetEnabledPlugins())
	{
		if (Plugin->GetLoadedFrom() != EPluginLoadedFrom::Project)
		{
			continue;
		}

		FString MountPoint = Plugin->GetMountedAssetPath();
		if (!MountPoint.IsEmpty())
		{
			AssetDirectories.Add(MountPoint);
			OutPluginFilterItems.Add(MakeShared<FString>(MountPoint));
		}
	}

	TArray<FTopLevelAssetPath> IgnoredClassPaths;
	IgnoredClassPaths.Reserve(PluginSettings->ExcludedScanAssetTypes.Num());
	for (const TSubclassOf<UDataAsset>& IgnoredClass : PluginSettings->ExcludedScanAssetTypes)
	{
		if (IgnoredClass)
		{
			IgnoredClassPaths.Add(IgnoredClass->GetClassPathName());
		}
	}

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetDataArray;
	const FTopLevelAssetPath DataAssetPath = UDataAsset::StaticClass()->GetClassPathName();
	if (!AssetRegistry.GetAssetsByClass(DataAssetPath, AssetDataArray, true))
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("%hs Failed to get assets by class"), __FUNCTION__);
		return;
	}

	OutDataAssets.Reserve(AssetDataArray.Num());
	for (const FAssetData& AssetData : AssetDataArray)
	{
		if (IgnoredClassPaths.Contains(AssetData.AssetClassPath))
		{
			continue;
		}

		FString NormalizedAssetPath = AssetData.PackagePath.ToString();
		FPaths::NormalizeDirectoryName(NormalizedAssetPath);

		if (Algo::AnyOf(AssetDirectories,
			[&NormalizedAssetPath](const FString& Directory)
			{
				return NormalizedAssetPath.StartsWith(Directory);
			}))
		{
			OutDataAssets.Add(MakeShared<FAssetData>(AssetData));
		}
	}

	FDataAssetListModel::SortByAssetName(OutDataAssets);
}

bool FDataAssetManagerAssetService::SaveAsset(const TSharedPtr<FAssetData>& AssetData)
{
	if (!AssetData.IsValid())
	{
		return false;
	}

	UDataAsset* DataAsset = Cast<UDataAsset>(AssetData->GetAsset());
	if (!IsValid(DataAsset))
	{
		return false;
	}

	UPackage* const AssetPackage = DataAsset->GetOutermost();
	const FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPackage->GetName(), FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_NoFlags;
	SaveArgs.Error = GError;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.bWarnOfLongFilename = false;

	const bool bSaved = UPackage::SavePackage(AssetPackage, DataAsset, *PackageFileName, SaveArgs);
	if (bSaved && CVarDebugDataAssetManager.GetValueOnAnyThread())
	{
		UE_LOG(SDataAssetManagerLog, Log, TEXT("DataAsset saved successfully: %s"), *PackageFileName);
	}

	return bSaved;
}

int32 FDataAssetManagerAssetService::SaveAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList)
{
	int32 SavedCount = 0;
	for (const TSharedPtr<FAssetData>& AssetData : AssetDataList)
	{
		if (SaveAsset(AssetData))
		{
			++SavedCount;
		}
	}

	return SavedCount;
}

bool FDataAssetManagerAssetService::RenameAsset(const TSharedPtr<FAssetData>& AssetData, const FString& NewName)
{
	UObject* Asset = nullptr;
	if (!ResolveDataAsset(AssetData, Asset) || NewName.IsEmpty() || Asset->GetName() == NewName)
	{
		return false;
	}

	const FString PackagePath = FPaths::GetPath(Asset->GetPathName());
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(DataAssetManager::ModuleName::AssetTools).Get();

	FScopedTransaction Transaction(LOCTEXT("RenameDataAssetTransaction", "Rename Data Asset"));
	if (AssetTools.RenameAssets({ FAssetRenameData(Asset, PackagePath, NewName) }))
	{
		return true;
	}

	Transaction.Cancel();
	return false;
}

int32 FDataAssetManagerAssetService::DuplicateAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList, TArray<FAssetData>* OutDuplicatedAssets)
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(DataAssetManager::ModuleName::AssetTools).Get();
	int32 DuplicatedCount = 0;

	FScopedTransaction Transaction(LOCTEXT("DuplicateDataAssetsTransaction", "Duplicate Data Assets"));
	for (const TSharedPtr<FAssetData>& AssetData : AssetDataList)
	{
		UObject* Asset = nullptr;
		if (!ResolveDataAsset(AssetData, Asset))
		{
			continue;
		}

		const FString SourcePackageName = Asset->GetOutermost()->GetName();
		FString UniquePackageName;
		FString UniqueAssetName;
		AssetTools.CreateUniqueAssetName(SourcePackageName, TEXT("_Copy"), UniquePackageName, UniqueAssetName);

		const FString DestinationPath = FPackageName::GetLongPackagePath(UniquePackageName);
		if (UObject* DuplicatedAsset = AssetTools.DuplicateAsset(UniqueAssetName, DestinationPath, Asset))
		{
			++DuplicatedCount;
			if (OutDuplicatedAssets)
			{
				OutDuplicatedAssets->Add(FAssetData(DuplicatedAsset));
			}
		}
	}

	if (DuplicatedCount == 0)
	{
		Transaction.Cancel();
	}

	return DuplicatedCount;
}

int32 FDataAssetManagerAssetService::MoveAssets(
	const TArray<TSharedPtr<FAssetData>>& AssetDataList,
	const FString& DestinationPath,
	TArray<FAssetData>* OutMovedAssets)
{
	const FString NormalizedDestinationPath = NormalizeLongPackagePath(DestinationPath);
	if (!FPackageName::IsValidLongPackageName(NormalizedDestinationPath))
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("Invalid destination path for moving Data Assets: %s"), *NormalizedDestinationPath);
		return 0;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(DataAssetManager::ModuleName::AssetTools).Get();
	TArray<FAssetRenameData> RenameData;
	TArray<UObject*> AssetsToMove;
	RenameData.Reserve(AssetDataList.Num());
	AssetsToMove.Reserve(AssetDataList.Num());

	for (const TSharedPtr<FAssetData>& AssetData : AssetDataList)
	{
		UObject* Asset = nullptr;
		if (!ResolveDataAsset(AssetData, Asset))
		{
			continue;
		}

		const FString CurrentPackagePath = NormalizeLongPackagePath(AssetData->PackagePath.ToString());
		if (CurrentPackagePath.Equals(NormalizedDestinationPath, ESearchCase::IgnoreCase))
		{
			continue;
		}

		const FString BasePackageName = NormalizedDestinationPath / AssetData->AssetName.ToString();
		FString UniquePackageName;
		FString UniqueAssetName;
		AssetTools.CreateUniqueAssetName(BasePackageName, TEXT(""), UniquePackageName, UniqueAssetName);

		RenameData.Add(FAssetRenameData(Asset, FPackageName::GetLongPackagePath(UniquePackageName), UniqueAssetName));
		AssetsToMove.Add(Asset);
	}

	if (RenameData.Num() == 0)
	{
		return 0;
	}

	FScopedTransaction Transaction(LOCTEXT("MoveDataAssetsTransaction", "Move Data Assets"));
	if (!AssetTools.RenameAssets(RenameData))
	{
		Transaction.Cancel();
		return 0;
	}

	if (OutMovedAssets)
	{
		for (UObject* Asset : AssetsToMove)
		{
			if (IsValid(Asset))
			{
				OutMovedAssets->Add(FAssetData(Asset));
			}
		}
	}

	return RenameData.Num();
}

bool FDataAssetManagerAssetService::DeleteAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList, bool bShowConfirmation)
{
	return DataAssetManager::DeleteMultiplyAsset(ToAssetDataArray(AssetDataList), bShowConfirmation);
}

bool FDataAssetManagerAssetService::SaveAllDataAssets()
{
	constexpr bool bPromptUserToSave = false;
	constexpr bool bSaveMapPackages = true;
	constexpr bool bSaveContentPackages = true;
	constexpr bool bFastSave = false;
	constexpr bool bNotifyNoPackagesSaved = false;
	constexpr bool bCanBeDeclined = false;

	return FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave, bNotifyNoPackagesSaved, bCanBeDeclined);
}

FDataAssetValidationResults FDataAssetManagerAssetService::ValidateAssets(const TArray<TSharedPtr<FAssetData>>& AssetDataList, bool bOpenMessageLog)
{
	FDataAssetValidationResults Results;
	FMessageLog MessageLog("AssetCheck");
	if (bOpenMessageLog)
	{
		MessageLog.NewPage(LOCTEXT("DataAssetValidationPage", "Data Asset Validation"));
	}

	for (const TSharedPtr<FAssetData>& AssetData : AssetDataList)
	{
		UObject* Asset = nullptr;
		if (!ResolveDataAsset(AssetData, Asset))
		{
			if (AssetData.IsValid())
			{
				FDataAssetValidationState State;
				State.Status = EDataAssetValidationStatus::Invalid;
				State.Summary = LOCTEXT("ValidationAssetLoadFailedSummary", "Failed to load Data Asset.");
				State.NumErrors = 1;

				Results.InvalidPackages.Add(AssetData->PackageName);
				Results.StatesByPackage.Add(AssetData->PackageName, State);
				if (bOpenMessageLog)
				{
					MessageLog.Error(FText::Format(
						LOCTEXT("ValidationAssetLoadFailed", "{0}: failed to load Data Asset."),
						FText::FromName(AssetData->AssetName)));
				}
			}
			continue;
		}

		TArray<FAssetData> AssociatedObjects;
		FDataValidationContext ValidationContext(false, EDataValidationUsecase::Manual, AssociatedObjects);
		const UObject* const AssetForValidation = Asset;
		const EDataValidationResult Result = AssetForValidation->IsDataValid(ValidationContext);

		TArray<FText> Warnings;
		TArray<FText> Errors;
		ValidationContext.SplitIssues(Warnings, Errors);

		const bool bInvalid = Result == EDataValidationResult::Invalid || Errors.Num() > 0;
		if (AssetData.IsValid())
		{
			Results.StatesByPackage.Add(AssetData->PackageName, MakeValidationState(Result, Warnings.Num(), Errors.Num()));
			if (bInvalid)
			{
				Results.InvalidPackages.Add(AssetData->PackageName);
			}
		}

		if (!bOpenMessageLog)
		{
			continue;
		}

		const FText AssetName = FText::FromString(Asset->GetPathName());
		if (Warnings.Num() == 0 && Errors.Num() == 0)
		{
			const EMessageSeverity::Type Severity = bInvalid ? EMessageSeverity::Error : EMessageSeverity::Info;
			MessageLog.Message(
				Severity,
				FText::Format(LOCTEXT("ValidationResultSummary", "{0}: {1}."), AssetName, GetValidationResultText(Result)));
			continue;
		}

		for (const FText& Error : Errors)
		{
			MessageLog.Error(FText::Format(LOCTEXT("ValidationErrorFormat", "{0}: {1}"), AssetName, Error));
		}

		for (const FText& Warning : Warnings)
		{
			MessageLog.Warning(FText::Format(LOCTEXT("ValidationWarningFormat", "{0}: {1}"), AssetName, Warning));
		}
	}

	if (bOpenMessageLog)
	{
		MessageLog.Open(EMessageSeverity::Info, true);
	}

	return Results;
}

FDataAssetReferenceInspectionResult FDataAssetManagerAssetService::InspectReferences(const TSharedPtr<FAssetData>& AssetData)
{
	FDataAssetReferenceInspectionResult Result;
	Result.SourceAsset = AssetData;
	if (!AssetData.IsValid())
	{
		return Result;
	}

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	UE::AssetRegistry::FDependencyQuery PackageQuery;

	TArray<FName> DependencyPackages;
	AssetRegistry.GetDependencies(
		AssetData->PackageName,
		DependencyPackages,
		UE::AssetRegistry::EDependencyCategory::Package,
		PackageQuery);

	TArray<FName> ReferencerPackages;
	AssetRegistry.GetReferencers(
		AssetData->PackageName,
		ReferencerPackages,
		UE::AssetRegistry::EDependencyCategory::Package,
		PackageQuery);

	AppendReferenceEntries(AssetRegistry, DependencyPackages, AssetData->PackageName, Result.References, Result.UnresolvedPackages);
	AppendReferenceEntries(AssetRegistry, ReferencerPackages, AssetData->PackageName, Result.ReferencedBy, Result.UnresolvedPackages);

	return Result;
}

FDataAssetDiffResult FDataAssetManagerAssetService::DiffAssets(
	const TSharedPtr<FAssetData>& LeftAssetData,
	const TSharedPtr<FAssetData>& RightAssetData)
{
	FDataAssetDiffResult Result;
	Result.LeftAsset = LeftAssetData;
	Result.RightAsset = RightAssetData;

	UObject* LeftAsset = nullptr;
	UObject* RightAsset = nullptr;
	if (!ResolveDataAsset(LeftAssetData, LeftAsset) || !ResolveDataAsset(RightAssetData, RightAsset))
	{
		Result.ErrorText = LOCTEXT("DiffAssetLoadFailed", "Failed to load both selected Data Assets.");
		return Result;
	}

	const UClass* LeftClass = LeftAsset->GetClass();
	const UClass* RightClass = RightAsset->GetClass();
	if (LeftClass != RightClass)
	{
		Result.ErrorText = FText::Format(
			LOCTEXT("DiffClassMismatch", "DataAsset Diff requires the same class. Left: {0}. Right: {1}."),
			FText::FromString(IsValid(LeftClass) ? LeftClass->GetName() : FString(TEXT("Unknown"))),
			FText::FromString(IsValid(RightClass) ? RightClass->GetName() : FString(TEXT("Unknown"))));
		return Result;
	}

	Result.bComparable = true;
	for (TFieldIterator<FProperty> PropertyIterator(LeftClass, EFieldIteratorFlags::IncludeSuper); PropertyIterator; ++PropertyIterator)
	{
		const FProperty* Property = *PropertyIterator;
		if (!ShouldDiffProperty(Property) || ArePropertyValuesIdentical(Property, LeftAsset, RightAsset))
		{
			continue;
		}

		TSharedPtr<FDataAssetDiffEntry> Entry = MakeShared<FDataAssetDiffEntry>();
		Entry->PropertyName = Property->GetFName();
		Entry->DisplayName = Property->GetDisplayNameText();
		Entry->Category = Property->GetMetaData(TEXT("Category"));
		Entry->LeftValue = ExportPropertyValue(Property, LeftAsset);
		Entry->RightValue = ExportPropertyValue(Property, RightAsset);
		Result.Entries.Add(Entry);
	}

	SortDiffEntries(Result.Entries);
	return Result;
}

bool FDataAssetManagerAssetService::CopyDiffPropertyValue(
	const TSharedPtr<FAssetData>& SourceAssetData,
	const TSharedPtr<FAssetData>& TargetAssetData,
	FName PropertyName,
	FText* OutErrorText)
{
	UObject* SourceAsset = nullptr;
	UObject* TargetAsset = nullptr;
	if (!ResolveDataAsset(SourceAssetData, SourceAsset) || !ResolveDataAsset(TargetAssetData, TargetAsset))
	{
		if (OutErrorText)
		{
			*OutErrorText = LOCTEXT("CopyDiffAssetLoadFailed", "Failed to load source or target Data Asset.");
		}
		return false;
	}

	const UClass* SourceClass = SourceAsset->GetClass();
	const UClass* TargetClass = TargetAsset->GetClass();
	if (SourceClass != TargetClass)
	{
		if (OutErrorText)
		{
			*OutErrorText = LOCTEXT("CopyDiffClassMismatch", "DataAsset Diff copy requires assets of the same class.");
		}
		return false;
	}

	FProperty* Property = FindFProperty<FProperty>(SourceClass, PropertyName);
	if (!ShouldDiffProperty(Property))
	{
		if (OutErrorText)
		{
			*OutErrorText = LOCTEXT("CopyDiffPropertyUnavailable", "The selected property cannot be copied.");
		}
		return false;
	}

	if (ArePropertyValuesIdentical(Property, SourceAsset, TargetAsset))
	{
		return true;
	}

	FScopedTransaction Transaction(LOCTEXT("CopyDataAssetDiffPropertyTransaction", "Copy Data Asset Property Value"));
	TargetAsset->Modify();

	bool bCopied = false;
	if (Property->ArrayDim == 1)
	{
		const FString SourceValue = ExportPropertyValue(Property, SourceAsset);
		bCopied = Property->ImportText_InContainer(*SourceValue, TargetAsset, TargetAsset, PPF_None) != nullptr;
	}
	else
	{
		Property->CopyCompleteValue_InContainer(TargetAsset, SourceAsset);
		bCopied = true;
	}

	if (!bCopied)
	{
		Transaction.Cancel();
		if (OutErrorText)
		{
			*OutErrorText = FText::Format(
				LOCTEXT("CopyDiffImportFailed", "Failed to import value for property {0}."),
				Property->GetDisplayNameText());
		}
		return false;
	}

	FPropertyChangedEvent ChangedEvent(Property, EPropertyChangeType::ValueSet);
	TargetAsset->PostEditChangeProperty(ChangedEvent);
	TargetAsset->MarkPackageDirty();

	return true;
}

TArray<FAssetData> FDataAssetManagerAssetService::ToAssetDataArray(const TArray<TSharedPtr<FAssetData>>& AssetDataList)
{
	TArray<FAssetData> Assets;
	Assets.Reserve(AssetDataList.Num());

	for (const TSharedPtr<FAssetData>& AssetData : AssetDataList)
	{
		if (AssetData.IsValid())
		{
			Assets.Add(*AssetData);
		}
	}

	return Assets;
}

FString FDataAssetManagerAssetService::BuildClipboardText(const TArray<TSharedPtr<FAssetData>>& AssetDataList, bool bCopyPaths)
{
	TArray<FAssetData> SelectedPackages = ToAssetDataArray(AssetDataList);
	SelectedPackages.Sort(
		[](const FAssetData& One, const FAssetData& Two)
		{
			return One.PackagePath.Compare(Two.PackagePath) < 0;
		});

	return FString::JoinBy(SelectedPackages, LINE_TERMINATOR,
		[bCopyPaths](const FAssetData& Item)
		{
			return DataAssetManager::BuildClipboardEntry(Item, bCopyPaths);
		});
}

void FDataAssetManagerAssetService::ProcessAssetData(
	const TArray<FAssetData>& AssetDataList,
	TFunction<void(const TArray<FAssetIdentifier>&)> ProcessFunction)
{
	DataAssetManager::ProcessAssetData(AssetDataList, MoveTemp(ProcessFunction));
}

#undef LOCTEXT_NAMESPACE
