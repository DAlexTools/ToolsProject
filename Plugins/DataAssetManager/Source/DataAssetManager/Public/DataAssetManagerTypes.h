// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RevisionControlStyle/RevisionControlStyle.h"
#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"
#include "ClassViewerFilter.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "IDetailRootObjectCustomization.h"
#include "DetailLayoutBuilder.h"
#include "IDetailCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"

#define LOCTEXT_NAMESPACE "SDataAssetManagerWidget"


namespace DataAssetManager
{
	constexpr float TabReopenDelaySeconds = 1.0f;
	const FName ToolProjectEditor(TEXT("ToolProjectEditor"));
	const FName StatusBarName(TEXT("DataAssetManagerStatusBar"));
	const FName DataAssetManagerTabName(TEXT("DataAssetManager"));

	namespace ModuleName
	{
		constexpr const TCHAR* AssetTools = TEXT("AssetTools");
		constexpr const TCHAR* AssetRegistry = TEXT("AssetRegistry");
		constexpr const TCHAR* ContentBrowser = TEXT("ContentBrowser");
		constexpr const TCHAR* MessageLog = TEXT("MessageLog");
		constexpr const TCHAR* PropertyEditor = TEXT("PropertyEditor");
		constexpr const TCHAR* OutputLog = TEXT("OutputLog");
		constexpr const TCHAR* Settings = TEXT("Settings");
		constexpr const TCHAR* DataAssetManager = TEXT("DataAssetManager");
	}

	namespace Private
	{
		/**
		 * Class used to filter asset classes based on certain conditions like class flags and blueprint base class restrictions.
		 * Implements the IClassViewerFilter interface to provide custom filtering logic for class viewer in the Unreal Editor.
		 */
		class FAssetClassParentFilter : public IClassViewerFilter
		{
		public:

			FAssetClassParentFilter()
				: DisallowedClassFlags(CLASS_None), bDisallowBlueprintBase(false) {
			}

			/**
			 * Set of allowed parent classes.
			 * All children of these classes will be included unless filtered out by another setting.
			 * This filter excludes classes that are not in this set unless specified otherwise.
			 */
			TSet< const UClass* > AllowedChildrenOfClasses;

			/**
			 * Disallowed class flags.
			 * Used to filter out classes that have the specified flags.
			 */
			EClassFlags DisallowedClassFlags;

			/**
			 * Flag that specifies whether blueprint base classes should be excluded from the class viewer.
			 * If set to true, blueprint base classes will be filtered out.
			 */
			bool bDisallowBlueprintBase;

			/**
			 * Determines whether the given class is allowed based on the filter settings.
			 *
			 * @param InInitOptions Initialization options for the class viewer.
			 * @param InClass The class to be checked.
			 * @param InFilterFuncs Filter functions that provide additional filtering logic.
			 *
			 * @return true if the class is allowed, false otherwise.
			 */
			virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
			{
				bool bAllowed = !InClass->HasAnyClassFlags(DisallowedClassFlags)
					&& InClass->CanCreateAssetOfClass()
					&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;

				if (bAllowed && bDisallowBlueprintBase)
				{
					if (FKismetEditorUtilities::CanCreateBlueprintOfClass(InClass))
					{
						return false;
					}
				}

				return bAllowed;
			}

			/**
			 * Determines whether an unloaded class is allowed based on the filter settings.
			 *
			 * @param InInitOptions Initialization options for the class viewer.
			 * @param InUnloadedClassData Data about the unloaded class to be checked.
			 * @param InFilterFuncs Filter functions that provide additional filtering logic.
			 *
			 * @return true if the unloaded class is allowed, false otherwise.
			 */
			virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
			{
				if (bDisallowBlueprintBase)
				{
					return false;
				}

				return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
					&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
			}
		};

	}
}

namespace DataAssetListColumns
{
	/** IDs for list columns */
	static const FName ColumnID_RC("RevisionControl");
	static const FName ColumnID_Name("Name");
	static const FName ColumnID_Type("Type");
	static const FName ColumnID_DiskSize("DiskSize");
	static const FName ColumnID_Path("Path");
}

namespace DataAssetManagerMenu
{
	namespace IconStyle
	{
		static const FName AppStyle = FAppStyle::GetAppStyleSetName();
		static const FName RevisionControlStyle = FRevisionControlStyleManager::GetStyleSetName();
	}

