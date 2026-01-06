// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SDataAssetManagerWidget.h"
#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"
#include "Widgets/SWidget.h" 
#include "Interfaces/IPluginManager.h"
#include "SourceControlHelpers.h"
#include "SPositiveActionButton.h"
#include "Algo/Transform.h"
#include "Algo/AnyOf.h"
#include "UI/SDataAssetTableRow.h"
#include "DataAssetManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Widgets/Images/SLayeredImage.h"
#include "Menu/DataAssetManagerMenu.h"
#include "UObject/MetaData.h"
#include "SMetaDataView.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/SavePackage.h"
#include "AssetManagerEditorModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "Editor/UnrealEd/Classes/Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "ISettingsModule.h"
#include "Kismet2/SClassPickerDialog.h"
#include "MessageLogModule.h"
#include "ObjectTools.h"
#include "OutputLogModule.h"
#include "ToolMenus.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "WidgetDrawerConfig.h"
#include "Editor/ContentBrowser/Private/ContentBrowserSingleton.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UI/SFolderTreeWidget.h"
#include "Customization/DetailsRootObjectCustomization.h"
/* clang-format off */

#define LOCTEXT_NAMESPACE "SDataAssetManagerWidget"

DEFINE_LOG_CATEGORY_STATIC(SDataAssetManagerWidgetLog, All, All);

static TAutoConsoleVariable<bool> CVarDebugDataAssetManager(TEXT("ShowDebugDataAssetManager"),
	false,
	TEXT("Show debug for da manager")
	TEXT("  0: off/n")
	TEXT("  1: on/n"),
	ECVF_Cheat);

namespace DataAssetManager
{
	constexpr float ItemHeigth = 24.0f;
	constexpr float DataAssetFontSize = 10.0f;
	constexpr float SearchBoxHideThreshold = 0.01f;
	constexpr float DefaultSplitterValueWhenVisible = 0.25f;
	constexpr float SplitterValueWhenHidden = 0.0f;
	constexpr float ExpireDuration = 3.0f;
	constexpr float MetaDataWindowWidth = 500.0f;
	constexpr float MetaDataWindowHeight = 250.0f;
	constexpr float RCFixedWidth = 30.0f;

	const FMargin SeparatorPadding = FMargin(5.f, 7.f);
}

void SDataAssetManagerWidget::Construct(const FArguments& InArgs)
{
	bCanSupportFocus = true;

	SubscribeToAssetRegistryEvent();
	LoadDataAssets(DataAssetManager::GetPluginSettings());
	UpdateFilteredAssetList();
	InitializeAssetTypeComboBox(AssetManagerData.FilteredDataAssets);
	InitializeTextFontInfo();
	CreateDetailsView();

	FSlateFontInfo BigFont = FAppStyle::Get().GetFontStyle("NormalText");
	BigFont.Size = 32;

	AssetManagerWidgets.MenuBar = FDataAssetManagerMenuFactory::CreateMenuBar(SharedThis(this));
	TSharedPtr<SLayeredImage> FilterImage = CreateFilterImage();

	ColumnData.InitializeColumnOrder();
	InitializeColumnAdders();

	AssetManagerWidgets.DetailsView->SetRootObjectCustomizationInstance(MakeShared<FDetailsRootObjectCustomization>());

	/* CONTENT SPLITTER SECTION */
	TSharedPtr<SSplitter> ContentSplitter = SAssignNew(AssetManagerWidgets.Splitter, SSplitter).Orientation(EOrientation::Orient_Horizontal);
	ContentSplitter->AddSlot()
		.Value_Lambda([&]() { return SplitterValue.Get(); })
		.OnSlotResized_Lambda([&](float NewSize) { SplitterValue.Set(NewSize); })
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(2.0f, 6.0f, 0.0f, 6.0f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(4.0f, 0.0f, 4.0f, 0.0f)
						[
							SNew(SMenuAnchor)
								.Placement(EMenuPlacement::MenuPlacement_ComboBoxRight)
								[
									SAssignNew(AssetManagerWidgets.ListViewSearchBox, SFilterSearchBox)
										.HintText(LOCTEXT("SearchDetailsHint", "Search"))
										.Cursor(EMouseCursor::Hand)
										.OnTextChanged(this, &SDataAssetManagerWidget::OnSearchTextChanged)
										.DelayChangeNotificationsWhileTyping(true)
										.AddMetaData<FTagMetaData>(TEXT("Details.Search"))
										.Visibility_Raw(this, &SDataAssetManagerWidget::GetVisibilitySearchBox)
								]
						]

					+ SHorizontalBox::Slot()
						.HAlign(HAlign_Right)
						.AutoWidth()
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						[
							SAssignNew(AssetManagerWidgets.ComboButton, SComboButton)
								.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButtonWithIcon"))
								.ForegroundColor(FSlateColor::UseStyle())
								.ContentPadding(FMargin(1, 0))
								.ButtonContent()[FilterImage.ToSharedRef()]
								.MenuContent()[CreateComboButtonContent()]
						]
				]

			+ SVerticalBox::Slot()
				[
					SAssignNew(AssetManagerWidgets.AssetListView, SListView<TSharedPtr<FAssetData>>)
						.ListItemsSource(&AssetManagerData.FilteredDataAssets)
						.OnGenerateRow(this, &SDataAssetManagerWidget::GenerateAssetListRow)
						.OnSelectionChanged(this, &SDataAssetManagerWidget::OnAssetSelected)
						.SelectionMode(ESelectionMode::Multi)
						.HeaderRow(GenerateHeaderRow())]

			+ SVerticalBox::Slot()
				.FillHeight(0.6f)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
								.Text_Lambda([this]() { return GetSelectedTextBlockInfo(); })
						]
				]
		];

	ContentSplitter->AddSlot()
		.Value(0.6)
		[
			SNew(SOverlay)

				+ SOverlay::Slot()
				[
					AssetManagerWidgets.DetailsView.ToSharedRef()
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("Select DataAsset")))
						.Font(BigFont)
						.ColorAndOpacity(FLinearColor::White)
						.Visibility_Lambda([this]()
							{
								return IsDetailsViewEmpty()
									? EVisibility::Visible
									: EVisibility::Collapsed;
							})
				]
		];


	/* TOOLBAR BUTTONS SECTION*/
	TSharedRef<SHorizontalBox> ToolbarButtonsSection = SNew(SHorizontalBox);
	ToolbarButtonsSection->AddSlot()
		.AutoWidth().HAlign(HAlign_Center)
		[
			SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ContentPadding(FMargin(2))
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("SaveButtonTooltip", "Click to save changes."))
				.OnClicked_Lambda([this]() { SaveDataAsset(); return FReply::Handled(); })
				[
					SNew(SImage)
						.Cursor(EMouseCursor::Hand)
						.Image(FAppStyle::Get().GetBrush("Icons.Save"))
				]
		];

	ToolbarButtonsSection->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.Cursor(EMouseCursor::Hand)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.ContentPadding(FMargin(2))
				.ToolTipText(LOCTEXT("FindAssetToolTip", "Find asset in content browser"))
				.OnClicked_Lambda([this]() { SyncContentBrowserToSelectedAsset(); return FReply::Handled(); })
				[
					SNew(SImage)
						.Cursor(EMouseCursor::Hand)
						.Image(FAppStyle::GetBrush("Icons.Search"))
				]
		];

	ToolbarButtonsSection->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(SPositiveActionButton)
				.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
				.Text(FText::FromString(TEXT("Add")))
				.Cursor(EMouseCursor::Hand)
				.ToolTipText(LOCTEXT("AddDataAssetTooltip", "Click to add a new Data Asset."))
				.OnClicked_Lambda([this]() { CreateNewDataAsset(); return FReply::Handled(); })
		];

	ToolbarButtonsSection->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(SPositiveActionButton)
				.Icon(FAppStyle::Get()
					.GetBrush("MainFrame.SaveAll"))
				.Text(FText::FromString(TEXT("Save All")))
				.Cursor(EMouseCursor::Hand)
				.ToolTipText(LOCTEXT("SaveAllDataAsset", "Save All Data Assets"))
				.OnClicked_Lambda([this]() { SaveAllData(); return FReply::Handled(); })
		];


	/* MAINBOX SECTION */
	TSharedRef<SVerticalBox> MainBox = SNew(SVerticalBox);
	MainBox->AddSlot()
		.AutoHeight()
		[
			AssetManagerWidgets.MenuBar.ToSharedRef()
		];

	MainBox->AddSlot()
		.AutoHeight()
		[
			SNew(SSeparator)
				.Orientation(Orient_Vertical)
				.Thickness(1.0f)
				.ColorAndOpacity(FColor::Transparent)
		];

	MainBox->AddSlot()
		.AutoHeight()
		[
			ToolbarButtonsSection
		];

	MainBox->AddSlot()
		.AutoHeight()
		[
			SNew(SSeparator)
				.Orientation(Orient_Vertical)
				.Thickness(0.1f)
				.ColorAndOpacity(FColor::Transparent)
		];

	MainBox->AddSlot()
		[
			ContentSplitter.ToSharedRef()
		];


	ChildSlot
		[
			SNew(SBorder)
				.Padding(FMargin(5.0f))
				.BorderBackgroundColor(FColor::Transparent)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				[
					MainBox
				]
		];

	if (AssetManagerData.FilteredDataAssets.Num() > 0)
	{
		AssetManagerWidgets.AssetListView->SetSelection(AssetManagerData.FilteredDataAssets[0]);
		OnAssetSelected(AssetManagerData.FilteredDataAssets[0], ESelectInfo::Direct);
	}
}

