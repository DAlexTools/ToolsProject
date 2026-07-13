// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Menu/IDataAssetManagerInterface.h"
#include "DataAssetManagerTypes.h"
#include "Types/DataAssetDiffTypes.h"
#include "Types/DataAssetReferenceTypes.h"

class UDataAssetManagerSettings;
class UPackage;
class SLayeredImage;
class SFilterSearchBox;

/**
 * @brief Main editor widget for browsing, filtering, editing, and managing Data Asset instances.
 */
class DATAASSETMANAGER_API SDataAssetManagerWidget : public SCompoundWidget, public IDataAssetManagerInterface
{
	/** @brief Slate argument type used to construct the widget. */
	SLATE_BEGIN_ARGS(SDataAssetManagerWidget) {}
	SLATE_END_ARGS()

public:
	/**
	 * @brief Constructs the widget and builds its Slate layout.
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * @brief Releases widget resources and unregisters editor delegates.
	 */
	~SDataAssetManagerWidget();

	/**
	 * @brief Selects the first available asset when the asset list is not empty.
	 */
	void SelectFirstAssetIfAvailable();

protected:
	/** @brief Starts creation of a new Data Asset in the currently selected folder. */
	virtual void CreateNewDataAsset() override;

	/** @brief Opens the selected Data Asset in the editor. */
	virtual void OpenSelectedDataAssetInEditor() override;

	/** @brief Toggles visibility of the Data Asset list panel. */
	virtual void ToggleDataAssetListVisibility() override;

	/** @brief Opens the configured documentation URL. */
	virtual void ShowDocumentation() override;

	/** @brief Saves the selected Data Asset packages. */
	virtual void SaveDataAsset() override;

	/** @brief Saves all dirty Data Asset packages. */
	virtual void SaveAllData() override;

	/** @brief Syncs the Content Browser selection to the selected Data Assets. */
	virtual void SyncContentBrowserToSelectedAsset() override;

	/**
	 * @brief Copies selected asset references or file paths to the clipboard.
	 * @param bCopyPaths true to copy disk paths, false to copy object references.
	 */
	virtual void CopyToClipboard(bool bCopyPaths) override;

	/** @brief Opens Reference Viewer for the selected Data Assets. */
	virtual void OpenReferenceViewer() override;

	/** @brief Opens the reference inspector for the selected Data Asset. */
	virtual void OpenReferenceInspector() override;

	/** @brief Opens Data Asset Diff for two selected Data Assets. */
	virtual void OpenDataAssetDiff() override;

	/**
	 * @brief Checks whether exactly two Data Assets are selected for diff.
	 * @return true when the selection can open the diff command.
	 */
	virtual bool CanOpenDataAssetDiff() const override;

	/** @brief Opens Size Map for the selected Data Assets. */
	virtual void OpenSizeMap() override;

	/** @brief Opens Asset Audit for the selected Data Assets. */
	virtual void OpenAuditAsset() override;

	/** @brief Opens Data Asset Manager settings in the editor settings UI. */
	virtual void OpenPluginSettings() override;

	/** @brief Opens the source control dialog for the selected Data Assets. */
	virtual void ShowSourceControlDialog() override;

	/** @brief Restarts the plugin widget through the module restart workflow. */
	virtual void RestartPlugin() override;

	/** @brief Opens the Message Log window. */
	virtual void OpenMessageLogWindow() override;

	/** @brief Opens the Output Log window. */
	virtual void OpenOutputLogWindow() override;

	/**
	 * @brief Checks whether inline rename is currently allowed.
	 * @return true when the current selection can be renamed.
	 */
	virtual bool CanRename() const override;

	/** @brief Focuses the editable text widget for the selected asset. */
	virtual void FocusOnSelectedAsset() override;

	/** @brief Deletes the selected Data Assets. */
	virtual void DeleteDataAsset() override;

	/** @brief Shows metadata for the selected Data Asset. */
	virtual void ShowAssetMetaData() override;

protected:
	/** @brief Opens the current selection in the Property Matrix editor. */
	void OpenSelectionInPropertyMatrix();