	namespace Icons
	{
		// File Menu
		const FSlateIcon AddNewAsset = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.AssetActions.ReimportAsset");
		const FSlateIcon SaveAsset = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.SaveAllCurrentFolder");
		const FSlateIcon SaveAll = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.SaveAllCurrentFolder");
		const FSlateIcon Validate = FSlateIcon(IconStyle::AppStyle, "Icons.Adjust");
		const FSlateIcon Rename = FSlateIcon(IconStyle::AppStyle, "GenericCommands.Rename");
		const FSlateIcon Delete = FSlateIcon(IconStyle::AppStyle, "GenericCommands.Delete");
		// Assets Menu
		const FSlateIcon OpenAsset = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.ShowInExplorer");
		const FSlateIcon FindInCB = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.ShowInExplorer");
		const FSlateIcon Copy = FSlateIcon(IconStyle::AppStyle, "GenericCommands.Copy");
		const FSlateIcon ReferenceViewer = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.ReferenceViewer");
		const FSlateIcon SizeMap = FSlateIcon(IconStyle::AppStyle, "ContentBrowser.SizeMap");
		const FSlateIcon Audit = FSlateIcon(IconStyle::AppStyle, "Icons.Audit");
		const FSlateIcon RevisionControl = FSlateIcon(IconStyle::RevisionControlStyle, "RevisionControl.Actions.Diff");

		// Settings Menu
		const FSlateIcon MessageLog = FSlateIcon(IconStyle::AppStyle, "MessageLog.TabIcon");
		const FSlateIcon Visibility = FSlateIcon(IconStyle::AppStyle, "Icons.Visibility");
		const FSlateIcon Settings = FSlateIcon(IconStyle::AppStyle, "Icons.Settings");
		const FSlateIcon Refresh = FSlateIcon(IconStyle::AppStyle, "Icons.Refresh");
		const FSlateIcon OutputLog = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Log.TabIcon");

		// Help Menu
		const FSlateIcon Documentation = FSlateIcon(IconStyle::AppStyle, "GraphEditor.GoToDocumentation");
	}

	namespace ExtensionHookNames
	{
		const FName ExtensionHookCreateName = TEXT("Created");
		const FName ExtensionHookEditName = TEXT("Edit");
		const FName ExtensionHookValidateName = TEXT("Validate");
		const FName ExtensionHookDebugName = TEXT("Debug");
		const FName ExtensionHookSettingsName = TEXT("Settings");
		const FName ExtensionHookPluginSettingsName = TEXT("PluginSettings");
		const FName ExtensionHookRestartName = TEXT("Restart");
	}


	namespace Texts
	{
		// File Menu
		const FText CreateSectionText = LOCTEXT("CreateSection", "Create");
		const FText AddNewAssetText = LOCTEXT("AddNewAsset", "Add New Data Asset");
		const FText AddNewAssetTooltip = LOCTEXT("AddNewAssetTooltip", "Create new Data Asset in Content Browser");
		const FText EditSectionText = LOCTEXT("EditSection", "Edit");
		const FText RenameText = LOCTEXT("RenameAsset", "Rename");
		const FText RenameTooltip = LOCTEXT("RenameTooltip", "Rename selected asset");
		const FText DeleteText = LOCTEXT("DeleteAsset", "Delete");
		const FText DeleteTooltip = LOCTEXT("DeleteTooltip", "Delete selected asset");
		const FText SaveAssetText = LOCTEXT("SaveAsset", "Save");
		const FText SaveAssetTooltip = LOCTEXT("SaveAssetTooltip", "Save the selected Data Asset");
		const FText SaveAllText = LOCTEXT("SaveAll", "Save All");
		const FText SaveAllTooltip = LOCTEXT("SaveAllTooltip", "Save all modified Data Assets");
		// Assets Menu
		const FText OpenAssetText = LOCTEXT("OpenAsset", "Open Asset");
		const FText OpenAssetTooltip = LOCTEXT("OpenAssetTooltip", "Open the selected Data Asset in editor");
		const FText FindInCBText = LOCTEXT("FindInContentBrowser", "Find In CB");
		const FText FindInCBTooltip = LOCTEXT("FindInContentBrowserTooltip", "Locate asset in Content Browser");
		const FText CopyRefText = LOCTEXT("CopyReference", "Copy Reference");
		const FText CopyRefTooltip = LOCTEXT("CopyReferenceTooltip", "Copy asset reference to clipboard");
		const FText CopyPathsText = LOCTEXT("CopyPaths", "Copy Paths");
		const FText CopyPathsTooltip = LOCTEXT("CopyPathsTooltip", "Copy asset paths to clipboard");
		const FText RefViewerText = LOCTEXT("ReferenceViewer", "Reference Viewer");
		const FText RefViewerTooltip = LOCTEXT("ReferenceViewerTooltip", "Open reference viewer for this asset");
		const FText SizeMapText = LOCTEXT("SizeMap", "Size Map");
		const FText SizeMapTooltip = LOCTEXT("SizeMapTooltip", "View asset size information");
		const FText AuditAssetText = LOCTEXT("AuditAsset", "Audit Asset");
		const FText AuditAssetTooltip = LOCTEXT("AuditAssetTooltip", "Audit asset metadata");
		const FText RevisionControlText = LOCTEXT("RevisionControl", "Revision Control");
		const FText RevisionControlTooltip = LOCTEXT("RevisionControlTooltip", "Open revision control menu");
		const FText ShowAssetMetadataText = LOCTEXT("ShowAssetMetaData", "Show Asset Metadata");
		const FText ShowAssetMetadataTooltip = LOCTEXT("ShowAssetMetadataTooltip", "Display the metadata information of the selected asset.");

