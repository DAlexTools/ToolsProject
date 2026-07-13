// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"

/**
 * @brief One differing property row in the Data Asset diff window.
 */
struct FDataAssetDiffEntry final
{
	/** @brief Internal property name. */
	FName PropertyName;

	/** @brief User-facing property label. */
	FText DisplayName;

	/** @brief Optional property category from metadata. */
	FString Category;

	/** @brief Exported value from the left Data Asset. */
	FString LeftValue;

	/** @brief Exported value from the right Data Asset. */
	FString RightValue;

	/** @brief Returns stable sort text for the row. */
	FString GetSortKey() const
	{
		return FString::Printf(TEXT("%s.%s"), *Category, *DisplayName.ToString());
	}
};

/**
 * @brief Full Data Asset diff result for two selected assets.
 */
struct FDataAssetDiffResult final
{
	/** @brief Left side asset data. */
	TSharedPtr<FAssetData> LeftAsset;

	/** @brief Right side asset data. */
	TSharedPtr<FAssetData> RightAsset;

	/** @brief Differing editable properties. */
	TArray<TSharedPtr<FDataAssetDiffEntry>> Entries;

	/** @brief true when both assets were loaded and share the same class. */
	bool bComparable = false;

	/** @brief Reason why the assets could not be compared. */
	FText ErrorText;

	/** @brief Returns true when comparable assets contain at least one property difference. */
	bool HasDifferences() const
	{
		return Entries.Num() > 0;
	}

	bool IsEmptyErrorText() const
	{
		return ErrorText.IsEmpty();
	}

	bool IsValidLeftAsset() const
	{
		return LeftAsset.IsValid();
	}

	bool IsValidRightAsset() const
	{
		return RightAsset.IsValid();
	}



};