SDataAssetManagerWidget::~SDataAssetManagerWidget()
{
	if (const FAssetRegistryModule* const AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry))
	{
		DataAssetManager::RemoveDelegateHandleSafe(ManagerDelegateHandles.AssetAddedDelegateHandle, AssetRegistryModule->Get().OnAssetAdded());
		DataAssetManager::RemoveDelegateHandleSafe(ManagerDelegateHandles.AssetRemovedDelegateHandle, AssetRegistryModule->Get().OnAssetRemoved());
		DataAssetManager::RemoveDelegateHandleSafe(ManagerDelegateHandles.AssetRenamedDelegateHandle, AssetRegistryModule->Get().OnAssetRenamed());
		DataAssetManager::RemoveDelegateHandleSafe(ManagerDelegateHandles.FilesLoadedHandle, AssetRegistryModule->Get().OnFilesLoaded());
	}
}

bool SDataAssetManagerWidget::IsDetailsViewEmpty() const
{
	return AssetManagerWidgets.DetailsView->GetSelectedObjects().Num() == 0;
}

#pragma region IDataAssetManagerInterface
void SDataAssetManagerWidget::CreateNewDataAsset()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(FText::FromString(TEXT("Select Folder")))
		.ClientSize(FVector2D(500, 600))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedPtr<SFolderTreeWidget> FolderTreeWidget;

	Window->SetContent(
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(10.f)
		[
			SAssignNew(FolderTreeWidget, SFolderTreeWidget)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10.f)
		.HAlign(HAlign_Right)
		[
			SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.f, 0.f)
				[
					SNew(SButton)
						.Text(FText::FromString(TEXT("Create")))
						.OnClicked_Lambda([this, FolderTreeWidget, Window]()
							{
								FSlateApplication::Get().RequestDestroyWindow(Window);
								FString SelectedDirectory = FolderTreeWidget->GetSelectedDirectory();

								FClassViewerInitializationOptions Options;
								Options.Mode = EClassViewerMode::ClassPicker;
								Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;
								TSharedPtr<DataAssetManager::Private::FAssetClassParentFilter> Filter = MakeShareable(new DataAssetManager::Private::FAssetClassParentFilter);
								Options.ClassFilters.Add(Filter.ToSharedRef());
								Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown;
								Filter->AllowedChildrenOfClasses.Add(UDataAsset::StaticClass());

								const FText TitleText = LOCTEXT("CreateDataAssetOptions", "Pick Class For Data Asset Instance");
								UClass* ChosenClass = nullptr;
								UClass* DataAssetClass = nullptr;
								if (SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UDataAsset::StaticClass()))
								{
									DataAssetClass = ChosenClass;
									if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
									{
										UE_LOG(SDataAssetManagerWidgetLog, Log, TEXT("Selected Data Asset Class: %s"), *DataAssetClass->GetName());
									}
									DataAssetManager::CreateNewDataAsset(DataAssetClass, SelectedDirectory);
								}

								return FReply::Handled();
							})
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.f, 0.f)
				[
					SNew(SButton)
						.Text(FText::FromString(TEXT("Cancel")))
						.OnClicked_Lambda([FolderTreeWidget, Window]()
							{
								FSlateApplication::Get().RequestDestroyWindow(Window);
								return FReply::Handled();
							})
				]
		]
	);

	FSlateApplication::Get().AddWindow(Window);
}

void SDataAssetManagerWidget::OpenSelectedDataAssetInEditor()
{
	const UObject* const AssetObject = AssetManagerData.SelectedAsset->GetAsset();
	if (!IsValid(AssetObject))
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("Selected Asset Object is not valid "));
		return;
	}

	const UDataAsset* const DataAsset = CastChecked<UDataAsset>(AssetObject);
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(DataAsset);
}

void SDataAssetManagerWidget::ToggleDataAssetListVisibility()
{
	bIsSlotVisible = !bIsSlotVisible;
	SplitterValue.Set(bIsSlotVisible ? DataAssetManager::DefaultSplitterValueWhenVisible : DataAssetManager::SplitterValueWhenHidden);
}

