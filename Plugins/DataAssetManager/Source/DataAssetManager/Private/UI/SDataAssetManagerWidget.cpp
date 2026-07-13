// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "UI/SDataAssetManagerWidget.h"
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
#include "Filters/SFilterSearchBox.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "ISettingsModule.h"
#include "ClassViewerFilter.h"
#include "Kismet2/SClassPickerDialog.h"
#include "MessageLogModule.h"
#include "ObjectTools.h"
#include "OutputLogModule.h"
#include "ToolMenus.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "WidgetDrawerConfig.h"
#include "Editor/ContentBrowser/Private/ContentBrowserSingleton.h"
#include "FunctionLibraries/DataAssetManagerFunctionLibrary.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UI/SFolderTreeWidget.h"
#include "Logging/DataAssetManagerLog.h"
#include "Customization/DetailsRootObjectCustomization.h"
#include "Customization/DataAssetDetailsExtensionHandler.h"
#include "Models/DataAssetListModel.h"
#include "Services/DataAssetManagerAssetService.h"
#include "Styling/SlateIconFinder.h"

#define LOCTEXT_NAMESPACE "SDataAssetManagerWidget"

DEFINE_LOG_CATEGORY(SDataAssetManagerWidgetLog);

namespace DataAssetManager
{
	inline constexpr float ItemHeigth = 24.0f;
	inline constexpr float DataAssetFontSize = 10.0f;
	inline constexpr float SearchBoxHideThreshold = 0.01f;
	inline constexpr float DefaultSplitterValueWhenVisible = 0.25f;
	inline constexpr float SplitterValueWhenHidden = 0.0f;
	inline constexpr float ExpireDuration = 3.0f;
	inline constexpr float MetaDataWindowWidth = 500.0f;
	inline constexpr float MetaDataWindowHeight = 250.0f;
	inline constexpr float RCFixedWidth = 30.0f;

	const FMargin SeparatorPadding = FMargin(5.f, 7.f);
}

namespace
{
	class FDataAssetClassParentFilter final : public IClassViewerFilter
	{
	public:
		EClassFlags DisallowedClassFlags = CLASS_None;
		TSet<const UClass*> AllowedChildrenOfClasses;

		virtual bool IsClassAllowed(
			const FClassViewerInitializationOptions&,
			const UClass* InClass,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return InClass
				&& !InClass->HasAnyClassFlags(DisallowedClassFlags)
				&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
		}

		virtual bool IsUnloadedClassAllowed(
			const FClassViewerInitializationOptions&,
			const TSharedRef<const IUnloadedBlueprintData>,
			TSharedRef<FClassViewerFilterFuncs>) override
		{
			return false;
		}
	};
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
		.OnSlotResized(SSplitter::FOnSlotResized::CreateLambda([this](float NewSize) { SplitterValue.Set(NewSize); }))
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
						.HeaderRow(GenerateHeaderRow())
				]

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
					SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
								.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
								.Padding(FMargin(8.0f, 4.0f))
								.Visibility_Lambda([this]()
									{
										return GetAssetListSelectedItem().Num() > 1 ? EVisibility::Visible : EVisibility::Collapsed;
									})
								[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.FillWidth(1.0f)
										.VAlign(VAlign_Center)
										[
											SNew(STextBlock)
												.Text_Lambda([this]()
													{
														return FText::Format(
															LOCTEXT("BulkEditSelectionLabel", "Bulk Edit: {0} assets"),
															FText::AsNumber(GetAssetListSelectedItem().Num()));
													})
										]

									+ SHorizontalBox::Slot()
										.AutoWidth()
										.VAlign(VAlign_Center)
										[
											SNew(SButton)
												.ButtonStyle(FAppStyle::Get(), "SimpleButton")
												.ContentPadding(FMargin(4.0f, 2.0f))
												.ToolTipText(LOCTEXT("OpenPropertyMatrixTooltip", "Open selected Data Assets in Property Matrix"))
												.IsEnabled(this, &SDataAssetManagerWidget::CanOpenSelectedAssetsInPropertyEditor)
												.OnClicked_Lambda([this]()
													{
														OpenSelectionInPropertyMatrix();
														return FReply::Handled();
													})
												[
													SNew(SHorizontalBox)
														+ SHorizontalBox::Slot()
														.AutoWidth()
														.VAlign(VAlign_Center)
														[
															SNew(SImage)
																.Image(FSlateIconFinder::FindIconBrushForClass(UDataAsset::StaticClass()))
														]

														+ SHorizontalBox::Slot()
														.AutoWidth()
														.VAlign(VAlign_Center)
														.Padding(4.0f, 0.0f, 0.0f, 0.0f)
														[
															SNew(STextBlock)
																.Text(LOCTEXT("OpenPropertyMatrixButton", "Property Matrix"))
														]
												]
										]
								]
						]

					+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							AssetManagerWidgets.DetailsView.ToSharedRef()
						]
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

	/* TOOLBAR BUTTONS SECTION */
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
				.Icon(FAppStyle::Get().GetBrush("MainFrame.SaveAll"))
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

	SelectFirstAssetIfAvailable();
#pragma endregion Slate

}

void SDataAssetManagerWidget::SelectFirstAssetIfAvailable()
{
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

void SDataAssetManagerWidget::CreateNewDataAsset()
{
	TSharedRef<SWindow> Window =
		SNew(SWindow)
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
								TSharedPtr<FDataAssetClassParentFilter> Filter = MakeShared<FDataAssetClassParentFilter>();
								Options.ClassFilters.Add(Filter.ToSharedRef());
								Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown;
								Filter->AllowedChildrenOfClasses.Add(UDataAsset::StaticClass());

								const FText TitleText = LOCTEXT("CreateDataAssetOptions", "Pick Class For Data Asset Instance");
								UClass* ChosenClass = nullptr;
								UClass* DataAssetClass = nullptr;
								if (SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UDataAsset::StaticClass()))
								{
									DataAssetClass = ChosenClass;
									if (CVarDebugDataAssetManager.GetValueOnAnyThread())
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
						.OnClicked_Lambda([Window]()
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
	const UObject* AssetObject{ AssetManagerData.SelectedAsset->GetAsset() };
	if (!IsValid(AssetObject))
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("Selected Asset Object is not valid "));
		return;
	}

	const UDataAsset* DataAsset = CastChecked<UDataAsset>(AssetObject);
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(DataAsset);
}

void SDataAssetManagerWidget::ToggleDataAssetListVisibility()
{
	bIsSlotVisible = !bIsSlotVisible;
	SplitterValue.Set(bIsSlotVisible ? DataAssetManager::DefaultSplitterValueWhenVisible : DataAssetManager::SplitterValueWhenHidden);
}