	/**
	 * @brief Builds one reference inspector list section.
	 * @param Title Section title.
	 * @param EmptyText Text shown when the section has no entries.
	 * @param InspectionResult Shared inspection result that owns the list items.
	 * @param bReferencedBy true for the referencer list, false for the dependency list.
	 * @return Slate widget for the section.
	 */
	TSharedRef<SWidget> BuildReferenceListSection(
		FText Title,
		FText EmptyText,
		TSharedRef<FDataAssetReferenceInspectionResult> InspectionResult,
		bool bReferencedBy);

	/**
	 * @brief Generates a reference inspector row.
	 * @param Entry Reference entry represented by the row.
	 * @param OwnerTable Owning table view.
	 * @return New row widget.
	 */
	TSharedRef<ITableRow> GenerateReferenceEntryRow(TSharedPtr<FDataAssetReferenceEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable);

	/** @brief Syncs a reference inspector entry to the Content Browser. */
	void SyncReferenceEntryInContentBrowser(TSharedPtr<FDataAssetReferenceEntry> Entry) const;

	/** @brief Opens a reference inspector entry in its editor when possible. */
	void OpenReferenceEntryInEditor(TSharedPtr<FDataAssetReferenceEntry> Entry) const;

	/**
	 * @brief Generates a Data Asset diff row.
	 * @param Entry Diff entry represented by the row.
	 * @param OwnerTable Owning table view.
	 * @param DiffResult Shared diff result that owns assets and row entries.
	 * @param RefreshDiff Callback that recalculates and refreshes the diff list.
	 * @return New row widget.
	 */
	TSharedRef<ITableRow> GenerateDataAssetDiffRow(
		TSharedPtr<FDataAssetDiffEntry> Entry,
		const TSharedRef<STableViewBase>& OwnerTable,
		TSharedRef<FDataAssetDiffResult> DiffResult,
		TSharedRef<TFunction<void()>> RefreshDiff);

	/**
	 * @brief Copies one diff value from one side to the other and refreshes the window.
	 * @param Entry Diff entry whose property should be copied.
	 * @param DiffResult Shared diff result containing left and right assets.
	 * @param bLeftToRight true copies left value into right asset, false copies right value into left asset.
	 * @param RefreshDiff Callback that recalculates the diff after copy.
	 */
	void CopyDataAssetDiffValue(
		TSharedPtr<FDataAssetDiffEntry> Entry,
		TSharedRef<FDataAssetDiffResult> DiffResult,
		bool bLeftToRight,
		TSharedRef<TFunction<void()>> RefreshDiff);

	/**
	 * @brief Checks whether the selected assets can be opened in Property Matrix.
	 * @return true when at least one selected asset can be edited in Property Matrix.
	 */
	bool CanOpenSelectedAssetsInPropertyEditor() const;

	/** @brief Duplicates all selected Data Assets. */
	void DuplicateSelectedDataAssets();

	/** @brief Prompts for a destination and moves selected Data Assets. */
	void MoveSelectedDataAssets();

	/**
	 * @brief Moves selected Data Assets to the specified package path.
	 * @param DestinationPath Destination long package path.
	 */
	void MoveSelectedDataAssetsToPath(const FString& DestinationPath);

	/** @brief Runs validation for selected Data Assets. */
	void ValidateSelectedDataAssets();

	/** @brief Runs validation for all loaded Data Assets. */
	void ValidateAllDataAssets();

	/**
	 * @brief Applies validation states and updates the invalid-assets filter.
	 * @param ValidationResults Validation states produced by the asset service.
	 */
	void ApplyValidationResults(FDataAssetValidationResults&& ValidationResults);

	/**
	 * @brief Returns cached validation state for a table row asset.
	 * @param AssetData Asset data represented by the row.
	 * @return Validation state, or nullptr when the asset has not been validated yet.
	 */
	const FDataAssetValidationState* GetValidationStateForAsset(TSharedPtr<FAssetData> AssetData) const;