void SDataAssetManagerWidget::OpenAuditAsset()
{
	if (!IsSelectedAssetValid()) return;

	TArray<FName> SelectedAssetPackageNames;
	for (const TSharedPtr<FAssetData>& Items : GetAssetListSelectedItem())
	{
		const FName PackageName = Items->PackageName;
		SelectedAssetPackageNames.Add(PackageName);
	}

	IAssetManagerEditorModule::Get().OpenAssetAuditUI(SelectedAssetPackageNames);
}

void SDataAssetManagerWidget::ShowDocumentation()
{
	const FString& URL = GetDefault<UDataAssetManagerSettings>()->DocumentationURL;
	if (!URL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
	}
	else
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("Documentation URL is not set in settings."));
	}
}

void SDataAssetManagerWidget::SaveDataAsset()
{
	if (!IsSelectedAssetValid()) 
	{
		return;
	}

	UDataAsset* const DataAsset = CastChecked<UDataAsset>(AssetManagerData.SelectedAsset->GetAsset());
	DataAsset->MarkPackageDirty();

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);
	AssetRegistryModule.Get().AssetCreated(DataAsset);
	UPackage* const AssetPackage = DataAsset->GetOutermost();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(AssetPackage->GetName(), FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.Error = GError;
	SaveArgs.SaveFlags = SAVE_NoError | SAVE_KeepDirty;
	SaveArgs.bWarnOfLongFilename = false;

	if (UPackage::SavePackage(AssetPackage, DataAsset, *PackageFileName, SaveArgs))
	{
		if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
		{
			UE_LOG(SDataAssetManagerWidgetLog, Log, TEXT("DataAsset saved successfully: %s"), *PackageFileName);
		}
	}
}

void SDataAssetManagerWidget::SaveAllData()
{
	if (SaveAllDataAsset())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Log, TEXT("Save All Data"));
	}
}

void SDataAssetManagerWidget::SyncContentBrowserToSelectedAsset()
{
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	if (!IsSelectedAssetValid()) 
	{
		return;
	}

	ContentBrowserModule.Get().SyncBrowserToAssets({ *AssetManagerData.SelectedAsset });
}

void SDataAssetManagerWidget::CopyToClipboard(bool bCopyPaths)
{
	if (!IsSelectedAssetValid())
	{
		return;
	}

	TArray<FAssetData> SelectedPackages;
	SelectedPackages.Add(*AssetManagerData.SelectedAsset);

	SelectedPackages.Sort([](const FAssetData& One, const FAssetData& Two)
		{
			/** Sort assets by package path for a consistent clipboard order */
			return One.PackagePath.Compare(Two.PackagePath) < 0;
		});

	// Build clipboard text from the selected asset packages
	const FString ClipboardText = FString::JoinBy(SelectedPackages, LINE_TERMINATOR, [bCopyPaths](const FAssetData& Item)
		{
			if (bCopyPaths)
			{
				const FString ItemFilename = FPackageName::LongPackageNameToFilename(Item.PackageName.ToString(), FPackageName::GetAssetPackageExtension());
				if (FPaths::FileExists(ItemFilename))
				{
					return FPaths::ConvertRelativePathToFull(ItemFilename);
				}
				else
				{
					return FString::Printf(TEXT("%s: No file on disk"), *Item.AssetName.ToString());
				}
			}
			else
			{
				return Item.GetExportTextName();
			}
		});

	FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);
}

void SDataAssetManagerWidget::OpenReferenceViewer()
{
	if (!IsSelectedAssetValid()) return;

	TArray<FAssetData> AssetDataArray;
	AssetDataArray.Add(*AssetManagerData.SelectedAsset.Get());

	DataAssetManager::ProcessAssetData(AssetDataArray, [](const TArray<FAssetIdentifier>& AssetIdentifiers)
		{
			IAssetManagerEditorModule::Get().OpenReferenceViewerUI(AssetIdentifiers);
		});
}

void SDataAssetManagerWidget::OpenSizeMap()
{
	if (!IsSelectedAssetValid()) return;

	TArray<FAssetData> AssetDataArray;
	AssetDataArray.Add(*AssetManagerData.SelectedAsset.Get());

	DataAssetManager::ProcessAssetData(AssetDataArray, [](const TArray<FAssetIdentifier>& AssetIdentifiers)
		{
			IAssetManagerEditorModule::Get().OpenSizeMapUI(AssetIdentifiers);
		});
}

void SDataAssetManagerWidget::OpenPluginSettings()
{
	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>(DataAssetManager::ModuleName::Settings);
	SettingsModule.ShowViewer("Project", "Plugins", "DataAssetManager");
}

void SDataAssetManagerWidget::ShowSourceControlDialog()
{
	ISourceControlModule::Get().ShowLoginDialog(FSourceControlLoginClosed(), ELoginWindowMode::Modeless);
}
void SDataAssetManagerWidget::RestartPlugin()
{
	FDataAssetManagerModule& DataManagerModule = FModuleManager::LoadModuleChecked<FDataAssetManagerModule>(DataAssetManager::ModuleName::DataAssetManager);
	DataManagerModule.RestartWidget();
}
void SDataAssetManagerWidget::OpenMessageLogWindow()
{
	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>(DataAssetManager::ModuleName::MessageLog);
	MessageLogModule.OpenMessageLog("AssetCheck");
}

void SDataAssetManagerWidget::OpenOutputLogWindow()
{
	FOutputLogModule& OutputLogModule = FModuleManager::LoadModuleChecked<FOutputLogModule>(DataAssetManager::ModuleName::OutputLog);
	OutputLogModule.OpenOutputLog();
}

bool SDataAssetManagerWidget::CanRename() const
{
	return EditableWidgets.bCanRename;
}
#pragma endregion IDataAssetManagerInterface

#pragma region DataAssetManagerInterface
void SDataAssetManagerWidget::HandleAssetDoubleClick(const FGeometry& InGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OpenSelectedDataAssetInEditor();
	}
}

FText SDataAssetManagerWidget::GetSelectedTextBlockInfo() const
{
	const FString SelectedStrItems = GetAssetListSelectedItem().Num() > 0
		? FString::Printf(TEXT("(%d selected)"), GetAssetListSelectedItem().Num())
		: TEXT("");

	return FText::FromString(FString::Printf(TEXT("   %d items %s"), AssetManagerData.FilteredDataAssets.Num(), *SelectedStrItems));
}

void SDataAssetManagerWidget::RegisterEditableText(TSharedPtr<FAssetData> AssetData, TSharedRef<SEditableText> EditableText)
{
	EditableWidgets.AddEditableTextWidget(AssetData.Get(), EditableText);
}