		// Settings Menu
		const FText DebugSectionText = LOCTEXT("DebugSection", "Debug");
		const FText OpenMessageLogText = LOCTEXT("OpenMessageLog_Label", "Open Message Log");
		const FText OpenMessageLogTooltip = LOCTEXT("OpenMessageLog_Tooltip", "Opens the Message Log window");
		const FText OpenOutputLogText = LOCTEXT("OpenOutputLog_Label", "Open Output Log");
		const FText OpenOutputLogTooltip = LOCTEXT("OpenOutputLog_Tooltip", "Opens the Output Log window");
		const FText SettingsSectionText = LOCTEXT("SettingsSection", "Settings");
		const FText ShowAssetsListText = LOCTEXT("ShowAssetsList", "Show Assets List");
		const FText ShowAssetsListTooltip = LOCTEXT("ShowAssetsListTooltip", "Toggle assets list visibility");
		const FText PluginSettingsSectionText = LOCTEXT("PluginSettingsSection", "Plugin");
		const FText PluginSettingsText = LOCTEXT("PluginSettings", "Plugin Settings");
		const FText PluginSettingsTooltip = LOCTEXT("PluginSettingsTooltip", "Open plugin settings");
		const FText RestartSectionText = LOCTEXT("RestartSection", "Maintenance");
		const FText RestartPluginText = LOCTEXT("RestartPlugin", "Restart Plugin");
		const FText RestartPluginTooltip = LOCTEXT("RestartPluginTooltip", "Restart the plugin");

		// Help Menu
		const FText DocumentationText = LOCTEXT("Documentation", "Documentation");
		const FText DocumentationTooltip = LOCTEXT("DocumentationTooltip", "Open documentation");

		// Menu Bar
		const FText FileMenuText = LOCTEXT("FileMenu", "File");
		const FText FileMenuTooltip = LOCTEXT("FileMenuTooltip", "File operations");
		const FText AssetMenuText = LOCTEXT("AssetMenu", "Asset");
		const FText AssetMenuTooltip = LOCTEXT("AssetMenuTooltip", "Asset operations");
		const FText SettingsMenuText = LOCTEXT("SettingsMenu", "Settings");
		const FText SettingsMenuTooltip = LOCTEXT("SettingsMenuTooltip", "Plugin settings");
		const FText HelpMenuText = LOCTEXT("HelpMenu", "Help");
		const FText HelpMenuTooltip = LOCTEXT("HelpMenuTooltip", "Help and documentation");
	}
}

/**
 * @struct FAssetTreeFolderNode
 * @brief Represents a folder node in an asset tree structure.
 *
 * This structure stores information about a folder within an asset hierarchy,
 * including its path, name, child nodes, and various state flags.
 */
struct FAssetTreeFolderNode final
{
	/** Full path to the folder in the asset tree. */
	FString FolderPath;

	/** The display name of the folder (without path). */
	FString FolderName;

