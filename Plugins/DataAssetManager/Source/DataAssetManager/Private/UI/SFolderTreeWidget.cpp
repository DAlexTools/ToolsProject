// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SFolderTreeWidget.h"

void SFolderTreeWidget::Construct(const FArguments& InArgs)
{
	bCanSupportFocus = true;

	SAssignNew(FolderTreeState.TreeListView, STreeView<TSharedPtr<FAssetTreeFolderNode>>)
		.TreeItemsSource(&FolderTreeData.TreeListItems)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SFolderTreeWidget::OnTreeGenerateRow)
		.OnGetChildren(this, &SFolderTreeWidget::OnTreeGetChildren)
		.OnSelectionChanged(this, &SFolderTreeWidget::OnTreeSelectionChanged)
		.OnExpansionChanged(this, &SFolderTreeWidget::OnTreeExpansionChanged)
		.HeaderRow(GetTreeHeaderRow());

	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox)
		.ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)
		.AnimateWheelScrolling(true)
		.AllowOverscroll(EAllowOverscroll::No)
		+ SScrollBox::Slot()
		[
			FolderTreeState.TreeListView.ToSharedRef()
		];


	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);
	VerticalBox->AddSlot().AutoHeight()
		[
			SAssignNew(FolderTreeState.SearchBox, SSearchBox)
				.HintText(FText::FromString("Search folders..."))
				.OnTextChanged(this, &SFolderTreeWidget::OnSearchTextChanged)
				.OnTextCommitted(this, &SFolderTreeWidget::OnSearchTextCommitted)
		];

	VerticalBox->AddSlot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				FolderTreeState.TreeListView.ToSharedRef()
			]
		];

	ChildSlot
		[
			VerticalBox
		];

	UpdateFolderTree();
}

FString SFolderTreeWidget::GetSelectedDirectory() const
{
	return FolderTreeData.SelectedDirectory;
}

void SFolderTreeWidget::OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
{
	FolderTreeState.TreeSearchText = InText;
	UpdateFilteredTree();
}

void SFolderTreeWidget::OnSearchTextChanged(const FText& InText)
{
	FolderTreeState.TreeSearchText = InText;
	UpdateFilteredTree();
}

void SFolderTreeWidget::UpdateFilteredTree()
{
	if (FolderTreeState.TreeSearchText.IsEmpty())
	{
		FolderTreeState.TreeListView->SetTreeItemsSource(&FolderTreeData.TreeListItems);
		FolderTreeState.TreeListView->RebuildList();
		return;
	}

	FolderTreeData.FilteredTreeListItems.Reset();
	const FString FilterText = FolderTreeState.TreeSearchText.ToString();
	for (const auto& Item : FolderTreeData.TreeListItems)
	{
		TSharedPtr<FAssetTreeFolderNode> FilteredRoot = FilterTreeItem(Item, FolderTreeState.TreeSearchText.ToString());
		if (FilteredRoot.IsValid())
		{
			FolderTreeData.FilteredTreeListItems.Add(FilteredRoot);
		}
	}

	FolderTreeState.TreeListView->SetTreeItemsSource(&FolderTreeData.FilteredTreeListItems);
	FolderTreeState.TreeListView->RebuildList();

	for (const auto& RootItem : FolderTreeData.FilteredTreeListItems)
	{
		ExpandAll(RootItem);
	}
}

void SFolderTreeWidget::ExpandAll(const TSharedPtr<FAssetTreeFolderNode>& Node)
{
	if (!Node.IsValid() || !FolderTreeState.TreeListView.IsValid())
	{
		return;
	}

	FolderTreeState.TreeListView->SetItemExpansion(Node, true);
	for (const auto& Child : Node->SubItems)
	{
		ExpandAll(Child);
	}
}

TSharedPtr<FAssetTreeFolderNode> SFolderTreeWidget::FilterTreeItem(const TSharedPtr<FAssetTreeFolderNode>& Item, const FString& FilterText)
{
	if (!Item.IsValid()) return nullptr;

	bool bMatches = Item->FolderName.Contains(FilterText);
	TSharedPtr<FAssetTreeFolderNode> FilteredNode = MakeShareable(new FAssetTreeFolderNode(*Item));
	FilteredNode->SubItems.Reset();

	for (const auto& SubItem : Item->SubItems)
	{
		TSharedPtr<FAssetTreeFolderNode> FilteredChild = FilterTreeItem(SubItem, FilterText);
		if (FilteredChild.IsValid())
		{
			FilteredNode->SubItems.Add(FilteredChild);
			bMatches = true;
		}
	}

	return bMatches ? FilteredNode : nullptr;
}

TSharedRef<SHeaderRow> SFolderTreeWidget::GetTreeHeaderRow()
{
	return SNew(SHeaderRow)
		+ SHeaderRow::Column(TEXT("Path"))
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HeaderContentPadding(FMargin{ 5.0f })
		.FillWidth(0.4f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Path")))
		];
}

