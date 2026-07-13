// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Input/SSearchBox.h"
#include "SFolderItemTree.h"
#include "DataAssetManagerTypes.h"

/**
 * @brief Slate widget that displays selectable project and plugin folder trees.
 */
class DATAASSETMANAGER_API SFolderTreeWidget final : public SCompoundWidget
{
public:
	/** @brief Slate arguments for constructing the folder tree widget. */
	SLATE_BEGIN_ARGS(SFolderTreeWidget) {}
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the folder tree widget.
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * @brief Returns the currently selected directory.
	 * @return Selected long package path or project folder path.
	 */
	const FString& GetSelectedDirectory() const;

private:
	/**
	 * @brief Handles committed search text.
	 * @param InText Text committed by the search box.
	 * @param CommitInfo Commit type reported by Slate.
	 */
	void OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);

	/**
	 * @brief Handles live search text changes.
	 * @param InText Current search text.
	 */
	void OnSearchTextChanged(const FText& InText);

	/**
	 * @brief Refreshes the visible tree according to the current search filter.
	 */
	void UpdateFilteredTree();

	/**
	 * @brief Recursively expands a folder tree node and its children.
	 * @param Node Root node to expand.
	 */
	void ExpandAll(const TSharedPtr<FAssetTreeFolderNode>& Node);

	/**
	 * @brief Builds a filtered copy of a folder tree node.
	 * @param Item Node to filter.
	 * @param FilterText Search text used for matching.
	 * @return Filtered node copy, or nullptr when the node and its children do not match.
	 */
	TSharedPtr<FAssetTreeFolderNode> FilterTreeItem(const TSharedPtr<FAssetTreeFolderNode>& Item, const FString& FilterText);

	/**
	 * @brief Creates the folder tree header row.
	 * @return Header row widget.
	 */
	TSharedRef<SHeaderRow> GetTreeHeaderRow();

	/**
	 * @brief Generates a row widget for a folder tree item.
	 * @param Item Folder node to display.
	 * @param OwnerTable Table view that owns the row.
	 * @return Row widget for the folder node.
	 */
	TSharedRef<ITableRow> OnTreeGenerateRow(TSharedPtr<FAssetTreeFolderNode> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	 * @brief Supplies child nodes for a tree item.
	 * @param Item Parent node.
	 * @param OutChildren Receives child nodes.
	 */
	void OnTreeGetChildren(TSharedPtr<FAssetTreeFolderNode> Item, TArray<TSharedPtr<FAssetTreeFolderNode>>& OutChildren);

	/**
	 * @brief Handles selection changes in the folder tree.
	 * @param Item Newly selected node.
	 * @param SelectInfo Selection cause.
	 */
	void OnTreeSelectionChanged(TSharedPtr<FAssetTreeFolderNode> Item, ESelectInfo::Type SelectInfo);

	/**
	 * @brief Handles expansion state changes for a tree node.
	 * @param Item Node whose expansion state changed.
	 * @param bIsExpanded true when the node is expanded.
	 */
	void OnTreeExpansionChanged(TSharedPtr<FAssetTreeFolderNode> Item, bool bIsExpanded);

	/**
	 * @brief Adds project plugin folders under the provided parent item.
	 * @param ParentItem Parent folder node.
	 */
	void PopulatePluginSubFolders(const TSharedPtr<FAssetTreeFolderNode>& ParentItem);

	/**
	 * @brief Rebuilds folder tree items from project and plugin paths.
	 */
	void UpdateFolderTree();

	/**
	 * @brief Populates a folder node from filesystem subdirectories.
	 * @param Item Folder node to populate.
	 * @param CachedItems Cached nodes used to preserve expansion and visibility state.
	 */
	void FillTreeFromPath(const TSharedPtr<FAssetTreeFolderNode>& Item, const TSet<TSharedPtr<FAssetTreeFolderNode>>& CachedItems);

	/**
	 * @brief Checks whether a tree item should be restored as expanded.
	 * @param Item Item to test.
	 * @param CachedItems Cached expanded items.
	 * @return true when the item was previously expanded.
	 */
	bool TreeItemIsExpanded(const TSharedPtr<FAssetTreeFolderNode>& Item, const TSet<TSharedPtr<FAssetTreeFolderNode>>& CachedItems) const;

	/**
	 * @brief Checks whether a tree item matches the active search text.
	 * @param Item Item to test.
	 * @return true when the item matches the search text.
	 */
	bool TreeItemContainsSearchText(const TSharedPtr<FAssetTreeFolderNode>& Item) const;

	/**
	 * @brief Sorts tree items and optionally toggles the saved sort order.
	 * @param UpdateSortingOrder Whether to update the stored sort mode.
	 */
	void SortTreeItems(const bool UpdateSortingOrder);

private:
	/** @brief Folder tree item data and selection state. */
	FFolderTreeData FolderTreeItem{};

	/** @brief Slate widget references and transient folder tree state. */
	FFolderTreeState FolderTreeState{};
};