void SDataAssetManagerWidget::HandleAssetRename(TSharedPtr<FAssetData> AssetData, const FText& InText, ETextCommit::Type CommitMethod)
{
	if (!AssetManagerData.SelectedAsset.IsValid() || InText.IsEmpty())
	{
		EditableWidgets.bRenamedProgress = false;
		return;
	}

	if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s EditableTextWidgets counts %d"), ANSI_TO_TCHAR(__FUNCTION__), EditableWidgets.EditableTextWidgets.Num());
	}

	if (CommitMethod == ETextCommit::OnEnter)
	{
		UObject* const Asset = AssetManagerData.SelectedAsset->GetAsset();
		if (!IsValid(Asset))
		{
			UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("Asset is not valid"));
			return;
		}
		const FString NewName = InText.ToString();
		FString PackagePath = Asset->GetPathName();
		PackagePath = FPaths::GetPath(PackagePath);
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>(DataAssetManager::ModuleName::AssetTools).Get();
		if (AssetTools.RenameAssets({ FAssetRenameData(Asset, PackagePath, NewName) }))
		{
			/** Use AssetTools to ensure proper rename across disk, memory, and editor state */
			EditableWidgets.bRenamedProgress = false;

			if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
			{
				UE_LOG(SDataAssetManagerWidgetLog, Log, TEXT("Asset renamed %s"), *Asset->GetName());
			}
		}
	}
}

EVisibility SDataAssetManagerWidget::GetVisibilitySearchBox() const
{
	return SplitterValue.Get() < DataAssetManager::SearchBoxHideThreshold ? EVisibility::Hidden : EVisibility::Visible;
}

void SDataAssetManagerWidget::SubscribeToAssetRegistryEvent()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);

	const auto SubscribeDelegates = [this, &AssetRegistryModule]()
		{
			ManagerDelegateHandles.AssetAddedDelegateHandle = AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &SDataAssetManagerWidget::OnAssetAdded);
			ManagerDelegateHandles.AssetRemovedDelegateHandle = AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &SDataAssetManagerWidget::OnAssetRemoved);
			ManagerDelegateHandles.AssetRenamedDelegateHandle = AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &SDataAssetManagerWidget::OnAssetRenamed);
		};

	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		ManagerDelegateHandles.FilesLoadedHandle = AssetRegistryModule.Get().OnFilesLoaded().AddLambda(
			[this, SubscribeDelegates]()
			{
				SubscribeDelegates();
			});
	}
	else
	{
		SubscribeDelegates();
	}

}

void SDataAssetManagerWidget::InitializeTextFontInfo()
{
	TextFontInfo = FCoreStyle::Get().GetFontStyle(FName("NormalText"));
	TextFontInfo.Size = DataAssetManager::DataAssetFontSize;
}

void SDataAssetManagerWidget::CreateDetailsView()
{
	const FDetailsViewArgs DetailsViewArgs = CreateDetailsViewArgs();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(DataAssetManager::ModuleName::PropertyEditor);
	AssetManagerWidgets.DetailsView = PropertyModule.CreateDetailView(DetailsViewArgs);
}

FDetailsViewArgs SDataAssetManagerWidget::CreateDetailsViewArgs() const
{
	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bShowObjectLabel = false;
	Args.bCustomNameAreaLocation = false;
	Args.bAllowMultipleTopLevelObjects = true;
	Args.bShowOptions = true;

	return Args;
}

TSharedPtr<SLayeredImage> SDataAssetManagerWidget::CreateFilterImage()
{
	TSharedRef<SLayeredImage> LayeredImage = SNew(SLayeredImage)
		.Image(FAppStyle::Get().GetBrush("Icons.Filter"))
		.ColorAndOpacity(FSlateColor(FColor::White))
		.Visibility_Lambda([this]()
			{
				return SplitterValue.Get() < 0.05f ? EVisibility::Hidden : EVisibility::Visible;
			});

	return LayeredImage;
}


void SDataAssetManagerWidget::CreateContextMenuFromDataAsset(const FGeometry& InGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		FMenuBuilder MenuBuilder(true, nullptr);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("RenameMenuEntry", "Rename         "),
			LOCTEXT("RenameMenuTooltip", "Rename this item"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::FocusOnSelectedAsset),
				FCanExecuteAction::CreateLambda([this]() { return CanRename(); })));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("DeleteMenuEntry", "Delete         "),
			LOCTEXT("DeleteMenuTooltip", "Delete this item"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Delete"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::DeleteDataAsset)));

		FSlateApplication::Get().PushMenu(
			AsShared(),
			FWidgetPath(),
			MenuBuilder.MakeWidget(),
			MouseEvent.GetScreenSpacePosition(),
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
	}
}

