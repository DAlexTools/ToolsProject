// Copyright Epic Games, Inc. All Rights Reserved.

#include "UNotepad.h"
#include "UI/SNotepadWidget.h"

#include "AssetRegistry/AssetData.h"
#include "ContentBrowserItem.h"
#include "ContentBrowserMenuContexts.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "SourceCodeNavigation.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "UObject/Class.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FUNotepadModule"

const FName FUNotepadModule::UNotepadTabName = FName("UNotepad");

void FUNotepadModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		UNotepadTabName,
		FOnSpawnTab::CreateRaw(this, &FUNotepadModule::CreateNotepadManagerTab))
		.SetDisplayName(LOCTEXT("UNotepadTabTitle", "UNotepad"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUNotepadModule::RegisterMenus));
}

void FUNotepadModule::ShutdownModule()
{
	if (FGlobalTabmanager::Get()->HasTabSpawner(UNotepadTabName))
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UNotepadTabName);
	}

	UToolMenus::UnRegisterStartupCallback(this);
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::Get()->UnregisterOwner(this);
	}
}

void FUNotepadModule::OpenManagerTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(UNotepadTabName);
}

void FUNotepadModule::OpenFileInNotepad(const FString& FilePath)
{
	OpenManagerTab();

	if (TSharedPtr<SNotepadWidget> NotepadWidget = ActiveNotepadWidget.Pin())
	{
		NotepadWidget->OpenFile(FilePath);
	}
}

void FUNotepadModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	const FUIAction OpenNotepadAction(FExecuteAction::CreateRaw(this, &FUNotepadModule::OpenManagerTab));
	const FSlateIcon NotepadIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit");

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& MenuSection = Menu->FindOrAddSection("Tools");
	MenuSection.AddMenuEntry(
		"OpenUNotepad",
		LOCTEXT("OpenUNotepad", "UNotepad"),
		LOCTEXT("OpenUNotepadTooltip", "Open UNotepad tab"),
		NotepadIcon,
		OpenNotepadAction);

	UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
	FToolMenuSection& ToolbarSection = Toolbar->FindOrAddSection("UNotepad");
	ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
		"OpenUNotepadToolbar",
		OpenNotepadAction,
		LOCTEXT("OpenUNotepadToolbar", "UNotepad"),
		LOCTEXT("OpenUNotepadToolbarTooltip", "Open UNotepad tab"),
		NotepadIcon));

	UToolMenu* ContentBrowserAssetMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu");
	FToolMenuSection& ContentBrowserSection = ContentBrowserAssetMenu->FindOrAddSection("GetAssetActions");
	ContentBrowserSection.AddDynamicEntry(
		"OpenSourceFilesInUNotepad",
		FNewToolMenuSectionDelegate::CreateRaw(this, &FUNotepadModule::AddContentBrowserOpenEntries));
}

TSharedRef<SDockTab> FUNotepadModule::CreateNotepadManagerTab(const FSpawnTabArgs& Args)
{
	TSharedPtr<SNotepadWidget> NotepadWidget;
	TSharedRef<SDockTab> NotepadTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("UNotepadTabLabel", "UNotepad"))
		[
			SAssignNew(NotepadWidget, SNotepadWidget)
		];

	ActiveNotepadWidget = NotepadWidget;
	return NotepadTab;
}

void FUNotepadModule::AddContentBrowserOpenEntries(FToolMenuSection& Section)
{
	TArray<FString> FilePaths = GetSupportedContentBrowserFilePaths(Section);
	if (FilePaths.IsEmpty())
	{
		return;
	}

	Section.AddMenuEntry(
		"OpenSelectedSourceFilesInUNotepad",
		LOCTEXT("OpenSelectedSourceFilesInUNotepad", "Открыть в UNotepad"),
		LOCTEXT("OpenSelectedSourceFilesInUNotepadTooltip", "Открыть выбранные исходные файлы в UNotepad."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
		FUIAction(FExecuteAction::CreateLambda([this, FilePaths]()
		{
			OpenContentBrowserSelectionInNotepad(FilePaths);
		})));
}

void FUNotepadModule::OpenContentBrowserSelectionInNotepad(TArray<FString> FilePaths)
{
	OpenManagerTab();

	TSharedPtr<SNotepadWidget> NotepadWidget = ActiveNotepadWidget.Pin();
	if (!NotepadWidget.IsValid())
	{
		return;
	}

	for (const FString& FilePath : FilePaths)
	{
		NotepadWidget->OpenFile(FilePath);
	}
}

TArray<FString> FUNotepadModule::GetSupportedContentBrowserFilePaths(const FToolMenuSection& Section) const
{
	TArray<FString> FilePaths;

	const auto AddSupportedFilePath = [this, &FilePaths](FString FilePath)
	{
		if (FilePath.IsEmpty())
		{
			return;
		}

		FilePath = FPaths::ConvertRelativePathToFull(FilePath);
		FPaths::NormalizeFilename(FilePath);

		if (IsSupportedSourceFilePath(FilePath) && IFileManager::Get().FileExists(*FilePath))
		{
			FilePaths.AddUnique(FilePath);
		}
	};

	const auto AddClassSourcePaths = [&AddSupportedFilePath](const UClass* SourceClass)
	{
		if (!SourceClass)
		{
			return;
		}

		FString HeaderPath;
		if (FSourceCodeNavigation::FindClassHeaderPath(SourceClass, HeaderPath))
		{
			AddSupportedFilePath(HeaderPath);
		}

		FString SourcePath;
		if (FSourceCodeNavigation::FindClassSourcePath(SourceClass, SourcePath))
		{
			AddSupportedFilePath(SourcePath);
		}
	};

	const auto AddAssetSourcePaths = [&AddClassSourcePaths](const FAssetData& AssetData)
	{
		const UClass* AssetClass = AssetData.GetClass();
		if (!AssetClass || !AssetClass->IsChildOf<UClass>())
		{
			return;
		}

		UClass* SourceClass = Cast<UClass>(AssetData.FastGetAsset(false));
		if (!SourceClass)
		{
			SourceClass = Cast<UClass>(AssetData.GetAsset());
		}

		AddClassSourcePaths(SourceClass);
	};

	const UContentBrowserAssetContextMenuContext* Context = Section.FindContext<UContentBrowserAssetContextMenuContext>();
	if (!Context)
	{
		return FilePaths;
	}

	for (const FContentBrowserItem& SelectedItem : Context->GetSelectedItems())
	{
		FString PhysicalPath;
		if (SelectedItem.IsFile() && SelectedItem.GetItemPhysicalPath(PhysicalPath))
		{
			AddSupportedFilePath(PhysicalPath);
		}

		FAssetData ItemAssetData;
		if (SelectedItem.Legacy_TryGetAssetData(ItemAssetData))
		{
			AddAssetSourcePaths(ItemAssetData);
		}
	}

	for (const FAssetData& SelectedAsset : Context->SelectedAssets)
	{
		AddAssetSourcePaths(SelectedAsset);
	}

	return FilePaths;
}

bool FUNotepadModule::IsSupportedSourceFilePath(const FString& FilePath)
{
	const FString Extension = FPaths::GetExtension(FilePath).ToLower();
	return Extension == TEXT("h")
		|| Extension == TEXT("hh")
		|| Extension == TEXT("hpp")
		|| Extension == TEXT("hxx")
		|| Extension == TEXT("inl")
		|| Extension == TEXT("ipp")
		|| Extension == TEXT("cpp")
		|| Extension == TEXT("cc")
		|| Extension == TEXT("cxx")
		|| Extension == TEXT("cs");
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUNotepadModule, UNotepad)