void SDataAssetManagerWidget::OpenAuditAsset()
{
	if (!IsSelectedAssetValid())
	{
		return;
	}

	TArray<FName> SelectedAssetPackageNames{};
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
	TArray<TSharedPtr<FAssetData>> AssetsToSave = GetAssetListSelectedItem();
	if (AssetsToSave.Num() == 0 && AssetManagerData.SelectedAsset.IsValid())
	{
		AssetsToSave.Add(AssetManagerData.SelectedAsset);
	}

	if (AssetsToSave.Num() == 0)
	{
		IsSelectedAssetValid();
		return;
	}

	const int32 SavedCount = FDataAssetManagerAssetService::SaveAssets(AssetsToSave);
	if (SavedCount > 0 && IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Log, TEXT("Saved %d selected Data Assets"), SavedCount);
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
	if (!IsSelectedAssetValid())
	{
		return;
	}

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(DataAssetManager::ModuleName::ContentBrowser);

	ContentBrowserModule.Get().SyncBrowserToAssets({ *AssetManagerData.SelectedAsset });
}

void SDataAssetManagerWidget::CopyToClipboard(bool bCopyPaths)
{
	if (!IsSelectedAssetValid())
	{
		return;
	}

	TArray<FAssetData> SelectedPackages{};
	SelectedPackages.Add(*AssetManagerData.SelectedAsset);

	SelectedPackages.Sort([](const FAssetData& One, const FAssetData& Two)
		{
			return One.PackagePath.Compare(Two.PackagePath) < 0;
		});

	const FString ClipboardText = FString::JoinBy(SelectedPackages, LINE_TERMINATOR,
		[bCopyPaths](const FAssetData& Item)
		{
			return DataAssetManager::BuildClipboardEntry(Item, bCopyPaths);
		});

	FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);
}

void SDataAssetManagerWidget::OpenReferenceViewer()
{
	if (!IsSelectedAssetValid())
	{
		return;
	}

	TArray<FAssetData> AssetDataArray{};
	AssetDataArray.Add(*AssetManagerData.SelectedAsset.Get());

	DataAssetManager::ProcessAssetData(AssetDataArray,
		[](const TArray<FAssetIdentifier>& AssetIdentifiers)
		{
			IAssetManagerEditorModule::Get().OpenReferenceViewerUI(AssetIdentifiers);
		});
}

void SDataAssetManagerWidget::OpenReferenceInspector()
{
	if (!IsSelectedAssetValid())
	{
		return;
	}

	TSharedRef<FDataAssetReferenceInspectionResult> InspectionResult =
		MakeShared<FDataAssetReferenceInspectionResult>(FDataAssetManagerAssetService::InspectReferences(AssetManagerData.SelectedAsset));

	const FText AssetName = InspectionResult->IsValidSourceAsset()
		? FText::FromName(InspectionResult->SourceAsset->AssetName)
		: LOCTEXT("ReferenceInspectorUnknownAsset", "Unknown Asset");

	const FText AssetPath = InspectionResult->IsValidSourceAsset()
		? FText::FromName(InspectionResult->SourceAsset->PackageName)
		: FText::GetEmpty();

	TSharedRef<SWindow> Window =
		SNew(SWindow)
		.Title(FText::Format(LOCTEXT("ReferenceInspectorWindowTitle", "Reference Inspector: {0}"), AssetName))
		.ClientSize(FVector2D(980.0f, 640.0f))
		.SupportsMaximize(true)
		.SupportsMinimize(false);

	Window->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
				.Padding(FMargin(8.0f, 6.0f))
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
										.Text(AssetName)
										.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 2.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
										.Text(AssetPath)
										.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(12.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text_Lambda([InspectionResult]()
									{
										return InspectionResult->IsPotentiallyUnused()
											? LOCTEXT("ReferenceInspectorPotentiallyUnused", "Potentially unused")
											: LOCTEXT("ReferenceInspectorHasReferencers", "Has referencers");
									})
								.ColorAndOpacity_Lambda([InspectionResult]()
									{
										return InspectionResult->IsPotentiallyUnused()
											? FSlateColor(FLinearColor(1.0f, 0.55f, 0.15f))
											: FSlateColor(FLinearColor(0.35f, 0.8f, 0.45f));
									})
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(12.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text_Lambda([InspectionResult]()
									{
										return FText::Format(
											LOCTEXT("ReferenceInspectorReferenceCount", "References: {0}"),
											FText::AsNumber(InspectionResult->References.Num()));
									})
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(12.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text_Lambda([InspectionResult]()
									{
										return FText::Format(
											LOCTEXT("ReferenceInspectorReferencedByCount", "Referenced by: {0}"),
											FText::AsNumber(InspectionResult->ReferencedBy.Num()));
									})
						]
				]
		]

	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0.0f, 6.0f)
		[
			SNew(SSplitter)
				.Orientation(EOrientation::Orient_Horizontal)
				+ SSplitter::Slot()
				.Value(0.5f)
				[
					BuildReferenceListSection(
						LOCTEXT("ReferenceInspectorReferencesTitle", "References"),
						LOCTEXT("ReferenceInspectorNoReferences", "No referenced assets found"),
						InspectionResult,
						false)
				]

			+ SSplitter::Slot()
				.Value(0.5f)
				[
					BuildReferenceListSection(
						LOCTEXT("ReferenceInspectorReferencedByTitle", "Referenced By"),
						LOCTEXT("ReferenceInspectorNoReferencers", "No assets reference this Data Asset"),
						InspectionResult,
						true)
				]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 8.0f)
		[
			SNew(STextBlock)
				.Text_Lambda([InspectionResult]()
					{
						return FText::Format(
							LOCTEXT("ReferenceInspectorUnresolvedPackages", "Unresolved package references: {0}"),
							FText::AsNumber(InspectionResult->UnresolvedPackages.Num()));
					})
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Visibility_Lambda([InspectionResult]()
					{
						return InspectionResult->UnresolvedPackages.Num() > 0
							? EVisibility::Visible
							: EVisibility::Collapsed;
					})
		]
		);

	FSlateApplication::Get().AddWindow(Window);
}