	/** Pointer to the parent folder node. Null if this node is the root. */
	TSharedPtr<FAssetTreeFolderNode> Parent;

	/** List of child folder nodes contained within this folder. */
	TArray<TSharedPtr<FAssetTreeFolderNode>> SubItems;

	/** Indicates if the folder belongs to a development directory. */
	bool bIsDev : 1;

	/** True if this node is the root of the folder hierarchy. */
	bool bIsRoot : 1;

	/** True if the folder contains no assets or subfolders. */
	bool bIsEmpty : 1;

	/** True if the folder is excluded from asset operations or visibility. */
	bool bIsExcluded : 1;

	/** True if the folder is currently expanded in the UI tree view. */
	bool bIsExpanded : 1;

	/** True if the folder is visible in the asset tree. */
	bool bIsVisible : 1;

	/**
	 * @brief Constructs a new asset tree folder node.
	 *
	 * Initializes a folder node with optional path, name, and parent node.
	 * Default values create an empty, visible, non-expanded, non-root node.
	 *
	 * @param InFolderPath   Full folder path (default: empty string).
	 * @param InFolderName   Folder name (default: empty string).
	 * @param InParent       Pointer to the parent folder node (default: nullptr).
	 */
	explicit FAssetTreeFolderNode(
		const FString& InFolderPath = TEXT(""),
		const FString& InFolderName = TEXT(""),
		TSharedPtr<FAssetTreeFolderNode> InParent = nullptr)
		: FolderPath(InFolderPath)
		, FolderName(InFolderName)
		, Parent(InParent)
		, bIsDev(false)
		, bIsRoot(false)
		, bIsEmpty(true)
		, bIsExcluded(false)
		, bIsExpanded(false)
		, bIsVisible(true)
	{
	}

	/**
	 * @brief Equality operator.
	 * @param Other The other folder node to compare with.
	 * @return True if both nodes have the same folder path.
	 */
	bool operator==(const FAssetTreeFolderNode& Other) const
	{
		return FolderPath.Equals(Other.FolderPath);
	}

