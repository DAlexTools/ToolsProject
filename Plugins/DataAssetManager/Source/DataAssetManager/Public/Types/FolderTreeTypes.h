// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STreeView.h"

/**
 * @brief Node displayed by the folder tree widget.
 */
struct FAssetTreeFolderNode final
{
	FString FolderPath;
	FString FolderName;
	TSharedPtr<FAssetTreeFolderNode> Parent;
	TArray<TSharedPtr<FAssetTreeFolderNode>> SubItems;

	bool bIsDev : 1;
	bool bIsPlugin : 1;
	bool bIsRoot : 1;
	bool bIsEmpty : 1;
	bool bIsExcluded : 1;
	bool bIsExpanded : 1;
	bool bIsVisible : 1;

	/**
	 * @brief Compares folder nodes by normalized folder path.
	 * @param Other Node to compare against.
	 * @return true when both nodes refer to the same folder path.
	 */
	bool operator==(const FAssetTreeFolderNode& Other) const
	{
		return FolderPath.Equals(Other.FolderPath);
	}

	/**
	 * @brief Compares folder nodes by normalized folder path.
	 * @param Other Node to compare against.
	 * @return true when nodes refer to different folder paths.
	 */
	bool operator!=(const FAssetTreeFolderNode& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * @brief Data backing the asset folder tree and its current selection.
 */
struct FFolderTreeData final
{
	TArray<TSharedPtr<FAssetTreeFolderNode>> TreeListItems;
	TArray<TSharedPtr<FAssetTreeFolderNode>> FilteredTreeListItems;
	TSharedPtr<FAssetTreeFolderNode> RootItem;
	FString SelectedDirectory;
	FString RootPath;
	FString PluginPath;
};

/**
 * @brief Slate widget references and transient UI state for the folder tree.
 */
struct FFolderTreeState final
{
	TSharedPtr<class STreeView<TSharedPtr<FAssetTreeFolderNode>>> TreeListView;
	TSharedPtr<class STreeView<TSharedPtr<FAssetTreeFolderNode>>> PluginTreeListView;
	TSharedPtr<class SSearchBox> SearchBox;
	FText TreeSearchText;
	TSet<FName> SelectedPaths;
	FName LastSortedColumn;
	EColumnSortMode::Type ColumnPathSortMode = EColumnSortMode::None;
};