void SDataAssetManagerWidget::OpenDataAssetDiff()
{
	const TArray<TSharedPtr<FAssetData>> SelectedAssets = GetAssetListSelectedItem();
	if (SelectedAssets.Num() != 2)
	{
		FNotificationInfo Info(LOCTEXT("DataAssetDiffRequiresTwoAssets", "Select exactly two Data Assets to compare."));
		Info.ExpireDuration = DataAssetManager::ExpireDuration;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	TSharedRef<FDataAssetDiffResult> DiffResult = MakeShared<FDataAssetDiffResult>(FDataAssetManagerAssetService::DiffAssets(SelectedAssets[0], SelectedAssets[1]));

	if (!DiffResult->bComparable)
	{
		FNotificationInfo Info(DiffResult->IsEmptyErrorText()
			? LOCTEXT("DataAssetDiffFailed", "Could not compare the selected Data Assets.")
			: DiffResult->ErrorText);
		Info.ExpireDuration = DataAssetManager::ExpireDuration;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	const FText LeftName = DiffResult->IsValidLeftAsset()
		? FText::FromName(DiffResult->LeftAsset->AssetName)
		: LOCTEXT("DataAssetDiffUnknownLeft", "Unknown");
	const FText RightName = DiffResult->IsValidRightAsset()
		? FText::FromName(DiffResult->RightAsset->AssetName)
		: LOCTEXT("DataAssetDiffUnknownRight", "Unknown");
	const FText LeftPath = DiffResult->IsValidLeftAsset()
		? FText::FromName(DiffResult->LeftAsset->PackageName)
		: FText::GetEmpty();
	const FText RightPath = DiffResult->IsValidRightAsset()
		? FText::FromName(DiffResult->RightAsset->PackageName)
		: FText::GetEmpty();

	TSharedPtr<SListView<TSharedPtr<FDataAssetDiffEntry>>> DiffListView;
	TSharedRef<TWeakPtr<SListView<TSharedPtr<FDataAssetDiffEntry>>>> DiffListViewWeak =
		MakeShared<TWeakPtr<SListView<TSharedPtr<FDataAssetDiffEntry>>>>();
	TSharedRef<TFunction<void()>> RefreshDiff = MakeShared<TFunction<void()>>();
	*RefreshDiff = [DiffResult, DiffListViewWeak]()
		{
			FDataAssetDiffResult UpdatedDiff = FDataAssetManagerAssetService::DiffAssets(DiffResult->LeftAsset, DiffResult->RightAsset);
			DiffResult->LeftAsset = UpdatedDiff.LeftAsset;
			DiffResult->RightAsset = UpdatedDiff.RightAsset;
			DiffResult->Entries = MoveTemp(UpdatedDiff.Entries);
			DiffResult->bComparable = UpdatedDiff.bComparable;
			DiffResult->ErrorText = UpdatedDiff.ErrorText;

			if (const TSharedPtr<SListView<TSharedPtr<FDataAssetDiffEntry>>> PinnedListView = DiffListViewWeak->Pin())
			{
				PinnedListView->RequestListRefresh();
			}
		};

	TSharedRef<SListView<TSharedPtr<FDataAssetDiffEntry>>> DiffListWidget =
		SAssignNew(DiffListView, SListView<TSharedPtr<FDataAssetDiffEntry>>)
		.ListItemsSource(&DiffResult->Entries)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow_Lambda([this, DiffResult, RefreshDiff](TSharedPtr<FDataAssetDiffEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
			{
				return GenerateDataAssetDiffRow(Entry, OwnerTable, DiffResult, RefreshDiff);
			});
	*DiffListViewWeak = DiffListView;

	TSharedRef<SWindow> Window =
		SNew(SWindow)
		.Title(FText::Format(LOCTEXT("DataAssetDiffWindowTitle", "DataAsset Diff: {0} vs {1}"), LeftName, RightName))
		.ClientSize(FVector2D(1080.0f, 680.0f))
		.SupportsMaximize(true)
		.SupportsMinimize(false);

	Window->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
				.Padding(FMargin(8.0f, 6.0f))
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(0.5f)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
										.Text(LeftName)
										.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 2.0f, 8.0f, 0.0f)
								[
									SNew(STextBlock)
										.Text(LeftPath)
										.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(10.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text_Lambda([DiffResult]()
									{
										return FText::Format(
											LOCTEXT("DataAssetDiffCount", "{0} difference(s)"),
											FText::AsNumber(DiffResult->Entries.Num()));
									})
						]

					+ SHorizontalBox::Slot()
						.FillWidth(0.5f)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
										.Text(RightName)
										.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
								]

								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(8.0f, 2.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
										.Text(RightPath)
										.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
						]
				]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.22f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("DataAssetDiffPropertyColumn", "Property"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.Padding(8.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("DataAssetDiffLeftColumn", "Left"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("DataAssetDiffRightColumn", "Right"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("DataAssetDiffCopyColumn", "Copy"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
		]

	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(8.0f, 0.0f, 8.0f, 8.0f)
		[
			SNew(SOverlay)
				+ SOverlay::Slot()
				[
					DiffListWidget
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("DataAssetDiffNoDifferences", "No editable property differences found"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Visibility_Lambda([DiffResult]()
							{
								return DiffResult->HasDifferences()
									? EVisibility::Collapsed
									: EVisibility::HitTestInvisible;
							})
				]
		]
		);

	FSlateApplication::Get().AddWindow(Window);
}

bool SDataAssetManagerWidget::CanOpenDataAssetDiff() const
{
	return GetAssetListSelectedItem().Num() == 2;
}

void SDataAssetManagerWidget::OpenSizeMap()
{
	if (!IsSelectedAssetValid())
	{
		return;
	}

	TArray<FAssetData> AssetDataArray{};
	AssetDataArray.Add(*AssetManagerData.SelectedAsset.Get());

	DataAssetManager::ProcessAssetData(AssetDataArray,
		[](const TArray<FAssetIdentifier>& AssetIdentifiers)
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
	FMessageLogModule& MessageLogModule{ FModuleManager::LoadModuleChecked<FMessageLogModule>(DataAssetManager::ModuleName::MessageLog) };
	MessageLogModule.OpenMessageLog("AssetCheck");
}

void SDataAssetManagerWidget::OpenOutputLogWindow()
{
	FOutputLogModule& OutputLogModule{ FModuleManager::LoadModuleChecked<FOutputLogModule>(DataAssetManager::ModuleName::OutputLog) };
	OutputLogModule.OpenOutputLog();
}

bool SDataAssetManagerWidget::CanRename() const
{
	return EditableWidgets.bCanRename;
}

void SDataAssetManagerWidget::HandleAssetDoubleClick(const FGeometry& InGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OpenSelectedDataAssetInEditor();
	}
}

FText SDataAssetManagerWidget::GetSelectedTextBlockInfo() const
{
	const FString SelectedStrItems{ GetAssetListSelectedItem().Num() > 0 ? FString::Printf(TEXT("(%d selected)"), GetAssetListSelectedItem().Num()) :
																		   TEXT("") };

	return FText::FromString(FString::Printf(TEXT("   %d items %s"), AssetManagerData.FilteredDataAssets.Num(), *SelectedStrItems));
}

bool SDataAssetManagerWidget::IsDetailsViewEmpty() const
{
	return AssetManagerWidgets.DetailsView->GetSelectedObjects().Num() == 0;
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
		UObject* const Asset{ AssetManagerData.SelectedAsset->GetAsset() };
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
			EditableWidgets.bRenamedProgress = false;
			if (IConsoleManager::Get().FindConsoleVariable(TEXT("ShowDebugDataAssetManager"))->GetBool())
			{
				UE_LOG(SDataAssetManagerWidgetLog, Log, TEXT("Asset renamed %s"), *Asset->GetName());
			}
		}
	}
}

ESelectionMode::Type SDataAssetManagerWidget::GetAssetListSelectionMode() const
{
	return EditableWidgets.bRenamedProgress ? ESelectionMode::Single : ESelectionMode::Multi;
}

EVisibility SDataAssetManagerWidget::GetVisibilitySearchBox() const
{
	return SplitterValue.Get() < DataAssetManager::SearchBoxHideThreshold ? EVisibility::Hidden : EVisibility::Visible;
}

void SDataAssetManagerWidget::SubscribeToAssetRegistryEvent()
{
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);
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
	AssetManagerWidgets.DetailsView->SetExtensionHandler(MakeShared<FDataAssetDetailsExtensionHandler>());
}

FDetailsViewArgs SDataAssetManagerWidget::CreateDetailsViewArgs() const
{
	FDetailsViewArgs Args{};
	Args.bHideSelectionTip = true;
	Args.bShowObjectLabel = false;
	Args.bCustomNameAreaLocation = false;
	Args.bAllowMultipleTopLevelObjects = true;
	Args.bShowOptions = true;
	Args.bShowPropertyMatrixButton = false;
	Args.bShowDifferingPropertiesOption = true;
	return Args;
}

TSharedPtr<SLayeredImage> SDataAssetManagerWidget::CreateFilterImage()
{
	return SNew(SLayeredImage)
		.Image(FAppStyle::Get().GetBrush("Icons.Filter"))
		.ColorAndOpacity(FSlateColor(FColor::White))
		.Visibility_Lambda(
			[this]()
			{
				return SplitterValue.Get() < 0.05f ? EVisibility::Hidden : EVisibility::Visible;
			});
}

void SDataAssetManagerWidget::CreateContextMenuFromDataAsset(const FGeometry& InGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		MenuBuilder.BeginSection("AssetActions", LOCTEXT("AssetActionsSection", "Asset"));
		MenuBuilder.AddMenuEntry(LOCTEXT("OpenInPropertyMatrixMenuEntry", "Open in Property Matrix"),
			LOCTEXT("OpenInPropertyMatrixMenuTooltip", "Open the selected DataAsset(s) in the Property Matrix panel"),
			FSlateIcon(FSlateIconFinder::FindIcon("PropertyEditor.Grid.TabIcon")),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::OpenSelectionInPropertyMatrix),
				FCanExecuteAction::CreateLambda([this]() { return CanOpenSelectedAssetsInPropertyEditor(); })));

		MenuBuilder.AddMenuEntry(LOCTEXT("ReferenceInspectorMenuEntry", "Reference Inspector"),
			LOCTEXT("ReferenceInspectorMenuTooltip", "Inspect assets referenced by this Data Asset and assets that reference it"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::OpenReferenceInspector),
				FCanExecuteAction::CreateSP(this, &SDataAssetManagerWidget::HasSelectedAssets)));

		MenuBuilder.AddMenuEntry(LOCTEXT("DataAssetDiffMenuEntry", "DataAsset Diff"),
			LOCTEXT("DataAssetDiffMenuTooltip", "Compare editable properties for two selected Data Assets of the same class"),
			FSlateIcon(FRevisionControlStyleManager::GetStyleSetName(), "RevisionControl.Actions.Diff"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::OpenDataAssetDiff),
				FCanExecuteAction::CreateSP(this, &SDataAssetManagerWidget::CanOpenDataAssetDiff)));

		MenuBuilder.AddMenuEntry(LOCTEXT("RenameMenuEntry", "Rename         "),
			LOCTEXT("RenameMenuTooltip", "Rename this item"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Rename"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::FocusOnSelectedAsset),
				FCanExecuteAction::CreateLambda([this]() { return CanRename(); })));

		MenuBuilder.AddMenuEntry(LOCTEXT("DeleteMenuEntry", "Delete         "),
			LOCTEXT("DeleteMenuTooltip", "Delete this item"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Delete"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::DeleteDataAsset)));
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection("BulkOperations", LOCTEXT("BulkOperationsSection", "Bulk Operations"));
		MenuBuilder.AddMenuEntry(LOCTEXT("BulkSaveSelectedMenuEntry", "Save Selected"),
			LOCTEXT("BulkSaveSelectedMenuTooltip", "Save all selected Data Assets"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::SaveDataAsset),
				FCanExecuteAction::CreateSP(this, &SDataAssetManagerWidget::HasSelectedAssets)));

		MenuBuilder.AddMenuEntry(LOCTEXT("BulkDuplicateSelectedMenuEntry", "Duplicate Selected"),
			LOCTEXT("BulkDuplicateSelectedMenuTooltip", "Duplicate all selected Data Assets next to the original assets"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Duplicate"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::DuplicateSelectedDataAssets),
				FCanExecuteAction::CreateSP(this, &SDataAssetManagerWidget::HasSelectedAssets)));

		MenuBuilder.AddMenuEntry(LOCTEXT("BulkMoveSelectedMenuEntry", "Move Selected..."),
			LOCTEXT("BulkMoveSelectedMenuTooltip", "Move all selected Data Assets to another content folder"),
			FSlateIcon(FSlateIconFinder::FindIcon("ContentBrowser.AddContent")),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::MoveSelectedDataAssets),
				FCanExecuteAction::CreateSP(this, &SDataAssetManagerWidget::HasSelectedAssets)));

		MenuBuilder.AddMenuEntry(LOCTEXT("BulkValidateSelectedMenuEntry", "Validate Selected"),
			LOCTEXT("BulkValidateSelectedMenuTooltip", "Validate all selected Data Assets and show results in Message Log"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::ValidateSelectedDataAssets),
				FCanExecuteAction::CreateSP(this, &SDataAssetManagerWidget::HasSelectedAssets)));

		MenuBuilder.AddMenuEntry(LOCTEXT("BulkValidateAllMenuEntry", "Validate All"),
			LOCTEXT("BulkValidateAllMenuTooltip", "Validate all loaded Data Assets and show results in Message Log"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"),
			FUIAction(FExecuteAction::CreateSP(this, &SDataAssetManagerWidget::ValidateAllDataAssets)));
		MenuBuilder.EndSection();

		FSlateApplication::Get().PushMenu(AsShared(),
			FWidgetPath(),
			MenuBuilder.MakeWidget(),
			MouseEvent.GetScreenSpacePosition(),
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
	}
}