TSharedRef<SWidget> SDataAssetManagerWidget::CreateComboButtonContent()
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection*/false, nullptr);

	// --- Reset Filters ---
	MenuBuilder.BeginSection("ResetSection", FText::FromString("Actions"));
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString("Reset Filters"),
			FText::FromString("Clear all selected type and plugin filters."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]()
				{
					AssetManagerData.ActiveFilters.Empty();
					AssetManagerData.ActivePluginFilters.Empty();
					UpdateFilteredAssetList();
				}))
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.AddWidget(
		SNew(SBox).Padding(FMargin(5.f, 7.f))
		[
			SNew(SSeparator)
		],
		FText::GetEmpty()
	);

	// --- Filters by Asset Type ---
	MenuBuilder.BeginSection("TypeFilters", FText::FromString("Asset Types"));
	for (const TSharedPtr<FString>& FilterItem : ComboBoxAssetListItems)
	{
		AddToggleFilterMenuEntry(MenuBuilder, *FilterItem, AssetManagerData.ActiveFilters, [this]()
			{
				UpdateFilteredAssetList();
			});
	}
	MenuBuilder.EndSection();

	MenuBuilder.AddWidget(
		SNew(SBox).Padding(FMargin(5.f, 7.f))
		[
			SNew(SSeparator)
		],
		FText::GetEmpty()
	);

	// --- Filters by Plugin ---
	MenuBuilder.BeginSection("PluginFilters", FText::FromString("Plugins"));

	for (const TSharedPtr<FString>& PluginFilterItem : PluginFilterListItems)
	{
		AddToggleFilterMenuEntry(MenuBuilder, *PluginFilterItem, AssetManagerData.ActivePluginFilters, [this]
			{
				UpdateFilteredAssetList();
			});
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SDataAssetManagerWidget::AddToggleFilterMenuEntry(FMenuBuilder& MenuBuilder, const FString& FilterName, TSet<FString>& ActiveFilters, TFunction<void()> UpdateFunc)
{
	FUIAction Action(
		FExecuteAction::CreateLambda([&ActiveFilters, FilterName, UpdateFunc]()
			{
				if (ActiveFilters.Contains(FilterName))
				{
					ActiveFilters.Remove(FilterName);
				}
				else
				{
					ActiveFilters.Add(FilterName);
				}
				UpdateFunc();
			}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([&ActiveFilters, FilterName]()
			{
				return ActiveFilters.Contains(FilterName);
			})
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(FilterName),
		FText::GetEmpty(),
		FSlateIcon(),
		Action,
		NAME_None,
		EUserInterfaceActionType::ToggleButton
	);
}

FReply SDataAssetManagerWidget::OnItemClicked(TSharedPtr<FString> SourceItem)
{
	SelectedAssetType = SourceItem;
	UpdateFilteredAssetList();
	if (AssetManagerWidgets.ComboButton.IsValid())
	{
		AssetManagerWidgets.ComboButton->SetIsOpen(false);
	}

	return FReply::Handled();
}

void SDataAssetManagerWidget::LoadDataAssets(const UDataAssetManagerSettings* PluginSettings)
{
	if (!IsValid(PluginSettings))
	{
		return;
	}

	MEASURE_SCOPE("Load Data Assets");

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
		if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project)
		{
			FString MountPoint = Plugin->GetMountedAssetPath();
			if (!MountPoint.IsEmpty())
			{
				AssetDirectories.Add(MountPoint);
				PluginFilterListItems.Add(MakeShared<FString>(MountPoint));
			}
		}
	}

	TArray<FTopLevelAssetPath> IgnoredClassPaths;
	IgnoredClassPaths.Reserve(PluginSettings->ExcludedScanAssetTypes.Num());
	for (const TSubclassOf<UDataAsset>& IgnoredClass : PluginSettings->ExcludedScanAssetTypes)
	{
		if (IsValid(IgnoredClass))
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
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s Failed to get assets by class"), ANSI_TO_TCHAR(__FUNCTION__));
		return;
	}

	AssetManagerData.DataAssets.Reset(AssetDataArray.Num());
	for (const FAssetData& AssetData : AssetDataArray)
	{
		if (IgnoredClassPaths.Contains(AssetData.AssetClassPath))
		{
			continue;
		}

		FString NormalizedAssetPath = AssetData.PackagePath.ToString();
		FPaths::NormalizeDirectoryName(NormalizedAssetPath);

		// Check if asset is in any of our directories
		if (Algo::AnyOf(AssetDirectories, [&NormalizedAssetPath](const FString& Directory)
			{
				return NormalizedAssetPath.StartsWith(Directory);
			}))
		{
			AssetManagerData.DataAssets.Add(MakeShared<FAssetData>(AssetData));
		}
	}

	/**
	 * Sorts the found DataAssets alphabetically by asset name.
	 *
	 * Uses lexicographical comparison (LexicalLess) which:
	 * - Is case-sensitive
	 * - More efficient than string comparison as it works directly with FName
	 */
	AssetManagerData.DataAssets.Sort([](const TSharedPtr<FAssetData>& A, const TSharedPtr<FAssetData>& B)
		{
			return A->AssetName.LexicalLess(B->AssetName);
		});
}

void SDataAssetManagerWidget::UpdateFilteredAssetList()
{
	MEASURE_SCOPE("UpdateFilteredAssetList");

	const FString SearchString = SearchText.Get().ToString();

	const int32 AssetCount = AssetManagerData.DataAssets.Num();
	TBitArray<> VisibilityMask(false, AssetCount); // false = не отображать

	for (int32 Index = 0; Index < AssetCount; ++Index)
	{
		const TSharedPtr<FAssetData>& AssetData = AssetManagerData.DataAssets[Index];
		if (!AssetData.IsValid())
		{
			continue;
		}

		const FString AssetClassName = AssetData->AssetClassPath.GetAssetName().ToString();
		const FString PackagePath = AssetData->PackagePath.ToString();
		const bool bMatchesType = AssetManagerData.ActiveFilters.Num() == 0
			|| AssetManagerData.ActiveFilters.Contains(AssetClassName);

		const bool bNameMatches = SearchString.IsEmpty()
			|| AssetData->AssetName.ToString().Contains(SearchString);

		bool bMatchesPlugin = true;
		if (AssetManagerData.ActivePluginFilters.Num() > 0)
		{
			bMatchesPlugin = false;
			for (const FString& PluginMount : AssetManagerData.ActivePluginFilters)
			{
				if (PackagePath.StartsWith(PluginMount))
				{
					bMatchesPlugin = true;
					break;
				}
			}
		}

		if (bMatchesType && bNameMatches && bMatchesPlugin)
		{
			VisibilityMask[Index] = true;
		}
	}

	AssetManagerData.FilteredDataAssets.Empty();
	for (int32 Index = 0; Index < AssetCount; ++Index)
	{
		if (VisibilityMask[Index])
		{
			AssetManagerData.FilteredDataAssets.Add(AssetManagerData.DataAssets[Index]);
		}
	}

	if (AssetManagerWidgets.AssetListView.IsValid())
	{
		AssetManagerWidgets.AssetListView->RequestListRefresh();
	}
}

void SDataAssetManagerWidget::OnSearchTextChanged(const FText& InText)
{
	SearchText.Set(InText);
	UpdateFilteredAssetList();
}

TSharedRef<ITableRow> SDataAssetManagerWidget::GenerateAssetListRow(TSharedPtr<FAssetData> Item, const TSharedRef<STableViewBase>& OwnerSTable)
{
	TSharedRef<SDataAssetTableRow> TableRow = SNew(SDataAssetTableRow, OwnerSTable)
		.Item(Item)
		.OnAssetRenamed(this, &SDataAssetManagerWidget::HandleAssetRename)
		.OnCreateContextMenu(this, &SDataAssetManagerWidget::CreateContextMenuFromDataAsset)
		.OnAssetDoubleClicked(this, &SDataAssetManagerWidget::HandleAssetDoubleClick)
		.OnRegisterEditableText(this, &SDataAssetManagerWidget::RegisterEditableText)
		.OnMouseButtonDown(this, &SDataAssetManagerWidget::HandleRowMouseButtonDown);

	return TableRow;
}

FReply SDataAssetManagerWidget::HandleRowMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}
	return FReply::Handled();
}

void SDataAssetManagerWidget::InitializeAssetTypeComboBox(TArray<TSharedPtr<FAssetData>> AssetDataList)
{
	if (!ComboBoxAssetListItems.IsEmpty())
	{
		ComboBoxAssetListItems.Reset();
	}

	TSet<FString> UniqueAssetNames;
	for (const auto& AssetData : AssetDataList)
	{
		if (AssetData.IsValid())
		{
			const FString AssetName = AssetData->AssetClassPath.GetAssetName().ToString();
			if (!UniqueAssetNames.Contains(AssetName))
			{
				/** Avoid duplicate class names in filter combo box */
				UniqueAssetNames.Add(AssetName);
				ComboBoxAssetListItems.Add(MakeShared<FString>(AssetName));
			}
		}
	}

	if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s FilteredDataAssets: %i"), ANSI_TO_TCHAR(__FUNCTION__), AssetManagerData.FilteredDataAssets.Num());
	}
}

