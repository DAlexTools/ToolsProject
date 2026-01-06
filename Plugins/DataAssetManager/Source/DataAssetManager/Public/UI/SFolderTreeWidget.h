// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"
#include "Widgets/SCompoundWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Input/SSearchBox.h"
#include "SFolderItemTree.h"
#include "DataAssetManagerTypes.h"


/**
 * @class SFolderTreeWidget
 * @brief A Slate widget representing a folder tree for assets, with search, filtering, and expansion capabilities.
 *
 * This widget allows users to navigate and interact with a hierarchical folder structure,
 * supports searching folders, filtering visible items, expanding/collapsing nodes, and selecting a folder.
 */
class DATAASSETMANAGER_API SFolderTreeWidget final : public SCompoundWidget
{
public:
	/**
	 * @brief Slate widget construction arguments.
	 */
	SLATE_BEGIN_ARGS(SFolderTreeWidget) {}
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the folder tree widget.
	 * @param InArgs The Slate arguments used for widget construction.
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * @brief Gets the currently selected folder path in the tree.
	 * @return The path of the selected directory as FString.
	 */
	FString GetSelectedDirectory() const;

private:
	/**
	 * @brief Handles search text committed by the user.
	 * @param InText The text entered by the user.
	 * @param CommitInfo Type of commit (e.g., Enter pressed, focus lost).
	 */
	void OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);

	/**
	 * @brief Handles changes to the search text as the user types.
	 * @param InText The current text in the search box.
	 */
	void OnSearchTextChanged(const FText& InText);

	/**
	 * @brief Updates the folder tree based on the current search filter.
	 */
	void UpdateFilteredTree();

	/**
	 * @brief Recursively expands all nodes in the tree starting from the given node.
	 * @param Node The root node to start expanding from.
	 */
	void ExpandAll(const TSharedPtr<FAssetTreeFolderNode>& Node);

	/**
	 * @brief Filters a single tree item according to the provided filter text.
	 * @param Item The tree item to filter.
	 * @param FilterText The text used to filter items.
	 * @return A pointer to the filtered item or nullptr if it does not match.
	 */
	TSharedPtr<FAssetTreeFolderNode> FilterTreeItem(const TSharedPtr<FAssetTreeFolderNode>& Item, const FString& FilterText);

	/**
	 * @brief Generates the header row for the folder tree.
	 * @return A shared reference to the header row widget.
	 */
	TSharedRef<SHeaderRow> GetTreeHeaderRow();

	/**
	 * @brief Generates a row widget for a single tree item.
	 * @param Item The item to generate a row for.
	 * @param OwnerTable The tree view that owns this row.
	 * @return A shared reference to the table row widget.
	 */
	TSharedRef<ITableRow> OnTreeGenerateRow(TSharedPtr<FAssetTreeFolderNode> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	 * @brief Retrieves the children of a given tree item.
	 * @param Item The parent item.
	 * @param OutChildren Array to populate with child items.
	 */
	void OnTreeGetChildren(TSharedPtr<FAssetTreeFolderNode> Item, TArray<TSharedPtr<FAssetTreeFolderNode>>& OutChildren);

	/**
	 * @brief Called when the tree selection changes.
	 * @param Item The newly selected item.
	 * @param SelectInfo The type of selection change.
	 */
	void OnTreeSelectionChanged(TSharedPtr<FAssetTreeFolderNode> Item, ESelectInfo::Type SelectInfo);

	/**
	 * @brief Called when a tree node is expanded or collapsed.
	 * @param Item The item that changed expansion state.
	 * @param bIsExpanded True if the item is now expanded.
	 */
	void OnTreeExpansionChanged(TSharedPtr<FAssetTreeFolderNode> Item, bool bIsExpanded);

	/**
	 * @brief Populates the tree with subfolders of the plugin.
	 * @param ParentItem The parent folder node to populate under.
	 */
	void PopulatePluginSubFolders(TSharedPtr<FAssetTreeFolderNode> ParentItem);

	/**
	 * @brief Refreshes the folder tree, updating items and their state.
	 */
	void UpdateFolderTree();

	/**
	 * @brief Fills the tree starting from the specified item, optionally using cached items to optimize.
	 * @param Item The root item to start filling from.
	 * @param CachedItems A set of cached items to reuse.
	 */
	void FillTreeFromPath(TSharedPtr<FAssetTreeFolderNode> Item, const TSet<TSharedPtr<FAssetTreeFolderNode>>& CachedItems);

	/**
	 * @brief Checks if a tree item or any of its children are expanded.
	 * @param Item The item to check.
	 * @param CachedItems Set of cached expanded items.
	 * @return True if the item or any child is expanded.
	 */
	bool TreeItemIsExpanded(const TSharedPtr<FAssetTreeFolderNode>& Item, const TSet<TSharedPtr<FAssetTreeFolderNode>>& CachedItems) const;

	/**
	 * @brief Checks if a tree item contains the current search text.
	 * @param Item The item to check.
	 * @return True if the item matches the search text.
	 */
	bool TreeItemContainsSearchText(const TSharedPtr<FAssetTreeFolderNode>& Item) const;

	/**
	 * @brief Sorts tree items, optionally updating the stored sorting order.
	 * @param UpdateSortingOrder Whether to update the saved sorting order.
	 */
	void SortTreeItems(const bool UpdateSortingOrder);

private:
	/**
	 * Structure containing the tree data (list of items, selected directory, etc.).
	 * Used for building and filtering the TreeView.
	 */
	FFolderTreeData FolderTreeData;

	/**
	 * Structure holding the widget state (expanded nodes, search text, Slate widget references, etc.).
	 * Used to control TreeView behavior and display.
	 */
	FFolderTreeState FolderTreeState;
};

