// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/**
 * @brief One package or asset entry displayed by the Data Asset reference inspector.
 */
struct FDataAssetReferenceEntry final
{
	/** @brief Package name reported by the Asset Registry dependency graph. */
	FName PackageName;

	/** @brief Resolved asset data when the package maps to a registry asset. */
	FAssetData AssetData;

	/** @brief true when AssetData is usable for editor actions. */
	bool bAssetDataResolved = false;

	/** @brief true when the resolved asset class is a Data Asset class or subclass. */
	bool bIsDataAsset = false;

	/** @brief Returns a short display name for the entry. */
	FText GetDisplayName() const
	{
		return bAssetDataResolved
			? FText::FromName(AssetData.AssetName)
			: FText::FromName(PackageName);
	}

	/** @brief Returns the class display name for the entry. */
	FText GetClassDisplayName() const
	{
		return bAssetDataResolved
			? FText::FromName(AssetData.AssetClassPath.GetAssetName())
			: FText::FromString(TEXT("Package"));
	}

	/** @brief Returns the path displayed in the inspector row. */
	FText GetPathText() const
	{
		return bAssetDataResolved
			? FText::FromName(AssetData.PackagePath)
			: FText::FromName(PackageName);
	}

	/** @brief Returns the full package or object path used for tooltips and sorting. */
	FString GetSortKey() const
	{
		return bAssetDataResolved
			? AssetData.GetObjectPathString()
			: PackageName.ToString();
	}
};

/**
 * @brief Full reference inspection data for one selected Data Asset.
 */
struct FDataAssetReferenceInspectionResult final
{
	/** @brief Source Data Asset inspected by the UI. */
	TSharedPtr<FAssetData> SourceAsset;

	/** @brief Assets and packages referenced by SourceAsset. */
	TArray<TSharedPtr<FDataAssetReferenceEntry>> References;

	/** @brief Assets and packages that reference SourceAsset. */
	TArray<TSharedPtr<FDataAssetReferenceEntry>> ReferencedBy;

	/** @brief Dependency packages that did not resolve to asset data. */
	TArray<FName> UnresolvedPackages;

	/** @brief Returns true when Asset Registry has no package referencers for SourceAsset. */
	bool IsPotentiallyUnused() const
	{
		return ReferencedBy.Num() == 0;
	}

	bool IsValidSourceAsset() const
	{
		return SourceAsset.IsValid();
	}
};