	/**
	 * @brief Inequality operator.
	 * @param Other The other folder node to compare with.
	 * @return True if the nodes represent different folder paths.
	 */
	bool operator!=(const FAssetTreeFolderNode& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * @brief Holds raw folder data and state-independent information for the folder tree.
 */
struct FFolderTreeData final 
{
	/**
	 * @brief Root-level folder nodes displayed in the tree view.
	 * These represent the main folder structure in the project.
	 */
	TArray<TSharedPtr<FAssetTreeFolderNode>> TreeListItems;

	/**
	 * @brief Filtered folder nodes used during search operations.
	 * Contains only nodes that match the active search query.
	 */
	TArray<TSharedPtr<FAssetTreeFolderNode>> FilteredTreeListItems;

	/**
	 * @brief The root folder node of the tree.
	 * Usually represents the `/Game` or plugin content root.
	 */
	TSharedPtr<FAssetTreeFolderNode> RootItem;

	/**
	 * @brief The currently selected directory path by the user.
	 */
	FString SelectedDirectory;

	/**
	 * @brief The base root path for project assets (commonly `/Game`).
	 */
	FString RootPath;

	/**
	 * @brief The base root path for plugin assets (commonly `/Plugins`).
	 */
	FString PluginPath;
};


/**
 * @brief Represents the current runtime state and UI-related widgets of the folder tree.
 */
struct FFolderTreeState final
{
	/**
	 * @brief Tree view widget displaying the main folder hierarchy.
	 */
	TSharedPtr<class STreeView<TSharedPtr<FAssetTreeFolderNode>>> TreeListView;

	/**
	 * @brief Tree view widget displaying plugin folders, if applicable.
	 */
	TSharedPtr<class STreeView<TSharedPtr<FAssetTreeFolderNode>>> PluginTreeListView;

	/**
	 * @brief Search box widget used for filtering folders in the tree view.
	 */
	TSharedPtr<class SSearchBox> SearchBox;

	/**
	 * @brief The current text input used for filtering folder names.
	 */
	FText TreeSearchText;

	/**
	 * @brief Set of currently selected folder paths in the tree view.
	 */
	TSet<FName> SelectedPaths;

	/**
	 * @brief The last column name used for sorting the folder tree.
	 */
	FName LastSortedColumn;

	/**
	 * @brief The current sorting mode of the folder tree column.
	 */
	EColumnSortMode::Type ColumnPathSortMode = EColumnSortMode::None;
};


/**
 * Structure that stores visibility flags for asset table columns.
 *
 * Provides control over which columns are currently displayed
 * in the asset management or data table view.
 */
struct FColumnVisibilityFlags
{
	/** Whether the asset type column is currently visible. */
	bool bShowTypeColumn = true;

	/** Whether the disk size column is currently visible. */
	bool bShowDiskSizeColumn = true;

	/** Whether the asset path column is currently visible. */
	bool bShowPathColumn = true;

	/** Whether the revision control column is currently visible. */
	bool bShowRevisionColumn = true;
};


/**
 * Structure that stores delegate handles used for asset registry event subscriptions.
 *
 * This structure provides a centralized way to manage all delegate registration
 * handles used by the manager to subscribe to AssetRegistry events such as:
 * - Asset addition
 * - Asset removal
 * - Asset renaming
 * - Files loaded notification
 *
 * Keeping these handles together ensures proper cleanup and safe unregistration
 * from AssetRegistry callbacks during shutdown or destruction.
 */
struct FManagerDelegateHandles final
{
	/**
	 * Registration handle for asset creation events.
	 *
	 * Used to safely unregister from AssetRegistry
	 * callbacks during shutdown.
	 */
	FDelegateHandle AssetAddedDelegateHandle{};

	/**
	 * Registration handle for asset deletion events.
	 *
	 * Tracks removal notifications from AssetRegistry.
	 */
	FDelegateHandle AssetRemovedDelegateHandle{};

	/**
	 * Registration handle for asset rename events.
	 *
	 * Maintains consistency when assets are
	 * renamed in content browser.
	 */
	FDelegateHandle AssetRenamedDelegateHandle{};

	/**
	 * Delegate handle for the OnFilesLoaded event subscription.
	 * Used to safely unsubscribe when the widget is destroyed.
	 * @see SubscribeToAssetRegistryEvent(), UnsubscribeFromAssetRegistryEvents()
	 */
	FDelegateHandle FilesLoadedHandle{};
};



/**
 * Structure that stores all UI widgets used in the asset manager panel.	
 *
 * Centralizes all Slate widget references for easier lifetime management and initialization.
 */
struct FAssetManagerWidgets final
{
	TSharedPtr<class SWidget> MenuBar = nullptr;
	TSharedPtr<class SSplitter> Splitter = nullptr;
	TSharedPtr<class IDetailsView> DetailsView = nullptr;
	TSharedPtr<class SComboButton> ComboButton = nullptr;
	TSharedPtr<class SEditableText> EditableTextWidget = nullptr;
	TSharedPtr<class SFilterSearchBox> ListViewSearchBox = nullptr;
	TSharedPtr<class SListView<TSharedPtr<FAssetData>>> AssetListView = nullptr;
};

/**
 * Structure that stores all asset data collections used by the asset manager.
 */
struct FAssetManagerData final
{
	/**
	 * Complete collection of discovered assets in the project.
	 *
	 * Populated during initial scan and updated via asset registry delegates.
	 * Contains raw FAssetData before any filtering is applied.
	 */
	TArray<TSharedPtr<FAssetData>> DataAssets;

	/**
	 * Subset of DataAssets that pass current filter criteria.
	 *
	 * Dynamically updated based on:
	 * - Search text matches
	 * - Asset type filters
	 * - Custom filter conditions
	 */
	TArray<TSharedPtr<FAssetData>> FilteredDataAssets;

	/**
	 * Assets queued for deferred deletion.
	 *
	 * Maintains references until deletion is confirmed
	 * or canceled via transaction.
	 */
	TArray<TSharedPtr<FAssetData>> DeletionDataAssets;

	/**
	 * Currently highlighted asset in UI.
	 *
	 * Synchronized between:
	 * - List view selection
	 * - Details view target
	 * - External selections
	 */
	TSharedPtr<FAssetData> SelectedAsset = nullptr;

	/**
	 * Holds the currently active filters in the data asset manager.
	 * Each filter is represented as a string.
	 */
	TSet<FString> ActiveFilters;

	/**
	 * Plugin directories filters
	 */
	TSet<FString> ActivePluginFilters;
};

/**
 * Structure that stores all data related to editable widgets and text inputs.
 */
struct FEditableWidgets final
{
	/**
	 * Mapping of editable text widgets used for asset property editing.
	 *
	 * Key:   TPair<PackagePath, AssetName> - Unique identifier combining asset location and name
	 * Value: TSharedPtr<SEditableText>     - Shared reference to the editable text widget
	 *
	 * Usage:
	 * - Provides O(1) access to text widgets by asset identifier
	 * - Maintains widget lifetime through shared pointers
	 * - Keys remain valid even if widget text changes
	 *
	 * @note Widgets are typically created during UI construction and removed when assets are unloaded.
	 * @see FAssetData, SEditableText
	 */
	TMap<TPair<FName, FName>, TSharedPtr<SEditableText>> EditableTextWidgets;

	/** Whether renaming is enabled and currently active. */
	bool bCanRename = true;
	bool bRenamedProgress = false;

	/**
	 * @brief Adds an editable text widget for the given asset.
	 * @param AssetData Pointer to the asset data providing PackagePath and AssetName.
	 * @param EditableText Shared pointer to the editable text widget to associate.
	 */
	FORCEINLINE void AddEditableTextWidget(const FAssetData* AssetData, const TSharedPtr<SEditableText>& EditableText)
	{
		if (!AssetData || !EditableText.IsValid())
		{
			return;
		}

		EditableTextWidgets.Add(
			{ 
				AssetData->PackagePath, 
				AssetData->AssetName },
			EditableText
		);
	}
};

/**
 * @brief Stores configuration and helper functions for managing asset table columns.
 *
 * Handles column visibility, order, and dynamic addition to a header row in the UI.
 */
struct FColumnData final
{

	/** Column visibility settings for the asset table. */
	FColumnVisibilityFlags ColumnVisibility;

	// Fixed-size allocator constants
	/** Maximum number of column adder functions. */
	static constexpr uint32 NumColumnAdders = 8;

	/**
	 * Mapping of column IDs to column adder functions.
	 *
	 * Key:   Column ID (FName)
	 * Value: Function that adds the column to a header row
	 *
	 * Uses a fixed-size allocator for performance and memory efficiency.
	 */
	TMap<FName, TFunction<void(TSharedPtr<SHeaderRow>)>, TFixedSetAllocator<NumColumnAdders>> ColumnAdders;

	/** Maximum number of ordered columns. */
	static constexpr uint32 NumColumnOrder = 6;

	/**
	 * Ordered list of column IDs defining display order.
	 *
	 * Used to maintain consistent column placement in the UI.
	 * Uses fixed-size memory for predictable layout.
	 */
	TArray<FName, TFixedAllocator<NumColumnOrder>> ColumnOrder;

	/**
	 * @brief Initializes the default order of columns.
	 */
	FORCEINLINE void InitializeColumnOrder()
	{
		ColumnOrder.Reset();
		ColumnOrder.Add(DataAssetListColumns::ColumnID_RC);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_Name);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_Type);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_DiskSize);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_Path);
	}

	/**
	 * @brief Initializes the column adder functions for the asset table.
	 *
	 * @tparam AddColumnFunc Callable type to add a standard column
	 * @tparam CreateRevisionFunc Callable type to add the revision control column
	 * @param AddColumnToHeader Function used to add a column to a header row
	 * @param CreateRevisionControlColumn Function used to create the revision control column
	 */
	template<typename AddColumnFunc, typename CreateRevisionFunc>
	void InitializeColumnAdders(AddColumnFunc&& AddColumnToHeader, CreateRevisionFunc&& CreateRevisionControlColumn)
	{
		ColumnAdders.Add(DataAssetListColumns::ColumnID_RC, [this, CreateRevisionControlColumn](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowRevisionColumn)
				{
					HeaderRow->AddColumn(CreateRevisionControlColumn());
				}
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_Name, [this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_Name, TEXT("Name"), 0.4f);
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_Type, [this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowTypeColumn)
				{
					AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_Type, TEXT("Type"), 0.3f);
				}
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_DiskSize, [this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowDiskSizeColumn)
				{
					AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_DiskSize, TEXT("DiskSize"), 0.15f);
				}
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_Path, [this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowPathColumn)
				{
					AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_Path, TEXT("Path"), 0.3f);
				}
			});
	}


	/**
	 * @brief Updates the visible columns in the provided header row based on ColumnOrder and visibility flags.
	 * @param HeaderRow The header row to update
	 */
	FORCEINLINE void UpdateColumnVisibility(TSharedPtr<SHeaderRow> HeaderRow)
	{
		if (!HeaderRow.IsValid()) return;
			
		HeaderRow->ClearColumns();

		for (const FName& ColumnId : ColumnOrder)
		{
			if (const TFunction<void(TSharedPtr<SHeaderRow>)>* AddFunc = ColumnAdders.Find(ColumnId))
			{
				(*AddFunc)(HeaderRow);
			}
		}
	}

	/**
	 * @brief Builds a new SHeaderRow and adds columns according to ColumnOrder and visibility flags.
	 * @return The constructed header row
	 */
	FORCEINLINE TSharedRef<SHeaderRow> BuildHeaderRow() const
	{
		TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow).Cursor(EMouseCursor::Hand);
		const TSharedPtr<SHeaderRow> HeaderRowPtr = HeaderRow;

		for (const FName& ColumnId : ColumnOrder)
		{
			if (const TFunction<void(TSharedPtr<SHeaderRow>)>* const AddFunc = ColumnAdders.Find(ColumnId))
			{
				(*AddFunc)(HeaderRowPtr);
			}
		}

		return HeaderRow;
	}

	/**
	 * @brief Toggles visibility for all columns.
	 * If any column is visible, all will be hidden. Otherwise, all will be shown.
	 */
	FORCEINLINE void ToggleAllColumnsVisibility()
	{
		const bool bShouldHide =
			ColumnVisibility.bShowDiskSizeColumn ||
			ColumnVisibility.bShowPathColumn ||
			ColumnVisibility.bShowTypeColumn ||
			ColumnVisibility.bShowRevisionColumn;

		const bool bNewVisibility = !bShouldHide;

		ColumnVisibility.bShowDiskSizeColumn = bNewVisibility;
		ColumnVisibility.bShowPathColumn = bNewVisibility;
		ColumnVisibility.bShowTypeColumn = bNewVisibility;
		ColumnVisibility.bShowRevisionColumn = bNewVisibility;
	}

	/**
	 * @brief Checks if all columns are currently hidden.
	 * @return true if all columns are hidden, false otherwise
	 */
	FORCEINLINE bool AreAllColumnsHidden() const
	{
		return !ColumnVisibility.bShowDiskSizeColumn &&
			!ColumnVisibility.bShowPathColumn &&
			!ColumnVisibility.bShowTypeColumn &&
			!ColumnVisibility.bShowRevisionColumn;
	}
};