TSharedRef<SWidget> SDataAssetManagerWidget::CreateComboButtonContent()
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection*/ false, nullptr);

	MenuBuilder.BeginSection("ResetSection", FText::FromString("Actions"));
	{
		MenuBuilder.AddMenuEntry(FText::FromString("Reset Filters"),
			FText::FromString("Clear all selected type and plugin filters."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda(
				[this]()
				{
					AssetManagerData.ActiveFilters.Empty();
					AssetManagerData.ActivePluginFilters.Empty();
					AssetManagerData.InvalidAssetPackages.Empty();
					AssetManagerData.bShowModifiedOnly = false;
					AssetManagerData.bShowInvalidOnly = false;
					UpdateFilteredAssetList();
				})));
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("QuickFilters", FText::FromString("Quick Filters"));
	{
		MenuBuilder.AddMenuEntry(LOCTEXT("ShowModifiedOnlyFilter", "Modified Only"),
			LOCTEXT("ShowModifiedOnlyFilterTooltip", "Show only Data Assets with dirty packages"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"),
			FUIAction(FExecuteAction::CreateLambda(
				[this]()
				{
					AssetManagerData.bShowModifiedOnly = !AssetManagerData.bShowModifiedOnly;
					UpdateFilteredAssetList();
				}),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda(
					[this]()
					{
						return AssetManagerData.bShowModifiedOnly;
					})),
			NAME_None,
			EUserInterfaceActionType::ToggleButton);

		MenuBuilder.AddMenuEntry(LOCTEXT("ShowInvalidOnlyFilter", "Invalid Only"),
			LOCTEXT("ShowInvalidOnlyFilterTooltip", "Show only Data Assets that failed the latest validation pass"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"),
			FUIAction(FExecuteAction::CreateLambda(
				[this]()
				{
					AssetManagerData.bShowInvalidOnly = !AssetManagerData.bShowInvalidOnly;
					UpdateFilteredAssetList();
				}),
				FCanExecuteAction::CreateLambda(
					[this]()
					{
						return AssetManagerData.InvalidAssetPackages.Num() > 0;
					}),
				FIsActionChecked::CreateLambda(
					[this]()
					{
						return AssetManagerData.bShowInvalidOnly;
					})),
			NAME_None,
			EUserInterfaceActionType::ToggleButton);
	}
	MenuBuilder.EndSection();

	MenuBuilder.AddWidget(
		SNew(SBox)
		.Padding(FMargin(5.f, 7.f))
		[
			SNew(SSeparator)
		],
		FText::GetEmpty());

	MenuBuilder.BeginSection("TypeFilters", FText::FromString("Asset Types"));
	for (const TSharedPtr<FString>& FilterItem : ComboBoxAssetListItems)
	{
		AddToggleFilterMenuEntry(MenuBuilder, *FilterItem, AssetManagerData.ActiveFilters,
			[this]
			{
				UpdateFilteredAssetList();
			});
	}
	MenuBuilder.EndSection();

	MenuBuilder.AddWidget(
		SNew(SBox)
		.Padding(FMargin(5.f, 7.f))
		[
			SNew(SSeparator)
		],
		FText::GetEmpty());

	MenuBuilder.BeginSection("PluginFilters", FText::FromString("Plugins"));

	for (const TSharedPtr<FString>& PluginItem : PluginFilterListItems)
	{
		AddToggleFilterMenuEntry(MenuBuilder,
			*PluginItem,
			AssetManagerData.ActivePluginFilters,
			[this]
			{
				UpdateFilteredAssetList();
			});
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SDataAssetManagerWidget::AddToggleFilterMenuEntry(FMenuBuilder& MenuBuilder, const FString& FilterName, TSet<FString>& ActiveFilters, TFunction<void()> UpdateFunc)
{
	FUIAction Action(FExecuteAction::CreateLambda(
		[&ActiveFilters, FilterName, UpdateFunc]()
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
		FIsActionChecked::CreateLambda(
			[&ActiveFilters, FilterName]()
			{
				return ActiveFilters.Contains(FilterName);
			}));

	MenuBuilder.AddMenuEntry(FText::FromString(FilterName), FText::GetEmpty(), FSlateIcon(), Action, NAME_None, EUserInterfaceActionType::ToggleButton);
}

FReply SDataAssetManagerWidget::OnItemClicked(TSharedPtr<FString> SourceItem)
{
	SelectedAssetType = SourceItem;
	UpdateFilteredAssetList();
	if (AssetManagerWidgets.IsValidComboButton())
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

	TArray<FString> AssetDirectories{};
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

	TArray<FTopLevelAssetPath> IgnoredClassPaths{};
	IgnoredClassPaths.Reserve(PluginSettings->ExcludedScanAssetTypes.Num());
	for (const TSubclassOf<UDataAsset>& IgnoredClass : PluginSettings->ExcludedScanAssetTypes)
	{
		if (IgnoredClass)
		{
			IgnoredClassPaths.Add(IgnoredClass->GetClassPathName());
		}
	}

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);
	const IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetDataArray{};
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
		if (Algo::AnyOf(AssetDirectories,
			[&NormalizedAssetPath](const FString& Directory)
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
	AssetManagerData.DataAssets.Sort(
		[](const TSharedPtr<FAssetData>& A, const TSharedPtr<FAssetData>& B)
		{
			return A->AssetName.LexicalLess(B->AssetName);
		});
}

void SDataAssetManagerWidget::UpdateFilteredAssetList()
{
	const FString SearchString = SearchText.Get().ToString();
	FDataAssetListModel::ApplyFilters(AssetManagerData.DataAssets,
		SearchString,
		AssetManagerData.ActiveFilters,
		AssetManagerData.ActivePluginFilters,
		AssetManagerData.InvalidAssetPackages,
		AssetManagerData.bShowModifiedOnly,
		AssetManagerData.bShowInvalidOnly,
		AssetManagerData.FilteredDataAssets);

	if (AssetManagerWidgets.IsValidAssetListView())
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
	return SNew(SDataAssetTableRow, OwnerSTable)
		.Item(Item)
		.OnAssetRenamed(SharedThis(this), &SDataAssetManagerWidget::HandleAssetRename)
		.OnCreateContextMenu(SharedThis(this), &SDataAssetManagerWidget::CreateContextMenuFromDataAsset)
		.OnAssetDoubleClicked(SharedThis(this), &SDataAssetManagerWidget::HandleAssetDoubleClick)
		.OnRegisterEditableText(SharedThis(this), &SDataAssetManagerWidget::RegisterEditableText)
		.OnGetValidationState(SharedThis(this), &SDataAssetManagerWidget::GetValidationStateForAsset)
		.OnMouseButtonDown(SharedThis(this), &SDataAssetManagerWidget::HandleRowMouseButtonDown);
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
		UE_LOG(SDataAssetManagerWidgetLog, Display, TEXT("%s FilteredDataAssets: %i"), ANSI_TO_TCHAR(__FUNCTION__), AssetManagerData.FilteredDataAssets.Num());
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
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s Newly added asset '%s' not found in filtered list"), ANSI_TO_TCHAR(__FUNCTION__), *NewAssetData.PackageName.ToString());
		return;
	}

	if (const UObject* AssetObject = NewAssetPtr->GetAsset())
	{
		if (AssetObject->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
		{
			/** Retrieve resources that have not yet been loaded */
			/** Accessing such objects may result in crashes or UB.*/
			UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s: Asset '%s' is not fully loaded (flags: %X)"), ANSI_TO_TCHAR(__FUNCTION__), *AssetObject->GetName(), AssetObject->GetFlags());
			return;
		}
	}
	else
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s: Failed to load asset '%s'"), ANSI_TO_TCHAR(__FUNCTION__), *NewAssetData.PackageName.ToString());
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
		FMenuBuilder MenuBuilder{ true, nullptr };

		MenuBuilder.BeginSection("AdditionalActions", LOCTEXT("AdditionalActionsSection", "Additional Actions"));
		{
			MenuBuilder.AddMenuEntry(LOCTEXT("ToggleAllColumns", "Hide All Columns"),
				LOCTEXT("ToggleAllColumnsTooltip", "Hide or show all columns at once"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda(
					[this]()
					{
						ColumnData.ToggleAllColumnsVisibility();
						UpdateColumnVisibility();
					}),
					FCanExecuteAction(),
					FIsActionChecked::CreateLambda(
						[this]()
						{
							/* Checked if all columns are hidden */
							return ColumnData.AreAllColumnsHidden();
						})),
				NAME_None,
				EUserInterfaceActionType::ToggleButton);
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
			AddColumnMenuEntry(MenuBuilder,
				LOCTEXT("ValidationColumn", "Validation"),
				LOCTEXT("ValidationColumnTooltip", "Toggle the visibility of the Validation column"),
				&ColumnData.ColumnVisibility.bShowValidationColumn);
		}
		MenuBuilder.EndSection();

		FSlateApplication::Get().PushMenu(SharedThis(this),
			FWidgetPath(),
			MenuBuilder.MakeWidget(),
			MouseEvent.GetScreenSpacePosition(),
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SDataAssetManagerWidget::AddColumnToHeader(TSharedPtr<SHeaderRow> InHeaderRow, const FName& ColumnId, const FString& Label, const float FillWidth)
{
	InHeaderRow->AddColumn(
		SHeaderRow::FColumn::FArguments()
		.ColumnId(ColumnId)
		.DefaultLabel(FText::FromString(Label))
		.FillWidth(FillWidth)
		.HeaderContent()
		[
			SNew(SBorder)
				.BorderBackgroundColor(FSlateColor(FColor::Transparent))
				.OnMouseButtonDown(this, &SDataAssetManagerWidget::ColumnButtonClicked)
				[
					SNew(STextBlock).Text(FText::FromString(Label))
				]
		]
	);
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
		});
}

void SDataAssetManagerWidget::ToggleColumn(bool* bColumnPtr)
{
	check(bColumnPtr);
	*bColumnPtr = !(*bColumnPtr);
	UpdateColumnVisibility();
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

SHeaderRow::FColumn::FArguments SDataAssetManagerWidget::CreateRevisionControlColumn()
{
	TSharedRef<SLayeredImage> RevisionControlColumnIcon =
		SNew(SLayeredImage)
		.ColorAndOpacity(FSlateColor::UseForeground())
		.Image(FRevisionControlStyleManager::Get().GetBrush("RevisionControl.Icon"));

	RevisionControlColumnIcon->AddLayer(TAttribute<const FSlateBrush*>::CreateSP(this, &SDataAssetManagerWidget::GetRevisionControlColumnIconBadge));

	return SHeaderRow::Column(DataAssetListColumns::ColumnID_RC)
		.FixedWidth(StaticCast<TOptional<float>>(DataAssetManager::RCFixedWidth))
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
	if (!IsSelectedAssetValid())
	{
		return;
	}

	UE_LOG(SDataAssetManagerLog, Warning, TEXT("%s EditableTextWidgets counts %d"), ANSI_TO_TCHAR(__FUNCTION__), EditableWidgets.EditableTextWidgets.Num());

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

	const TPair<FName, FName> WidgetKey{ FoundAsset->PackagePath, FoundAsset->AssetName };

	if (TSharedPtr<SEditableText>* FoundWidget = EditableWidgets.EditableTextWidgets.Find(WidgetKey))
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

	if (SelectedItems.Num() > 1)
	{
		EditableWidgets.bCanRename = false;
	}
	else if (SelectedItems.Num() == 1)
	{
		EditableWidgets.bCanRename = true;
	}
}

void SDataAssetManagerWidget::OpenDetailViewPanelForAsset(TSharedPtr<FAssetData> SelectedItem)
{
	if (!SelectedItem.IsValid())
	{
		UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("Selected Item is not valid"));
		return;
	}

	UDataAsset* DataAsset = Cast<UDataAsset>(SelectedItem->GetAsset());
	if (!IsValid(DataAsset))
	{
		UE_LOG(SDataAssetManagerWidgetLog, Error, TEXT("Failed to cast SelectedItem to UDataAsset. The asset might be of a different type or invalid."));
		return;
	}
	const TArray<TSharedPtr<FAssetData>> SelectedAssets = GetAssetListSelectedItem();
	if (SelectedAssets.Num() == 1)
	{
		AssetManagerWidgets.DetailsView->SetObject(DataAsset);
	}
	else
	{
		TArray<TWeakObjectPtr<UObject>> ObjectsToView{};
		for (const TSharedPtr<FAssetData>& AssetData : SelectedAssets)
		{
			if (AssetData.IsValid())
			{
				if (UDataAsset* Asset = Cast<UDataAsset>(AssetData->GetAsset()))
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
	TArray<FAssetIdentifier> AssetIdentifiers{};

	/** Converts asset data to identifiers for reference viewer / size map / audit tools */
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(RefAssetData, AssetIdentifiers);
	ProcessFunction(AssetIdentifiers);
}

void SDataAssetManagerWidget::RefreshAssetList()
{
	LoadDataAssets(DataAssetManager::GetPluginSettings());
	UpdateFilteredAssetList();
	InitializeAssetTypeComboBox(AssetManagerData.DataAssets);
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

void SDataAssetManagerWidget::DeleteDataAsset()
{
	TArray<FAssetData> AssetsToDelete;
	EditableWidgets.bCanRename = false;
	for (const TSharedPtr<FAssetData>& AssetData : GetAssetListSelectedItem())
	{
		if (AssetData.IsValid())
		{
			AssetsToDelete.Add(*AssetData);
		}
	}

	if (AssetsToDelete.Num() > 0)
	{
		DataAssetManager::DeleteMultiplyAsset(AssetsToDelete);
	}
}

void SDataAssetManagerWidget::DuplicateSelectedDataAssets()
{
	const TArray<TSharedPtr<FAssetData>> SelectedAssets = GetAssetListSelectedItem();
	if (SelectedAssets.Num() == 0)
	{
		return;
	}

	TArray<FAssetData> DuplicatedAssets;
	const int32 DuplicatedCount = FDataAssetManagerAssetService::DuplicateAssets(SelectedAssets, &DuplicatedAssets);
	if (DuplicatedCount <= 0)
	{
		return;
	}

	RefreshAssetList();
	if (DuplicatedAssets.Num() > 0)
	{
		FocusOnNewlyAddedAsset(DuplicatedAssets[0]);
	}
}

void SDataAssetManagerWidget::MoveSelectedDataAssets()
{
	if (!HasSelectedAssets())
	{
		return;
	}

	TSharedRef<SWindow> Window =
		SNew(SWindow)
		.Title(LOCTEXT("MoveSelectedDataAssetsWindowTitle", "Select Destination Folder"))
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
						.Text(LOCTEXT("MoveSelectedDataAssetsButton", "Move"))
						.OnClicked_Lambda([this, FolderTreeWidget, Window]()
							{
								const FString DestinationPath = FolderTreeWidget.IsValid() ? FolderTreeWidget->GetSelectedDirectory() : FString();
								FSlateApplication::Get().RequestDestroyWindow(Window);
								if (!DestinationPath.IsEmpty())
								{
									MoveSelectedDataAssetsToPath(DestinationPath);
								}

								return FReply::Handled();
							})
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.f, 0.f)
				[
					SNew(SButton)
						.Text(LOCTEXT("MoveSelectedDataAssetsCancelButton", "Cancel"))
						.OnClicked_Lambda([Window]()
							{
								FSlateApplication::Get().RequestDestroyWindow(Window);
								return FReply::Handled();
							})
				]
		]
	);

	FSlateApplication::Get().AddWindow(Window);
}

void SDataAssetManagerWidget::MoveSelectedDataAssetsToPath(const FString& DestinationPath)
{
	const TArray<TSharedPtr<FAssetData>> SelectedAssets = GetAssetListSelectedItem();
	if (SelectedAssets.Num() == 0)
	{
		return;
	}

	TArray<FAssetData> MovedAssets;
	const int32 MovedCount = FDataAssetManagerAssetService::MoveAssets(SelectedAssets, DestinationPath, &MovedAssets);
	if (MovedCount <= 0)
	{
		return;
	}

	RefreshAssetList();
	if (MovedAssets.Num() > 0)
	{
		FocusOnNewlyAddedAsset(MovedAssets[0]);
	}
}

void SDataAssetManagerWidget::ValidateSelectedDataAssets()
{
	const TArray<TSharedPtr<FAssetData>> SelectedAssets = GetAssetListSelectedItem();
	if (SelectedAssets.Num() == 0)
	{
		return;
	}

	ApplyValidationResults(FDataAssetManagerAssetService::ValidateAssets(SelectedAssets, true));
}

void SDataAssetManagerWidget::ValidateAllDataAssets()
{
	ApplyValidationResults(FDataAssetManagerAssetService::ValidateAssets(AssetManagerData.DataAssets, true));
}

void SDataAssetManagerWidget::ApplyValidationResults(FDataAssetValidationResults&& ValidationResults)
{
	for (const TPair<FName, FDataAssetValidationState>& ValidationState : ValidationResults.StatesByPackage)
	{
		AssetManagerData.ValidationStates.Add(ValidationState.Key, ValidationState.Value);
	}

	AssetManagerData.InvalidAssetPackages = MoveTemp(ValidationResults.InvalidPackages);
	AssetManagerData.bShowInvalidOnly = AssetManagerData.InvalidAssetPackages.Num() > 0;
	UpdateFilteredAssetList();
	UpdateComboButtonContent();
}

const FDataAssetValidationState* SDataAssetManagerWidget::GetValidationStateForAsset(TSharedPtr<FAssetData> AssetData) const
{
	return AssetData.IsValid()
		? AssetManagerData.ValidationStates.Find(AssetData->PackageName)
		: nullptr;
}

bool SDataAssetManagerWidget::HasSelectedAssets() const
{
	return GetAssetListSelectedItem().Num() > 0;
}

// TODO!!! Original code copied from the FAssetFileContextMenu class
// AssetFileContextMenu::ExecuteShowAssetMetaData()
// (*minor refactoring of the original code *const fix added)
void SDataAssetManagerWidget::ShowAssetMetaData()
{
	for (const TSharedPtr<FAssetData>& AssetData : GetAssetListSelectedItem())
	{
		const UObject* Asset = AssetData.Get()->GetAsset();
		if (IsValid(Asset))
		{
			const TMap<FName, FString>* TagValues = UMetaData::GetMapForObject(Asset);
			if (TagValues)
			{
				const FString Title = FString::Printf(TEXT("Metadata: %s"), *AssetData.Get()->AssetName.ToString());

				TSharedPtr<SWindow> Window =
					SNew(SWindow)
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
}

TSharedRef<SWidget> SDataAssetManagerWidget::BuildReferenceListSection( FText Title, FText EmptyText, TSharedRef<FDataAssetReferenceInspectionResult> InspectionResult, bool bReferencedBy)
{
	const TArray<TSharedPtr<FDataAssetReferenceEntry>>* EntriesSource = bReferencedBy
		? &InspectionResult->ReferencedBy
		: &InspectionResult->References;

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(6.0f))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 6.0f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(Title)
								.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text_Lambda([InspectionResult, bReferencedBy]()
									{
										const int32 EntryCount = bReferencedBy
											? InspectionResult->ReferencedBy.Num()
											: InspectionResult->References.Num();
										return FText::AsNumber(EntryCount);
									})
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.0f, 0.0f, 52.0f, 4.0f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(0.32f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("ReferenceInspectorAssetColumn", "Asset"))
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]

						+ SHorizontalBox::Slot()
						.FillWidth(0.20f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("ReferenceInspectorClassColumn", "Class"))
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]

						+ SHorizontalBox::Slot()
						.FillWidth(0.48f)
						[
							SNew(STextBlock)
								.Text(LOCTEXT("ReferenceInspectorPathColumn", "Path"))
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
				]

			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SListView<TSharedPtr<FDataAssetReferenceEntry>>)
								.ListItemsSource(EntriesSource)
								.SelectionMode(ESelectionMode::Single)
								.OnGenerateRow_Lambda([this, InspectionResult](TSharedPtr<FDataAssetReferenceEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
									{
										return GenerateReferenceEntryRow(Entry, OwnerTable);
									})
								.OnMouseButtonDoubleClick_Lambda([this, InspectionResult](TSharedPtr<FDataAssetReferenceEntry> Entry)
									{
										OpenReferenceEntryInEditor(Entry);
									})
						]

					+ SOverlay::Slot()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(EmptyText)
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								.Visibility_Lambda([InspectionResult, bReferencedBy]()
									{
										const int32 EntryCount = bReferencedBy
											? InspectionResult->ReferencedBy.Num()
											: InspectionResult->References.Num();
										return EntryCount == 0
											? EVisibility::HitTestInvisible
											: EVisibility::Collapsed;
									})
						]
				]
		];
}

TSharedRef<ITableRow> SDataAssetManagerWidget::GenerateReferenceEntryRow( TSharedPtr<FDataAssetReferenceEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FText DisplayName = Entry.IsValid() ? Entry->GetDisplayName() : FText::GetEmpty();
	const FText ClassName = Entry.IsValid() ? Entry->GetClassDisplayName() : FText::GetEmpty();
	const FText PathText = Entry.IsValid() ? Entry->GetPathText() : FText::GetEmpty();
	const FText TooltipText = Entry.IsValid() ? FText::FromString(Entry->GetSortKey()) : FText::GetEmpty();

	return SNew(STableRow<TSharedPtr<FDataAssetReferenceEntry>>, OwnerTable)
		.Padding(FMargin(2.0f))
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.32f)
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(DisplayName)
						.ToolTipText(TooltipText)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.20f)
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(ClassName)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.ToolTipText(TooltipText)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(0.48f)
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(PathText)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.ToolTipText(TooltipText)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(FMargin(2.0f))
						.ToolTipText(LOCTEXT("ReferenceInspectorSyncTooltip", "Find in Content Browser"))
						.IsEnabled_Lambda([Entry]()
							{
								return Entry.IsValid() && Entry->bAssetDataResolved;
							})
						.OnClicked_Lambda([this, Entry]()
							{
								SyncReferenceEntryInContentBrowser(Entry);
								return FReply::Handled();
							})
						[
							SNew(SImage)
								.Image(FAppStyle::GetBrush("Icons.Search"))
						]
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(FMargin(2.0f))
						.ToolTipText(LOCTEXT("ReferenceInspectorOpenTooltip", "Open asset"))
						.IsEnabled_Lambda([Entry]()
							{
								return Entry.IsValid() && Entry->bAssetDataResolved;
							})
						.OnClicked_Lambda([this, Entry]()
							{
								OpenReferenceEntryInEditor(Entry);
								return FReply::Handled();
							})
						[
							SNew(SImage)
								.Image(FAppStyle::GetBrush("Icons.Edit"))
						]
				]
		];
}