void SDataAssetManagerWidget::FocusOnNewlyAddedAsset(const FAssetData& NewAssetData)
{
	TSharedPtr<FAssetData> NewAssetPtr = nullptr;
	for (const TSharedPtr<FAssetData>& Asset : AssetManagerData.FilteredDataAssets)
	{
		if (Asset->PackageName == NewAssetData.PackageName)
		{
			NewAssetPtr = Asset;
			break;
		}
	}

	if (!NewAssetPtr.IsValid())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s Newly added asset '%s' not found in filtered list"),
			ANSI_TO_TCHAR(__FUNCTION__), *NewAssetData.PackageName.ToString());
		return;
	}

	if (const UObject* const AssetObject = NewAssetPtr->GetAsset())
	{
		if (AssetObject->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
		{
			/**	Avoid using assets that are not fully loaded.*/
			/**	Accessing such objects could result in crashes or undefined behavior.*/
			UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s: Asset '%s' is not fully loaded (flags: %X)"),
				ANSI_TO_TCHAR(__FUNCTION__), *AssetObject->GetName(), AssetObject->GetFlags());
			return;
		}
	}
	else
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s: Failed to load asset '%s'"),
			ANSI_TO_TCHAR(__FUNCTION__), *NewAssetData.PackageName.ToString());
		return;
	}

	if (AssetManagerWidgets.AssetListView.IsValid())
	{
		AssetManagerWidgets.AssetListView->SetSelection(NewAssetPtr);
		OnAssetSelected(NewAssetPtr, ESelectInfo::Direct);
		AssetManagerWidgets.AssetListView->RequestScrollIntoView(NewAssetPtr);
	}
}

FReply SDataAssetManagerWidget::ColumnButtonClicked(const FGeometry& InGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		MenuBuilder.BeginSection("AdditionalActions", LOCTEXT("AdditionalActionsSection", "Additional Actions"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ToggleAllColumns", "Hide All Columns"),
				LOCTEXT("ToggleAllColumnsTooltip", "Hide or show all columns at once"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this]()
					{
						ColumnData.ToggleAllColumnsVisibility();
						UpdateColumnVisibility();
					}),
					FCanExecuteAction(),
					FIsActionChecked::CreateLambda([this]()
						{
							/* Checked if all columns are hidden */
							return ColumnData.AreAllColumnsHidden();
						})
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
		}
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("ColumnVisibility", LOCTEXT("ColumnVisibilitySection", "Visible Columns"));
		{
			AddColumnMenuEntry(MenuBuilder,
				LOCTEXT("ShowType", "Show Type"),
				LOCTEXT("ShowTypeTooltip", "Toggle the visibility of the Type column"),
				&ColumnData.ColumnVisibility.bShowTypeColumn);
			AddColumnMenuEntry(MenuBuilder,
				LOCTEXT("ShowPath", "Show Path"),
				LOCTEXT("ShowPathTooltip", "Toggle the visibility of the Path column"),
				&ColumnData.ColumnVisibility.bShowPathColumn);
			AddColumnMenuEntry(MenuBuilder,
				LOCTEXT("ShowDiskSize", "Show Disk Size"),
				LOCTEXT("ShowDiskSizeTooltip", "Toggle the visibility of the Disk Size column"),
				&ColumnData.ColumnVisibility.bShowDiskSizeColumn);
			AddColumnMenuEntry(MenuBuilder,
				LOCTEXT("RevisionControl", "Revision Control"),
				LOCTEXT("RevisionControlTooltip", "Toggle the visibility of the Revision control column"),
				&ColumnData.ColumnVisibility.bShowRevisionColumn);
		}
		MenuBuilder.EndSection();

		FSlateApplication::Get().PushMenu(
			SharedThis(this),
			FWidgetPath(),
			MenuBuilder.MakeWidget(),
			MouseEvent.GetScreenSpacePosition(),
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
		);

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SDataAssetManagerWidget::AddColumnToHeader(TSharedPtr<SHeaderRow> InHeaderRow, const FName& ColumnId, const FString& Label, const float FillWidth)
{
	InHeaderRow->AddColumn(SHeaderRow::FColumn::FArguments()
		.ColumnId(ColumnId)
		.DefaultLabel(FText::FromString(Label))
		.FillWidth(FillWidth)
		.HeaderContent()
		[
			SNew(SBorder)
				.BorderBackgroundColor(FSlateColor(FColor::Transparent))
				.OnMouseButtonDown(this, &SDataAssetManagerWidget::ColumnButtonClicked)
				[
					SNew(STextBlock)
						.Text(FText::FromString(Label))
				]
		]);
}

const FSlateBrush* SDataAssetManagerWidget::GetRevisionControlColumnIconBadge() const
{
	if (ISourceControlModule::Get().IsEnabled())
	{
		return FRevisionControlStyleManager::Get().GetBrush("RevisionControl.Icon.ConnectedBadge");
	}
	else
	{
		return nullptr;
	}
}

void SDataAssetManagerWidget::InitializeColumnAdders()
{
	ColumnData.InitializeColumnAdders(
		[this](TSharedPtr<SHeaderRow> HeaderRow, FName ColumnId, const TCHAR* Label, float Width)
		{
			AddColumnToHeader(HeaderRow, ColumnId, Label, Width);
		},
		[this]
		{
			return CreateRevisionControlColumn();
		}
	);
}

SHeaderRow::FColumn::FArguments SDataAssetManagerWidget::CreateRevisionControlColumn()
{
	TSharedRef<SLayeredImage> RevisionControlColumnIcon = SNew(SLayeredImage)
		.ColorAndOpacity(FSlateColor::UseForeground())
		.Image(FRevisionControlStyleManager::Get()
		.GetBrush("RevisionControl.Icon"));

	RevisionControlColumnIcon->AddLayer(TAttribute<const FSlateBrush*>::CreateSP(this,
		&SDataAssetManagerWidget::GetRevisionControlColumnIconBadge));

	return SHeaderRow::Column(DataAssetListColumns::ColumnID_RC)
		.FixedWidth(DataAssetManager::RCFixedWidth)
		.HAlignHeader(HAlign_Center)
		.VAlignHeader(VAlign_Center)
		.HAlignCell(HAlign_Center)
		.VAlignCell(VAlign_Center)
		.DefaultLabel(LOCTEXT("Column_RC", "Revision Control"))
		[
			RevisionControlColumnIcon
		];
}


TSharedRef<SHeaderRow> SDataAssetManagerWidget::GenerateHeaderRow()
{
	return ColumnData.BuildHeaderRow();
}

void SDataAssetManagerWidget::UpdateColumnVisibility()
{
	ColumnData.UpdateColumnVisibility(AssetManagerWidgets.AssetListView->GetHeaderRow());
	AssetManagerWidgets.AssetListView->RequestListRefresh();
}

void SDataAssetManagerWidget::FocusOnSelectedAsset()
{
	if (!IsSelectedAssetValid()) return;

	UE_LOG(LogTemp, Warning, TEXT("%s EditableTextWidgets counts %d"), ANSI_TO_TCHAR(__FUNCTION__), EditableWidgets.EditableTextWidgets.Num());

	TSharedPtr<FAssetData> FoundAsset = nullptr;
	for (const TSharedPtr<FAssetData>& DataAsset : AssetManagerData.FilteredDataAssets)
	{
		if (!DataAsset->PackageName.IsEqual(AssetManagerData.SelectedAsset->PackageName))
		{
			continue;
		}

		FoundAsset = DataAsset;
		break;
	}

	const TPair<FName, FName> WidgetKey(FoundAsset->PackagePath, FoundAsset->AssetName);
	if (TSharedPtr<SEditableText>* const FoundWidget = EditableWidgets.EditableTextWidgets.Find(WidgetKey))
	{
		if (FoundWidget->IsValid())
		{
			EditableWidgets.bRenamedProgress = true;
			(*FoundWidget)->SetIsReadOnly(false);

			FSlateApplication::Get().SetKeyboardFocus(*FoundWidget, EFocusCause::SetDirectly);
			AssetManagerWidgets.EditableTextWidget = *FoundWidget;
		}
	}
	else
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("Widget not found in EditableTextMap"));
	}
}