	/**
	 * @brief Checks whether the asset list has an active selection.
	 * @return true when one or more assets are selected.
	 */
	bool HasSelectedAssets() const;

	/**
	 * @brief Registers an editable text widget for inline asset rename.
	 * @param AssetData Asset data associated with the editable text widget.
	 * @param EditableText Editable text widget used for rename focus.
	 */
	void RegisterEditableText(TSharedPtr<FAssetData> AssetData, TSharedRef<SEditableText> EditableText);

	/**
	 * @brief Handles committed inline asset name edits.
	 * @param AssetData Asset data being renamed.
	 * @param InText New asset name text.
	 * @param CommitMethod Commit action reported by Slate.
	 */
	void HandleAssetRename(TSharedPtr<FAssetData> AssetData, const FText& InText, ETextCommit::Type CommitMethod);

	/**
	 * @brief Handles double-clicks on an asset list row.
	 * @param InGeometry Row geometry at the time of the event.
	 * @param MouseEvent Mouse event that triggered the action.
	 */
	void HandleAssetDoubleClick(const FGeometry& InGeometry, const FPointerEvent& MouseEvent);

	/**
	 * @brief Opens the context menu for a Data Asset row.
	 * @param InGeometry Row geometry used for menu placement.
	 * @param MouseEvent Mouse event that requested the menu.
	 */
	void CreateContextMenuFromDataAsset(const FGeometry& InGeometry, const FPointerEvent& MouseEvent);