TSharedRef<ITableRow> SDataAssetManagerWidget::GenerateDataAssetDiffRow(TSharedPtr<FDataAssetDiffEntry> Entry, const TSharedRef<STableViewBase>& OwnerTable, TSharedRef<FDataAssetDiffResult> DiffResult, TSharedRef<TFunction<void()>> RefreshDiff)
{
	const FText PropertyName = Entry.IsValid() ? Entry->DisplayName : FText::GetEmpty();
	const FText PropertyDetails = Entry.IsValid()
		? FText::FromString(Entry->Category.IsEmpty()
			? Entry->PropertyName.ToString()
			: FString::Printf(TEXT("%s / %s"), *Entry->Category, *Entry->PropertyName.ToString()))
		: FText::GetEmpty();
	const FText LeftValue = Entry.IsValid() && !Entry->LeftValue.IsEmpty()
		? FText::FromString(Entry->LeftValue)
		: LOCTEXT("DataAssetDiffEmptyLeftValue", "<Empty>");
	const FText RightValue = Entry.IsValid() && !Entry->RightValue.IsEmpty()
		? FText::FromString(Entry->RightValue)
		: LOCTEXT("DataAssetDiffEmptyRightValue", "<Empty>");

	return SNew(STableRow<TSharedPtr<FDataAssetDiffEntry>>, OwnerTable)
		.Padding(FMargin(2.0f, 4.0f))
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.22f)
				.VAlign(VAlign_Top)
				.Padding(2.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
								.Text(PropertyName)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(PropertyDetails)
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
				]

			+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Top)
				.Padding(8.0f, 0.0f)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(FMargin(6.0f, 4.0f))
						[
							SNew(STextBlock)
								.Text(LeftValue)
								.AutoWrapText(true)
								.ToolTipText(LeftValue)
						]
				]

			+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Top)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(FMargin(6.0f, 4.0f))
						[
							SNew(STextBlock)
								.Text(RightValue)
								.AutoWrapText(true)
								.ToolTipText(RightValue)
						]
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.ContentPadding(FMargin(6.0f, 2.0f))
								.ToolTipText(LOCTEXT("DataAssetDiffCopyLeftToRightTooltip", "Copy the left value to the right Data Asset"))
								.OnClicked_Lambda([this, Entry, DiffResult, RefreshDiff]()
									{
										CopyDataAssetDiffValue(Entry, DiffResult, true, RefreshDiff);
										return FReply::Handled();
									})
								[
									SNew(STextBlock)
										.Text(LOCTEXT("DataAssetDiffCopyLeftToRight", "L -> R"))
								]
						]

					+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.ContentPadding(FMargin(6.0f, 2.0f))
								.ToolTipText(LOCTEXT("DataAssetDiffCopyRightToLeftTooltip", "Copy the right value to the left Data Asset"))
								.OnClicked_Lambda([this, Entry, DiffResult, RefreshDiff]()
									{
										CopyDataAssetDiffValue(Entry, DiffResult, false, RefreshDiff);
										return FReply::Handled();
									})
								[
									SNew(STextBlock)
										.Text(LOCTEXT("DataAssetDiffCopyRightToLeft", "L <- R"))
								]
						]
				]
		];
}