void SDataAssetManagerWidget::OnAssetSelected(TSharedPtr<FAssetData> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (!SelectedItem.IsValid())
	{
		AssetManagerWidgets.DetailsView->SetObject(nullptr);
		return;
	}

	AssetManagerData.SelectedAsset = SelectedItem;

	OpenDetailViewPanelForAsset(SelectedItem);
	const TArray<TSharedPtr<FAssetData>> SelectedItems = GetAssetListSelectedItem();

	if (SelectedItems.Num() == 1)
	{
		EditableWidgets.bCanRename = true;
	}
	else 
	{
		EditableWidgets.bCanRename = false;
	}
	
}

void SDataAssetManagerWidget::OpenDetailViewPanelForAsset(TSharedPtr<FAssetData> SelectedItem)
{
	if (!SelectedItem.IsValid())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("Selected Item is not valid"));
		return;
	}

	if (!GetAssetListSelectedItem().IsEmpty())
	{
		TArray<TWeakObjectPtr<UObject>> ObjectsToView;
		for (const TSharedPtr<FAssetData>& AssetData : GetAssetListSelectedItem())
		{
			if (AssetData.IsValid())
			{
				if (UDataAsset* const Asset = Cast<UDataAsset>(AssetData->GetAsset()))
				{
					ObjectsToView.Add(Asset);
				}
			}
		}

		AssetManagerWidgets.DetailsView->SetObjects(ObjectsToView, true, true);
	}
}

void SDataAssetManagerWidget::ProcessAssetData(const TArray<FAssetData>& RefAssetData, TFunction<void(const TArray<FAssetIdentifier>&)> ProcessFunction)
{
	TArray<FAssetIdentifier> AssetIdentifiers;
	/** Converts asset data to identifiers for reference viewer / size map / audit tools */
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(RefAssetData, AssetIdentifiers);
	ProcessFunction(AssetIdentifiers);
}

void SDataAssetManagerWidget::OnAssetAdded(const FAssetData& NewAssetData)
{
	RefreshAssetList();
	FocusOnNewlyAddedAsset(NewAssetData);

	if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s Call Delegate"), ANSI_TO_TCHAR(__FUNCTION__));
	}
}

void SDataAssetManagerWidget::OnAssetRemoved(const FAssetData& AssetToRemoved)
{
	RefreshAssetList();
	if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s Call Delegate"), ANSI_TO_TCHAR(__FUNCTION__));
	}
}

void SDataAssetManagerWidget::OnAssetRenamed(const FAssetData& NewAssetData, const FString& Name)
{
	RefreshAssetList();
	FocusOnNewlyAddedAsset(NewAssetData);

	if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s Call Delegate"), ANSI_TO_TCHAR(__FUNCTION__));
	}
}

void SDataAssetManagerWidget::RefreshAssetList()
{
	LoadDataAssets(DataAssetManager::GetPluginSettings());
	UpdateFilteredAssetList();
	InitializeAssetTypeComboBox(AssetManagerData.DataAssets);
}

void SDataAssetManagerWidget::DeleteDataAsset()
{
#pragma region Depricated
	//TArray<FAssetData> AssetsToDelete;
	//TArray<FAssetData> LockedAssets;

	//for (const TSharedPtr<FAssetData>& Item : GetAssetListSelectedItem())
	//{
	//	if (!Item.IsValid()) continue;

	//	const FAssetData AssetData = *Item;
	//	const FString PackageFilename = USourceControlHelpers::PackageFilename(AssetData.PackageName.ToString());
	//	const FSourceControlState FileState = USourceControlHelpers::QueryFileState(PackageFilename);

	//	bool bIsLocked = false;
	//	if (FileState.bIsValid)
	//	{
	//		bIsLocked = FileState.bIsCheckedOut || FileState.bIsCheckedOutOther || (FileState.bIsSourceControlled && !FileState.bCanCheckIn);
	//	}

	//	if (bIsLocked)
	//	{
	//		LockedAssets.Add(AssetData);
	//	}
	//	else
	//	{
	//		AssetsToDelete.Add(AssetData);
	//	}
	//}

	//if (LockedAssets.Num() > 0)
	//{
	//	FString LockedAssetsList;
	//	for (const FAssetData& Asset : LockedAssets)
	//	{
	//		LockedAssetsList += FString::Printf(TEXT("\n - %s"), *Asset.AssetName.ToString());

	//		const FString PackageFilename = USourceControlHelpers::PackageFilename(Asset.PackageName.ToString());
	//		FSourceControlState State = USourceControlHelpers::QueryFileState(PackageFilename);

	//		if (State.bIsCheckedOutOther)
	//		{
	//			LockedAssetsList += TEXT(" (Checked out by another user)");
	//		}
	//		else if (State.bIsCheckedOut)
	//		{
	//			LockedAssetsList += TEXT(" (Checked out by you)");
	//		}
	//		else if (!State.bCanCheckIn)
	//		{
	//			LockedAssetsList += TEXT(" (Pending review or locked)");
	//		}
	//	}

	//	FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("CannotDeleteLockedAssets", "Cannot delete assets locked in Revision Control:{0}\n\nPlease check them in or unlock first."),
	//		FText::FromString(LockedAssetsList)));
	//}

	//EditableWidgets.bCanRename = false;
	//if (AssetsToDelete.Num() > 0)
	//{
	//	DataAssetManager::DeleteMultiplyAsset(AssetsToDelete);
	//}
#pragma endregion Depricated

	auto AssetsPair = CategorizeAssets(GetAssetListSelectedItem());
	TArray<FAssetData>& AssetsToDelete = AssetsPair.Get<0>();
	TArray<FAssetData>& LockedAssets = AssetsPair.Get<1>();

	if (LockedAssets.Num() > 0)
	{
		FString LockedAssetsList;
		for (const FAssetData& Asset : LockedAssets)
		{
			LockedAssetsList += FString::Printf(TEXT("\n - %s"), *Asset.AssetName.ToString());

			const FString PackageFilename = USourceControlHelpers::PackageFilename(Asset.PackageName.ToString());
			FSourceControlState State = USourceControlHelpers::QueryFileState(PackageFilename);

			if (State.bIsCheckedOutOther)
			{
				LockedAssetsList += TEXT(" (Checked out by another user)");
			}
			else if (State.bIsCheckedOut)
			{
				LockedAssetsList += TEXT(" (Checked out by you)");
			}
			else if (!State.bCanCheckIn)
			{
				LockedAssetsList += TEXT(" (Pending review or locked)");
			}
		}

		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("CannotDeleteLockedAssets", "Cannot delete assets locked in Revision Control:{0}\n\nPlease check them in or unlock first."),
			FText::FromString(LockedAssetsList)
		));
	}

	EditableWidgets.bCanRename = false;
	if (AssetsToDelete.Num() > 0)
	{
		DataAssetManager::DeleteMultiplyAsset(AssetsToDelete);
	}
}