TSharedRef<ITableRow> SFolderTreeWidget::OnTreeGenerateRow(TSharedPtr<FAssetTreeFolderNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SFolderItemTree, OwnerTable).Item(Item).HighlightText(FolderTreeState.TreeSearchText);
}

void SFolderTreeWidget::OnTreeGetChildren(TSharedPtr<FAssetTreeFolderNode> Item, TArray<TSharedPtr<FAssetTreeFolderNode>>& OutChildren)
{
	if (!Item.IsValid())
	{
		return;
	}

	for (const auto& SubItem : Item->SubItems)
	{
		if (SubItem->bIsVisible)
		{
			OutChildren.Emplace(SubItem);
		}
	}
}

void SFolderTreeWidget::OnTreeSelectionChanged(TSharedPtr<FAssetTreeFolderNode> Item, ESelectInfo::Type SelectInfo)
{
	if (!FolderTreeState.TreeListView.IsValid())
	{
		return;
	}

	const auto& SelectedItems = FolderTreeState.TreeListView->GetSelectedItems();

	FolderTreeData.SelectedDirectory.Empty();
	FolderTreeData.SelectedDirectory.Reserve(SelectedItems.Num());

	for (const auto& SelectedItem : SelectedItems)
	{
		FolderTreeData.SelectedDirectory = SelectedItem->FolderPath;
	}
}

void SFolderTreeWidget::OnTreeExpansionChanged(TSharedPtr<FAssetTreeFolderNode> Item, bool bIsExpanded)
{
	if (!Item.IsValid() || !FolderTreeState.TreeListView.IsValid())
	{
		return;
	}

	Item->bIsExpanded = bIsExpanded;
	FolderTreeState.TreeListView->SetItemExpansion(Item, Item->bIsExpanded);
	FolderTreeState.TreeListView->RebuildList();
}

void SFolderTreeWidget::PopulatePluginSubFolders(TSharedPtr<FAssetTreeFolderNode> ParentItem)
{
	if (!ParentItem.IsValid())
	{
		return;
	}

	TArray<FString> SubPaths;
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetSubPaths(ParentItem->FolderPath, SubPaths, true);

	for (const auto& SubPath : SubPaths)
	{
		const FString FolderName = FPaths::GetCleanFilename(SubPath);
		const TSharedPtr<FAssetTreeFolderNode> SubItem = MakeShareable(new FAssetTreeFolderNode);
		SubItem->FolderPath = SubPath;
		SubItem->FolderName = FolderName;
		SubItem->bIsRoot = false;
		SubItem->bIsExpanded = false;
		SubItem->bIsVisible = true;
		SubItem->Parent = ParentItem;

		ParentItem->SubItems.Emplace(SubItem);
		//Recursive
		PopulatePluginSubFolders(SubItem);
	}
}

void SFolderTreeWidget::UpdateFolderTree()
{
	if (!FolderTreeState.TreeListView.IsValid()) 
	{
		return;
	}

	TSet<TSharedPtr<FAssetTreeFolderNode>> CachedExpandedItems;
	FolderTreeState.TreeListView->GetExpandedItems(CachedExpandedItems);

	FolderTreeData.TreeListItems.Reset();

	/* Content FolderTree */
	TSharedPtr<FAssetTreeFolderNode> RootContent = MakeShareable(new FAssetTreeFolderNode);
	RootContent->FolderPath = DataAssetManager::GetPathRootToString();
	RootContent->FolderName = TEXT("Content");
	RootContent->bIsRoot = true;
	RootContent->bIsDev = false;
	RootContent->bIsExpanded = false;
	RootContent->bIsVisible = true;
	RootContent->Parent = nullptr;

	FillTreeFromPath(RootContent, CachedExpandedItems);

	FolderTreeData.TreeListItems.Add(RootContent);

	/* Plugins FolderTree */
	TSharedPtr<FAssetTreeFolderNode> RootPlugins = MakeShareable(new FAssetTreeFolderNode);
	RootPlugins->FolderPath = TEXT("/Plugins");
	RootPlugins->FolderName = TEXT("Plugins");
	RootPlugins->bIsRoot = true;
	RootPlugins->bIsExpanded = false;
	RootPlugins->bIsVisible = true;
	RootPlugins->Parent = nullptr;

	const TArray<TSharedRef<IPlugin>> EnabledPlugins = IPluginManager::Get().GetEnabledPlugins();
	for (const auto& Plugin : EnabledPlugins)
	{
		if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project)
		{
			FString PluginContentAbsPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Content"));
			FPaths::NormalizeDirectoryName(PluginContentAbsPath);
			FString PluginGamePath;
			if (!FPackageName::TryConvertFilenameToLongPackageName(PluginContentAbsPath, PluginGamePath))
			{
				PluginGamePath = FString::Printf(TEXT("/Game/Plugins/%s"), *Plugin->GetName());
			}

			const TSharedPtr<FAssetTreeFolderNode> PluginItem = MakeShareable(new FAssetTreeFolderNode);
			PluginItem->FolderPath = PluginGamePath;
			PluginItem->FolderName = Plugin->GetName();
			PluginItem->bIsRoot = false;
			PluginItem->bIsExpanded = false;
			PluginItem->bIsVisible = true;
			PluginItem->Parent = RootPlugins;

			RootPlugins->SubItems.Add(PluginItem);

			PopulatePluginSubFolders(PluginItem);
		}
	}
	SortTreeItems(false);

	FolderTreeData.TreeListItems.Emplace(RootPlugins);
	FolderTreeState.TreeListView->RebuildList();
}