DEFINE_LOG_CATEGORY_STATIC(SDataAssetManagerScopeLog, Log, All);

/**
 * @brief A simple scope-based timer for measuring execution time of a code block.
 *
 * This struct starts timing upon construction and logs the elapsed time when it is destroyed.
 * Useful for profiling sections of code in Unreal Engine projects.
 */
struct FScopeTimer
{
	/**
	 * @brief A descriptive name for the scope being measured.
	 */
	const TCHAR* Description;

	/**
	 * @brief The time (in seconds) when the timer was started.
	 */
	double StartTime;

	/**
	 * @brief Constructs a new FScopeTimer and starts measuring time.
	 *
	 * @param InDescription A human-readable description of the scope being timed.
	 */
	explicit FScopeTimer(const TCHAR* InDescription)
		: Description(InDescription)
	{
		StartTime = FPlatformTime::Seconds();
	}

	/**
	 * @brief Destructor that stops the timer and logs the elapsed time.
	 *
	 * Logs the execution duration using UE_LOG with the given Description.
	 */
	~FScopeTimer()
	{
		double EndTime = FPlatformTime::Seconds();
		UE_LOG(SDataAssetManagerScopeLog, Warning, TEXT("%s executed in %.6f seconds."), Description, EndTime - StartTime);
	}
};

/**
 * @brief Macro to create a scope timer with a given description.
 *
 * This macro instantiates an FScopeTimer with a unique name based on the current line number.
 * When the scope ends, the timer will automatically log the elapsed time.
 *
 * @param Description A string describing the scope being measured.
 */
#define MEASURE_SCOPE(Description) FScopeTimer Timer_##__LINE__(TEXT(Description))

#undef LOCTEXT_NAMESPACE