	/**
	 * @brief Handles mouse button presses on an asset list row.
	 * @param InGeometry Row geometry at the time of the event.
	 * @param MouseEvent Mouse event to process.
	 * @return Reply describing how the event was handled.
	 */
	FReply HandleRowMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& MouseEvent);

	/** @brief Subscribes the widget to Asset Registry change events. */
	void SubscribeToAssetRegistryEvent();

	/** @brief Initializes font data used by text widgets. */
	void InitializeTextFontInfo();

	/** @brief Creates the details view used to inspect selected assets. */
	void CreateDetailsView();

	/**
	 * @brief Creates configuration arguments for the details view.
	 * @return Details view argument structure.
	 */
	FDetailsViewArgs CreateDetailsViewArgs() const;

	/**
	 * @brief Loads Data Assets using plugin scan settings.
	 * @param PluginSettings Plugin settings that define scan paths and exclusions.
	 */
	void LoadDataAssets(const UDataAssetManagerSettings* PluginSettings);

	/** @brief Rebuilds the filtered asset list from the current filters and search text. */
	void UpdateFilteredAssetList();

	/**
	 * @brief Handles asset selection changes in the list view.
	 * @param SelectedItem Newly selected asset data.
	 * @param SelectInfo Selection cause reported by Slate.
	 */
	void OnAssetSelected(TSharedPtr<FAssetData> SelectedItem, ESelectInfo::Type SelectInfo);

	/**
	 * @brief Opens the details view for the selected asset.
	 * @param SelectedItem Asset data to display.
	 */
	void OpenDetailViewPanelForAsset(TSharedPtr<FAssetData> SelectedItem);

	/**
	 * @brief Handles changes in the asset search box.
	 * @param InText Current search text.
	 */
	void OnSearchTextChanged(const FText& InText);

	/**
	 * @brief Generates a table row widget for an asset list item.
	 * @param Item Asset data displayed by the row.
	 * @param OwnerSTable Table view that owns the row.
	 * @return New table row widget.
	 */
	TSharedRef<ITableRow> GenerateAssetListRow(TSharedPtr<FAssetData> Item, const TSharedRef<STableViewBase>& OwnerSTable);

	/**
	 * @brief Initializes the asset type combo box from loaded assets.
	 * @param AssetDataList Asset data used to populate type entries.
	 */
	void InitializeAssetTypeComboBox(TArray<TSharedPtr<FAssetData>> AssetDataList);

	/**
	 * @brief Saves all dirty Data Asset packages.
	 * @return true when every requested save succeeds.
	 */
	bool SaveAllDataAsset();

	/**
	 * @brief Resolves asset identifiers and invokes a processing callback.
	 * @param RefAssetData Assets to resolve into identifiers.
	 * @param ProcessFunction Callback invoked with resolved identifiers.
	 */
	void ProcessAssetData(const TArray<FAssetData>& RefAssetData, TFunction<void(const TArray<FAssetIdentifier>&)> ProcessFunction);

	/** @brief Refreshes loaded assets and updates the visible list. */
	void RefreshAssetList();

	/**
	 * @brief Handles Asset Registry notifications for newly added assets.
	 * @param NewAssetData Asset data for the added asset.
	 */
	void OnAssetAdded(const FAssetData& NewAssetData);

	/**
	 * @brief Handles Asset Registry notifications for removed assets.
	 * @param AssetToRemoved Asset data for the removed asset.
	 */
	void OnAssetRemoved(const FAssetData& AssetToRemoved);

	/**
	 * @brief Handles Asset Registry notifications for renamed assets.
	 * @param NewAssetData Asset data after rename.
	 * @param Name Previous object path or name reported by the registry.
	 */
	void OnAssetRenamed(const FAssetData& NewAssetData, const FString& Name);

	/**
	 * @brief Handles dirty state changes for asset packages.
	 * @param DirtyPackage Package whose dirty state changed.
	 */
	void OnPackageDirtyStateChanged(UPackage* DirtyPackage);

	/**
	 * @brief Creates the icon widget used by the filter controls.
	 * @return Layered image widget for filter state.
	 */
	TSharedPtr<SLayeredImage> CreateFilterImage();

	/**
	 * @brief Returns the current selection mode for the asset list.
	 * @return Selection mode used by the list view.
	 */
	ESelectionMode::Type GetAssetListSelectionMode() const;

	/**
	 * @brief Returns search box visibility based on widget state.
	 * @return Visibility value for the search box.
	 */
	EVisibility GetVisibilitySearchBox() const;

	/**
	 * @brief Handles selecting an item in the asset type combo box.
	 * @param SourceItem Selected combo-box entry.
	 * @return Reply produced by the selection action.
	 */
	FReply OnItemClicked(TSharedPtr<FString> SourceItem);

	/**
	 * @brief Creates popup content for the asset type combo button.
	 * @return Combo-button content widget.
	 */
	TSharedRef<SWidget> CreateComboButtonContent();

	/**
	 * @brief Adds a toggleable filter entry to a menu.
	 * @param MenuBuilder Menu builder that receives the entry.
	 * @param FilterName Filter key shown by the entry.
	 * @param ActiveFilters Active filter set updated by the entry.
	 * @param UpdateFunc Callback invoked after filter state changes.
	 */
	void AddToggleFilterMenuEntry(FMenuBuilder& MenuBuilder, const FString& FilterName, TSet<FString>& ActiveFilters, TFunction<void()> UpdateFunc);

	/**
	 * @brief Adds an asset list column to the provided header row.
	 * @param InHeaderRow Header row that receives the column.
	 * @param ColumnId Unique column identifier.
	 * @param Label Display label for the column.
	 * @param FillWidth Relative fill width used by the column.
	 */
	void AddColumnToHeader(TSharedPtr<SHeaderRow> InHeaderRow, const FName& ColumnId, const FString& Label, const float FillWidth);

	/**
	 * @brief Creates arguments for the revision-control status column.
	 * @return Column arguments for the revision-control column.
	 */
	SHeaderRow::FColumn::FArguments CreateRevisionControlColumn();

	/** @brief Updates the text and state shown by the combo button. */
	void UpdateComboButtonContent();

	/**
	 * @brief Returns selected asset list items.
	 * @return Array of selected asset data pointers.
	 */
	TArray<TSharedPtr<FAssetData>> GetAssetListSelectedItem() const;

	/**
	 * @brief Checks whether the current selected asset is valid for an action.
	 * @param CustomMessage Optional message included in failure feedback.
	 * @return true when the current selection is valid.
	 */
	bool IsSelectedAssetValid(const FString& CustomMessage = "") const;

	/**
	 * @brief Selects and focuses a newly added asset in the list.
	 * @param NewAssetData Asset data for the newly added asset.
	 */
	void FocusOnNewlyAddedAsset(const FAssetData& NewAssetData);

	/**
	 * @brief Returns the icon badge used for revision-control column state.
	 * @return Slate brush for the current revision-control badge.
	 */
	const FSlateBrush* GetRevisionControlColumnIconBadge() const;

	/**
	 * @brief Creates the asset list header row.
	 * @return Header row with all currently registered columns.
	 */
	TSharedRef<SHeaderRow> GenerateHeaderRow();

	/** @brief Applies column visibility settings to the asset list header. */
	void UpdateColumnVisibility();

	/**
	 * @brief Handles clicks on the column-visibility header button.
	 * @param InGeometry Button geometry at the time of the event.
	 * @param MouseEvent Mouse event that triggered the button.
	 * @return Reply describing how the click was handled.
	 */
	FReply ColumnButtonClicked(const FGeometry& InGeometry, const FPointerEvent& MouseEvent);

	/** @brief Initializes functions that add asset list columns. */
	void InitializeColumnAdders();

	/**
	 * @brief Toggles a column visibility flag.
	 * @param bColumnFlag Pointer to the visibility flag to toggle.
	 */
	void ToggleColumn(bool* bColumnFlag);

	/**
	 * @brief Adds a column visibility entry to a menu.
	 * @param MenuBuilder Menu builder that receives the entry.
	 * @param Label Entry label displayed in the menu.
	 * @param Tooltip Tooltip text displayed for the entry.
	 * @param ColumnFlag Visibility flag controlled by the entry.
	 */
	void AddColumnMenuEntry(FMenuBuilder& MenuBuilder, FText Label, FText Tooltip, bool* ColumnFlag);

	/**
	 * @brief Checks whether a column visibility flag is enabled.
	 * @param bColumnPtr Pointer to the visibility flag.
	 * @return true when the flag exists and is enabled.
	 */
	bool IsColumnVisible(bool* bColumnPtr) const;

	/**
	 * @brief Builds status text describing the current selection.
	 * @return Text shown in the manager status area.
	 */
	FText GetSelectedTextBlockInfo() const;

	/**
	 * @brief Checks whether the details view has no selected objects.
	 * @return true when the details view is empty or unavailable.
	 */
	bool IsDetailsViewEmpty() const;