void SDataAssetManagerWidget::CopyDataAssetDiffValue(TSharedPtr<FDataAssetDiffEntry> Entry, TSharedRef<FDataAssetDiffResult> DiffResult, bool bLeftToRight, TSharedRef<TFunction<void()>> RefreshDiff)
{
	if (!Entry.IsValid())
	{
		return;
	}

	FText ErrorText;
	const TSharedPtr<FAssetData>& SourceAsset = bLeftToRight ? DiffResult->LeftAsset : DiffResult->RightAsset;
	const TSharedPtr<FAssetData>& TargetAsset = bLeftToRight ? DiffResult->RightAsset : DiffResult->LeftAsset;
	if (!FDataAssetManagerAssetService::CopyDiffPropertyValue(SourceAsset, TargetAsset, Entry->PropertyName, &ErrorText))
	{
		FNotificationInfo Info(ErrorText.IsEmpty()
			? LOCTEXT("DataAssetDiffCopyFailed", "Failed to copy the property value.")
			: ErrorText);
		Info.ExpireDuration = DataAssetManager::ExpireDuration;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	(*RefreshDiff)();

	if (AssetManagerWidgets.IsValidDetailsView())
	{
		AssetManagerWidgets.DetailsView->ForceRefresh();
	}
}

void SDataAssetManagerWidget::SyncReferenceEntryInContentBrowser(TSharedPtr<FDataAssetReferenceEntry> Entry) const
{
	if (!Entry.IsValid() || !Entry->bAssetDataResolved)
	{
		return;
	}

	const FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(DataAssetManager::ModuleName::ContentBrowser);
	ContentBrowserModule.Get().SyncBrowserToAssets({ Entry->AssetData });
}

void SDataAssetManagerWidget::OpenReferenceEntryInEditor(TSharedPtr<FDataAssetReferenceEntry> Entry) const
{
	if (!Entry.IsValid() || !Entry->bAssetDataResolved)
	{
		return;
	}

	UObject* Asset = Entry->AssetData.GetAsset();
	if (IsValid(Asset))
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
	}
}