// TODO !!! original code copy-paste from FAssetFileContextMenu class 
// AssetFileContextMenu::ExecuteShowAssetMetaData() 
// (*mini refactoring for original code *add const corection)
// 
void SDataAssetManagerWidget::ShowAssetMetaData()
{
	for (const TSharedPtr<FAssetData>& AssetData : GetAssetListSelectedItem())
	{
		const UObject* const Asset = AssetData.Get()->GetAsset();
		if (!IsValid(Asset)) continue;

		const TMap<FName, FString>* TagValues = UMetaData::GetMapForObject(Asset);
		if (TagValues)
		{
			// Create and display a resizable window to display the MetaDataView for each asset with metadata
			const FString Title = FString::Printf(TEXT("Metadata: %s"), *AssetData.Get()->AssetName.ToString());

			TSharedPtr<SWindow> Window = SNew(SWindow)
				.Title(FText::FromString(Title))
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.MinWidth(DataAssetManager::MetaDataWindowWidth)
				.MinHeight(DataAssetManager::MetaDataWindowHeight)
				[
					SNew(SBorder)
						.Padding(4.f)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SMetaDataView, *TagValues)
						]
				];

			FSlateApplication::Get().AddWindow(Window.ToSharedRef());
		}
		else
		{
			FNotificationInfo Info(FText::Format(LOCTEXT("NoMetaDataFound", "No metadata found for asset {0}."), FText::FromString(Asset->GetName())));
			Info.ExpireDuration = DataAssetManager::ExpireDuration;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}
}

TTuple<TArray<FAssetData>, TArray<FAssetData>> SDataAssetManagerWidget::CategorizeAssets(const TArray<TSharedPtr<FAssetData>>& SelectedItems)
{
	TArray<FAssetData> AssetsToDelete;
	TArray<FAssetData> LockedAssets;

	for (const TSharedPtr<FAssetData>& Item : SelectedItems)
	{
		if (!Item.IsValid())
		{
			continue;
		}

		const FAssetData AssetData = *Item;
		const FString PackageFilename = USourceControlHelpers::PackageFilename(AssetData.PackageName.ToString());
		const FSourceControlState FileState = USourceControlHelpers::QueryFileState(PackageFilename);

		bool bIsLocked = FileState.bIsValid &&
			(FileState.bIsCheckedOut || FileState.bIsCheckedOutOther ||
				(FileState.bIsSourceControlled && !FileState.bCanCheckIn));

		if (bIsLocked)
		{
			LockedAssets.Add(AssetData);
		}
		else
		{
			AssetsToDelete.Add(AssetData);
		}
	}

	return MakeTuple(AssetsToDelete, LockedAssets);
}

void SDataAssetManagerWidget::AddColumnMenuEntry(FMenuBuilder& MenuBuilder, FText Label, FText Tooltip, bool* ColumnFlag)
{
	MenuBuilder.AddMenuEntry(Label,
		Tooltip,
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(SharedThis(this), &SDataAssetManagerWidget::ToggleColumn, ColumnFlag),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(SharedThis(this), &SDataAssetManagerWidget::IsColumnVisible, ColumnFlag)),
		NAME_None,
		EUserInterfaceActionType::ToggleButton);
}

bool SDataAssetManagerWidget::IsColumnVisible(bool* bColumnPtr) const
{
	check(bColumnPtr);
	return *bColumnPtr;
}

void SDataAssetManagerWidget::ToggleColumn(bool* bColumnPtr)
{
	check(bColumnPtr);
	*bColumnPtr = !(*bColumnPtr);
	UpdateColumnVisibility();
}

bool SDataAssetManagerWidget::SaveAllDataAsset()
{
	constexpr bool bPromptUserToSave = false;
	constexpr bool bSaveMapPackages = true;
	constexpr bool bSaveContentPackages = true;
	constexpr bool bFastSave = false;
	constexpr bool bNotifyNoPackagesSaved = false;
	constexpr bool bCanBeDeclined = false;

	return FEditorFileUtils::SaveDirtyPackages(bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave, bNotifyNoPackagesSaved, bCanBeDeclined);
}

void SDataAssetManagerWidget::UpdateComboButtonContent()
{
	if (AssetManagerWidgets.ComboButton.IsValid())
	{
		AssetManagerWidgets.ComboButton->SetMenuContent(CreateComboButtonContent());
	}
}

TArray<TSharedPtr<FAssetData>> SDataAssetManagerWidget::GetAssetListSelectedItem()	const
{
	TArray<TSharedPtr<FAssetData>> SelectedItems;
	AssetManagerWidgets.AssetListView->GetSelectedItems(SelectedItems);

	return SelectedItems;
}

bool SDataAssetManagerWidget::IsSelectedAssetValid(const FString& CustomMessage) const
{
	if (AssetManagerData.SelectedAsset.IsValid()) return true;

	const FString ErrorMsg = CustomMessage.IsEmpty()
		? FString::Printf(TEXT("%s Selected Asset is not valid"), ANSI_TO_TCHAR(__FUNCTION__))
		: CustomMessage;

	UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s"), *ErrorMsg);
	return false;
}
#pragma endregion DataAssetManagerInterface

#undef LOCTEXT_NAMESPACE