private:
	/** @brief Asset lists, selection, and active filter state. */
	FAssetManagerData AssetManagerData;

	/** @brief Slate widgets owned by this manager widget. */
	FAssetManagerWidgets AssetManagerWidgets;

	/** @brief Search text attribute bound to the asset list filter. */
	TAttribute<FText> SearchText = TAttribute<FText>();

	/** @brief Font information used by text controls. */
	FSlateFontInfo TextFontInfo = {};

	/** @brief Delegate handles registered with editor systems. */
	FManagerDelegateHandles ManagerDelegateHandles;

	/** @brief Inline rename widget registry and rename state. */
	FEditableWidgets EditableWidgets;

	/** @brief Combo-box entries for asset type filters. */
	TArray<TSharedPtr<FString>> ComboBoxAssetListItems = {};

	/** @brief Combo-box entries for plugin path filters. */
	TArray<TSharedPtr<FString>> PluginFilterListItems = {};

	/** @brief Currently selected asset type filter. */
	TSharedPtr<FString> SelectedAssetType = nullptr;

	/** @brief Visibility flag for the optional asset list slot. */
	bool bIsSlotVisible = true;

	/** @brief Splitter size value for the asset list and details panels. */
	TAttribute<float> SplitterValue = 0.4f;

	/** @brief Column registration, order, and visibility state. */
	FColumnData ColumnData;
#pragma endregion Data
};