void SDataAssetManagerWidget::OpenSelectionInPropertyMatrix()
{
	const TArray<TSharedPtr<FAssetData>> SelectedItems = GetAssetListSelectedItem();
	TArray<UObject*> SelectedObjects;
	for (const TSharedPtr<FAssetData>& AssetDataPtr : SelectedItems)
	{
		if (AssetDataPtr.IsValid())
		{
			UObject* Asset = AssetDataPtr->GetAsset();
			if (Asset)
			{
				SelectedObjects.Add(Asset);
			}
		}
	}

	if (SelectedObjects.Num() > 0)
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(DataAssetManager::ModuleName::PropertyEditor);
		PropertyEditorModule.CreatePropertyEditorToolkit(TSharedPtr<IToolkitHost>(), SelectedObjects);
	}
}

bool SDataAssetManagerWidget::CanOpenSelectedAssetsInPropertyEditor() const
{
	const FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(DataAssetManager::ModuleName::PropertyEditor);
	if (!PropertyEditorModule.GetCanUsePropertyMatrix())
	{
		return false;
	}

	TArray<TSharedPtr<FAssetData>> SelectedItems = GetAssetListSelectedItem();
	return SelectedItems.Num() > 0;
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
	if (AssetManagerWidgets.IsValidComboButton())
	{
		AssetManagerWidgets.ComboButton->SetMenuContent(CreateComboButtonContent());
	}
}

TArray<TSharedPtr<FAssetData>> SDataAssetManagerWidget::GetAssetListSelectedItem() const
{
	TArray<TSharedPtr<FAssetData>> SelectedItems{};
	if (AssetManagerWidgets.IsValidAssetListView())
	{
		AssetManagerWidgets.AssetListView->GetSelectedItems(SelectedItems);
	}

	return SelectedItems;
}

bool SDataAssetManagerWidget::IsSelectedAssetValid(const FString& CustomMessage) const
{
	if (AssetManagerData.IsValidSelectedAsset())
	{
		return true;
	}

	const FString ErrorMsg = CustomMessage.IsEmpty() ? FString::Printf(TEXT("%hs Selected Asset is not valid"), __FUNCTION__) : CustomMessage;

	UE_LOG(SDataAssetManagerWidgetLog, Warning, TEXT("%s"), *ErrorMsg);
	return false;
}

#undef LOCTEXT_NAMESPACE