void SFolderTreeWidget::FillTreeFromPath(TSharedPtr<FAssetTreeFolderNode> Item, const TSet<TSharedPtr<FAssetTreeFolderNode>>& CachedItems)
{
	if (!Item.IsValid()) 
	{
		return;
	}

	TArray<FString> SubPaths;
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetSubPaths(Item->FolderPath, SubPaths, false);

	for (const auto& SubPath : SubPaths)
	{
		if (DataAssetManager::FolderIsExternal(SubPath)) continue;

		const TSharedPtr<FAssetTreeFolderNode> SubItem = MakeShareable(new FAssetTreeFolderNode);
		SubItem->FolderPath = SubPath;
		SubItem->FolderName = FPaths::GetPathLeaf(SubPath);
		SubItem->bIsDev = SubItem->FolderPath.StartsWith(DataAssetManager::GetPathDevToString());
		SubItem->bIsRoot = false;
		SubItem->bIsEmpty = DataAssetManager::FolderIsEmpty(SubPath);
		SubItem->Parent = Item;
		SubItem->bIsExpanded = TreeItemIsExpanded(SubItem, CachedItems);
		SubItem->bIsVisible = true;

		Item->SubItems.Add(SubItem);

		FillTreeFromPath(SubItem, CachedItems);
	}
}

bool SFolderTreeWidget::TreeItemIsExpanded(const TSharedPtr<FAssetTreeFolderNode>& Item, const TSet<TSharedPtr<FAssetTreeFolderNode>>& CachedItems) const
{
	if (!FolderTreeState.TreeSearchText.IsEmpty() && TreeItemContainsSearchText(Item))
	{
		auto CurrentItem = Item;
		while (CurrentItem.IsValid())
		{
			CurrentItem->bIsExpanded = true;
			FolderTreeState.TreeListView->SetItemExpansion(CurrentItem, CurrentItem->bIsExpanded);
			CurrentItem = CurrentItem->Parent;
		}

		return true;
	}

	for (const auto& ExpandedItem : CachedItems)
	{
		if (!ExpandedItem.IsValid())
		{
			continue;
		}

		if (ExpandedItem->FolderPath.Equals(Item->FolderPath))
		{
			return true;
		}
	}

	return false;
}

bool SFolderTreeWidget::TreeItemContainsSearchText(const TSharedPtr<FAssetTreeFolderNode>& Item) const
{
	TArray<FString> SubPaths;

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().GetSubPaths(Item->FolderPath, SubPaths, true);
	for (const auto& Path : SubPaths)
	{
		if (Path.Contains(FolderTreeState.TreeSearchText.ToString()))
		{
			return true;
		}
	}

	return false;
}

void SFolderTreeWidget::SortTreeItems(const bool UpdateSortingOrder)
{
	auto SortTreeItems = [&](auto& SortMode, auto SortFunc)
		{
			if (UpdateSortingOrder)
			{
				SortMode = SortMode == EColumnSortMode::Ascending ? EColumnSortMode::Descending : EColumnSortMode::Ascending;
			}

			TArray<TSharedPtr<FAssetTreeFolderNode>> Stack;
			Stack.Push(FolderTreeData.RootItem);

			while (Stack.Num() > 0)
			{
				const auto& CurrentItem = Stack.Pop(EAllowShrinking::No);
				if (!CurrentItem.IsValid()) continue;

				TArray<TSharedPtr<FAssetTreeFolderNode>>& SubItems = CurrentItem->SubItems;
				SubItems.Sort(SortFunc);

				Stack.Append(CurrentItem->SubItems);
			}
		};

	if (FolderTreeState.LastSortedColumn.IsEqual(TEXT("Path")))
	{
		SortTreeItems(FolderTreeState.ColumnPathSortMode, [&](const TSharedPtr<FAssetTreeFolderNode>& Item1, const TSharedPtr<FAssetTreeFolderNode>& Item2)
			{
				return FolderTreeState.ColumnPathSortMode == EColumnSortMode::Ascending ? Item1->FolderPath < Item2->FolderPath : Item1->FolderPath > Item2->FolderPath;
			});
	}
}

