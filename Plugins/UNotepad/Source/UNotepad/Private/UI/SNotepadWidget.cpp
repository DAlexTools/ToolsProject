// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SNotepadWidget.h"

#include "DesktopPlatformModule.h"
#if WITH_LIVE_CODING
#include "ILiveCodingModule.h"
#endif
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandList.h"
#include "HAL/FileManager.h"
#include "IDesktopPlatform.h"
#include "ISettingsModule.h"
#include "InputCoreTypes.h"
#include "Misc/HotReloadInterface.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Settings/UNotepadSettings.h"
#include "Services/NotepadDocumentUtils.h"
#include "Services/NotepadDocumentHistoryService.h"
#include "Services/NotepadFileService.h"
#include "Services/NotepadFormatService.h"
#include "Services/NotepadTextActionService.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "UI/SNotepadEditor.h"
#include "UI/SNotepadTabStrip.h"
#include "UI/SNotepadToolbar.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"

#define LOCTEXT_NAMESPACE "UNotepadWidget"

class FUNotepadEditorCommands final : public TCommands<FUNotepadEditorCommands>
{
public:
	FUNotepadEditorCommands()
		: TCommands<FUNotepadEditorCommands>(
			TEXT("UNotepadEditor"),
			LOCTEXT("UNotepadEditorCommands", "UNotepad Editor"),
			NAME_None,
			FAppStyle::GetAppStyleSetName())
	{
	}

	virtual void RegisterCommands() override
	{
		UI_COMMAND(NewDocument, "New", "Create a new text tab", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::N));
		UI_COMMAND(OpenDocument, "Open", "Open one or more files", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::O));
		UI_COMMAND(SaveDocument, "Save", "Save the active tab", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::S));
		UI_COMMAND(SaveDocumentAs, "Save As", "Save the active tab to another file", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::S));
		UI_COMMAND(CloseDocument, "Close Tab", "Close the active tab", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::W));
		UI_COMMAND(ShowFindReplace, "Find / Replace", "Show the find and replace panel", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
		UI_COMMAND(ShowReplace, "Replace", "Show the find and replace panel", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::H));
		UI_COMMAND(FindNext, "Find Next", "Find the next match", EUserInterfaceActionType::Button, FInputChord(EKeys::F3));
		UI_COMMAND(FindPrevious, "Find Previous", "Find the previous match", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::F3));
		UI_COMMAND(GoToLine, "Go To Line", "Go to a line in the active document", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::G));
		UI_COMMAND(Undo, "Undo", "Undo the last document text change", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Z));
		UI_COMMAND(Redo, "Redo", "Redo the last undone document text change", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Y));
		UI_COMMAND(ToggleLineComment, "Toggle Comment", "Comment or uncomment the selected code lines", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Slash));
		UI_COMMAND(DuplicateLineOrSelection, "Duplicate Line / Selection", "Duplicate the current line or selected text", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::D));
		UI_COMMAND(MoveLineUp, "Move Line Up", "Move the current line up", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::Up));
		UI_COMMAND(MoveLineDown, "Move Line Down", "Move the current line down", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::Down));
		UI_COMMAND(TrimTrailingWhitespace, "Trim Trailing Whitespace", "Remove trailing spaces and tabs from every line", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(ConvertTabsToSpaces, "Convert Tabs to Spaces", "Convert tab characters to spaces using the configured tab size", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(ConvertSpacesToTabs, "Convert Leading Spaces to Tabs", "Convert leading indentation spaces to tabs using the configured tab size", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(EnsureFinalNewline, "Ensure Newline at EOF", "Append a final newline if the document does not have one", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(NormalizeLineEndingsLf, "Line Endings: LF", "Convert document line endings to LF", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(NormalizeLineEndingsCrlf, "Line Endings: CRLF", "Convert document line endings to CRLF", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(OpenHeaderSourcePair, "Open Header/Source Pair", "Open the matching header or source file", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::O));
		UI_COMMAND(CompileProject, "Compile", "Compile the current Unreal project code", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::B));
		UI_COMMAND(OpenSettings, "Settings...", "Open UNotepad project settings", EUserInterfaceActionType::Button, FInputChord());
	}

	TSharedPtr<FUICommandInfo> NewDocument;
	TSharedPtr<FUICommandInfo> OpenDocument;
	TSharedPtr<FUICommandInfo> SaveDocument;
	TSharedPtr<FUICommandInfo> SaveDocumentAs;
	TSharedPtr<FUICommandInfo> CloseDocument;
	TSharedPtr<FUICommandInfo> ShowFindReplace;
	TSharedPtr<FUICommandInfo> ShowReplace;
	TSharedPtr<FUICommandInfo> FindNext;
	TSharedPtr<FUICommandInfo> FindPrevious;
	TSharedPtr<FUICommandInfo> GoToLine;
	TSharedPtr<FUICommandInfo> Undo;
	TSharedPtr<FUICommandInfo> Redo;
	TSharedPtr<FUICommandInfo> ToggleLineComment;
	TSharedPtr<FUICommandInfo> DuplicateLineOrSelection;
	TSharedPtr<FUICommandInfo> MoveLineUp;
	TSharedPtr<FUICommandInfo> MoveLineDown;
	TSharedPtr<FUICommandInfo> TrimTrailingWhitespace;
	TSharedPtr<FUICommandInfo> ConvertTabsToSpaces;
	TSharedPtr<FUICommandInfo> ConvertSpacesToTabs;
	TSharedPtr<FUICommandInfo> EnsureFinalNewline;
	TSharedPtr<FUICommandInfo> NormalizeLineEndingsLf;
	TSharedPtr<FUICommandInfo> NormalizeLineEndingsCrlf;
	TSharedPtr<FUICommandInfo> OpenHeaderSourcePair;
	TSharedPtr<FUICommandInfo> CompileProject;
	TSharedPtr<FUICommandInfo> OpenSettings;
};

namespace UNotepad::UI
{
	const FLinearColor BackgroundColor(0.035f, 0.038f, 0.045f, 1.0f);
	const FLinearColor PanelColor(0.055f, 0.060f, 0.070f, 1.0f);
	const FLinearColor RaisedPanelColor(0.075f, 0.083f, 0.095f, 1.0f);
	const FLinearColor ActiveTabColor(0.120f, 0.135f, 0.160f, 1.0f);
	const FLinearColor TextColor(0.830f, 0.860f, 0.900f, 1.0f);
	const FLinearColor MutedTextColor(0.520f, 0.570f, 0.650f, 1.0f);
}

void SNotepadWidget::Construct(const FArguments& InArgs)
{
	DocumentsRootPath = FPaths::ProjectSavedDir() / TEXT("UNotepad");
	IFileManager::Get().MakeDirectory(*DocumentsRootPath, true);

	const UUNotepadSettings* Settings = UUNotepadSettings::Get();
	bShowLineNumbers = Settings->bShowLineNumbersByDefault;
	bShowWhitespace = Settings->bShowWhitespaceByDefault;
	bShowSolutionExplorer = Settings->bShowSolutionExplorerByDefault;

	BindCommands();

	FMenuBarBuilder MenuBuilder(CommandList);
	MenuBuilder.AddPullDownMenu(
		LOCTEXT("FileMenu", "File"),
		LOCTEXT("FileMenu_ToolTip", "File commands"),
		FNewMenuDelegate::CreateSP(this, &SNotepadWidget::FillFileMenu));
	MenuBuilder.AddPullDownMenu(
		LOCTEXT("ToolsMenu", "Tools"),
		LOCTEXT("ToolsMenu_ToolTip", "Document tools"),
		FNewMenuDelegate::CreateSP(this, &SNotepadWidget::FillToolsMenu));
	MenuBuilder.AddPullDownMenu(
		LOCTEXT("WindowMenu", "Window"),
		LOCTEXT("WindowMenu_ToolTip", "Document group layout commands"),
		FNewMenuDelegate::CreateSP(this, &SNotepadWidget::FillWindowMenu));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::UI::BackgroundColor)
		.Padding(0.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MenuBuilder.MakeWidget()
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(ToolbarWidget, SNotepadToolbar)
				.OnNew(FNotepadToolbarAction::CreateLambda([this]() { OnNewClicked(); }))
				.OnOpen(FNotepadToolbarAction::CreateLambda([this]() { OnOpenClicked(); }))
				.OnSave(FNotepadToolbarAction::CreateLambda([this]() { OnSaveClicked(); }))
				.OnSaveAs(FNotepadToolbarAction::CreateLambda([this]() { OnSaveAsClicked(); }))
				.OnValidate(FNotepadToolbarAction::CreateLambda([this]() { OnValidateClicked(); }))
				.OnFormat(FNotepadToolbarAction::CreateLambda([this]() { OnFormatClicked(); }))
				.OnCompile(FNotepadToolbarAction::CreateLambda([this]() { OnCompileClicked(); }))
				.OnToggleLineNumbers(FNotepadToolbarAction::CreateLambda([this]() { OnToggleLineNumbersClicked(); }))
				.OnToggleWhitespace(FNotepadToolbarAction::CreateLambda([this]() { OnToggleWhitespaceClicked(); }))
				.OnClose(FNotepadToolbarAction::CreateLambda([this]() { OnCloseActiveTabClicked(); }))
				.OnModeChanged(FNotepadToolbarModeChanged::CreateSP(this, &SNotepadWidget::HandleModeChanged))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildFindReplacePanel()
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildGoToLinePanel()
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(WorkspaceHost, SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(UNotepad::UI::BackgroundColor)
				.Padding(0.0f)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				CreateStatusBar()
			]
		]
	];

	RebuildWorkspaceContent();
	GetOrCreateActiveGroup();
	CreateNewDocument(EUNotepadDocumentMode::Text);
	SetStatus(LOCTEXT("ReadyStatus", "Ready"));
}

bool SNotepadWidget::SupportsKeyboardFocus() const
{
	return true;
}

FReply SNotepadWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	return HandleShortcutKeyDown(MyGeometry, InKeyEvent);
}

bool SNotepadWidget::OpenFile(const FString& FilePath)
{
	return LoadDocumentFromFile(FilePath);
}

void SNotepadWidget::BindCommands()
{
	if (!FUNotepadEditorCommands::IsRegistered())
	{
		FUNotepadEditorCommands::Register();
	}

	CommandList = MakeShared<FUICommandList>();
	const FUNotepadEditorCommands& Commands = FUNotepadEditorCommands::Get();

	CommandList->MapAction(
		Commands.NewDocument,
		FExecuteAction::CreateLambda([this]() { CreateNewDocument(EUNotepadDocumentMode::Text); }));
	CommandList->MapAction(
		Commands.OpenDocument,
		FExecuteAction::CreateSP(this, &SNotepadWidget::OpenTextFileDialog));
	CommandList->MapAction(
		Commands.SaveDocument,
		FExecuteAction::CreateLambda([this]() { SaveActiveDocument(false); }));
	CommandList->MapAction(
		Commands.SaveDocumentAs,
		FExecuteAction::CreateLambda([this]() { SaveActiveDocument(true); }));
	CommandList->MapAction(
		Commands.CloseDocument,
		FExecuteAction::CreateLambda([this]() { CloseDocument(ActiveDocument); }));
	CommandList->MapAction(
		Commands.ShowFindReplace,
		FExecuteAction::CreateLambda([this]() { ShowFindReplacePanel(false); }));
	CommandList->MapAction(
		Commands.ShowReplace,
		FExecuteAction::CreateLambda([this]() { ShowFindReplacePanel(true); }));
	CommandList->MapAction(
		Commands.FindNext,
		FExecuteAction::CreateLambda([this]() { FindNext(false); }));
	CommandList->MapAction(
		Commands.FindPrevious,
		FExecuteAction::CreateLambda([this]() { FindNext(true); }));
	CommandList->MapAction(
		Commands.GoToLine,
		FExecuteAction::CreateSP(this, &SNotepadWidget::ShowGoToLinePanel));
	CommandList->MapAction(
		Commands.Undo,
		FExecuteAction::CreateSP(this, &SNotepadWidget::UndoActiveDocumentChange));
	CommandList->MapAction(
		Commands.Redo,
		FExecuteAction::CreateSP(this, &SNotepadWidget::RedoActiveDocumentChange));
	CommandList->MapAction(
		Commands.ToggleLineComment,
		FExecuteAction::CreateSP(this, &SNotepadWidget::ToggleLineComment));
	CommandList->MapAction(
		Commands.DuplicateLineOrSelection,
		FExecuteAction::CreateSP(this, &SNotepadWidget::DuplicateLineOrSelection));
	CommandList->MapAction(
		Commands.MoveLineUp,
		FExecuteAction::CreateSP(this, &SNotepadWidget::MoveCurrentLine, -1));
	CommandList->MapAction(
		Commands.MoveLineDown,
		FExecuteAction::CreateSP(this, &SNotepadWidget::MoveCurrentLine, 1));
	CommandList->MapAction(
		Commands.TrimTrailingWhitespace,
		FExecuteAction::CreateSP(this, &SNotepadWidget::TrimTrailingWhitespace));
	CommandList->MapAction(
		Commands.ConvertTabsToSpaces,
		FExecuteAction::CreateSP(this, &SNotepadWidget::ConvertTabsToSpaces));
	CommandList->MapAction(
		Commands.ConvertSpacesToTabs,
		FExecuteAction::CreateSP(this, &SNotepadWidget::ConvertSpacesToTabs));
	CommandList->MapAction(
		Commands.EnsureFinalNewline,
		FExecuteAction::CreateSP(this, &SNotepadWidget::EnsureFinalNewline));
	CommandList->MapAction(
		Commands.NormalizeLineEndingsLf,
		FExecuteAction::CreateSP(this, &SNotepadWidget::NormalizeLineEndings, ENotepadLineEnding::LF));
	CommandList->MapAction(
		Commands.NormalizeLineEndingsCrlf,
		FExecuteAction::CreateSP(this, &SNotepadWidget::NormalizeLineEndings, ENotepadLineEnding::CRLF));
	CommandList->MapAction(
		Commands.OpenHeaderSourcePair,
		FExecuteAction::CreateLambda([this]() { OpenHeaderSourcePair(); }));
	CommandList->MapAction(
		Commands.CompileProject,
		FExecuteAction::CreateSP(this, &SNotepadWidget::CompileProject));
	CommandList->MapAction(
		Commands.OpenSettings,
		FExecuteAction::CreateSP(this, &SNotepadWidget::OpenSettings));
}

FReply SNotepadWidget::HandleShortcutKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	(void)MyGeometry;

	if (InKeyEvent.GetKey() == EKeys::Escape && bShowFindReplace)
	{
		HideFindReplacePanel();
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == EKeys::Escape && bShowGoToLine)
	{
		HideGoToLinePanel();
		return FReply::Handled();
	}

	if (CommandList.IsValid() && CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedPtr<FNotepadDocument> SNotepadWidget::CreateNewDocumentInGroup(EUNotepadDocumentMode Mode, TSharedPtr<FNotepadDocumentGroup> Group)
{
	Group = Group.IsValid() ? Group : GetOrCreateActiveGroup();

	TSharedPtr<FNotepadDocument> Document = MakeShared<FNotepadDocument>();
	Document->Mode = Mode;
	Document->DisplayName = FNotepadDocumentUtils::GetUntitledName(Mode, UntitledCounter++);
	Document->Content = Mode == EUNotepadDocumentMode::Json ? TEXT("{\n}\n") : TEXT("");
	Document->SavedContent = Document->Content;
	Document->bDirty = false;

	Documents.Add(Document);
	Group->Documents.Add(Document);
	SetActiveDocument(Document, Group);
	return Document;
}

void SNotepadWidget::CreateNewDocument(EUNotepadDocumentMode Mode)
{
	CreateNewDocumentInGroup(Mode, GetOrCreateActiveGroup());
	SetStatus(FText::Format(LOCTEXT("NewDocumentStatus", "Created {0} document"), FText::FromString(FNotepadDocumentUtils::GetModeLabel(Mode))));
}

void SNotepadWidget::OpenTextFileDialog()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		SetStatus(LOCTEXT("DesktopPlatformMissing", "DesktopPlatform module is not available"), true);
		return;
	}

	const FString DefaultPath = ActiveDocument.IsValid() && !ActiveDocument->FilePath.IsEmpty()
		? FPaths::GetPath(ActiveDocument->FilePath)
		: DocumentsRootPath;

	TArray<FString> OutFiles;
	const FString FileTypes =
		TEXT("Editable Files (*.txt;*.json;*.csv;*.h;*.hpp;*.cpp;*.cs;*.ini;*.uplugin;*.uproject)|*.txt;*.json;*.csv;*.h;*.hpp;*.cpp;*.cs;*.ini;*.uplugin;*.uproject|")
		TEXT("Text Files (*.txt)|*.txt|JSON Files (*.json;*.uplugin;*.uproject)|*.json;*.uplugin;*.uproject|CSV Files (*.csv)|*.csv|All Files (*.*)|*.*");

	const bool bFileSelected = DesktopPlatform->OpenFileDialog(
		nullptr,
		TEXT("Open File"),
		DefaultPath,
		TEXT(""),
		FileTypes,
		EFileDialogFlags::Multiple,
		OutFiles);

	if (!bFileSelected)
	{
		return;
	}

	for (const FString& FilePath : OutFiles)
	{
		LoadDocumentFromFile(FilePath);
	}
}

bool SNotepadWidget::LoadDocumentFromFile(const FString& FilePath)
{
	const FString NormalizedPath = FNotepadFileService::NormalizeFilePath(FilePath);

	for (const TSharedPtr<FNotepadDocument>& Document : Documents)
	{
		if (Document.IsValid() && Document->FilePath.Equals(NormalizedPath, ESearchCase::IgnoreCase))
		{
			SetActiveDocument(Document, FindGroupForDocument(Document));
			SetStatus(FText::Format(LOCTEXT("FileAlreadyOpenStatus", "{0} is already open"), FText::FromString(Document->DisplayName)));
			return true;
		}
	}

	FString LoadedText;
	FString Error;
	if (!FNotepadFileService::LoadFileToString(NormalizedPath, LoadedText, Error))
	{
		SetStatus(FText::FromString(Error), true);
		return false;
	}

	TSharedPtr<FNotepadDocument> Document = MakeShared<FNotepadDocument>();
	Document->FilePath = NormalizedPath;
	Document->DisplayName = FNotepadDocumentUtils::MakeDisplayName(NormalizedPath);
	Document->Content = LoadedText;
	Document->SavedContent = LoadedText;
	Document->Mode = FNotepadDocumentUtils::DetectModeFromPath(NormalizedPath);
	Document->bDirty = false;

	Documents.Add(Document);
	TSharedPtr<FNotepadDocumentGroup> TargetGroup = GetOrCreateActiveGroup();
	TargetGroup->Documents.Add(Document);
	SetActiveDocument(Document, TargetGroup);
	SetStatus(FText::Format(LOCTEXT("FileOpenedStatus", "Opened {0}"), FText::FromString(Document->DisplayName)));
	return true;
}

bool SNotepadWidget::SaveActiveDocument(bool bSaveAs)
{
	return SaveDocument(ActiveDocument, bSaveAs);
}

bool SNotepadWidget::SaveDocument(const TSharedPtr<FNotepadDocument>& Document, bool bSaveAs)
{
	if (!Document.IsValid())
	{
		SetStatus(LOCTEXT("NoDocumentToSave", "No document to save"), true);
		return false;
	}

	SyncDocumentFromOwningEditor(Document);

	FString SavePath = Document->FilePath;
	if (bSaveAs || SavePath.IsEmpty())
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		if (!DesktopPlatform)
		{
			SetStatus(LOCTEXT("DesktopPlatformMissingSave", "DesktopPlatform module is not available"), true);
			return false;
		}

		TArray<FString> OutFiles;
		const FString DefaultPath = SavePath.IsEmpty() ? DocumentsRootPath : FPaths::GetPath(SavePath);
		const FString DefaultFile = SavePath.IsEmpty() ? Document->DisplayName : FPaths::GetCleanFilename(SavePath);
		const FString FileTypes =
			TEXT("Text Files (*.txt)|*.txt|Code Files (*.h;*.hpp;*.cpp;*.cs;*.ini)|*.h;*.hpp;*.cpp;*.cs;*.ini|")
			TEXT("JSON Files (*.json;*.uplugin;*.uproject)|*.json;*.uplugin;*.uproject|CSV Files (*.csv)|*.csv|All Files (*.*)|*.*");

		const bool bFileSelected = DesktopPlatform->SaveFileDialog(
			nullptr,
			TEXT("Save File"),
			DefaultPath,
			DefaultFile,
			FileTypes,
			EFileDialogFlags::None,
			OutFiles);

		if (!bFileSelected || OutFiles.IsEmpty())
		{
			SetStatus(LOCTEXT("SaveCancelledStatus", "Save cancelled"));
			return false;
		}

		SavePath = OutFiles[0];
		if (FPaths::GetExtension(SavePath).IsEmpty())
		{
			SavePath += TEXT(".") + FNotepadDocumentUtils::GetDefaultExtension(Document->Mode);
		}
	}

	FString Error;
	if (!FNotepadFileService::SaveStringToFile(SavePath, Document->Content, Error))
	{
		SetStatus(FText::FromString(Error), true);
		return false;
	}

	Document->FilePath = FNotepadFileService::NormalizeFilePath(SavePath);
	Document->DisplayName = FNotepadDocumentUtils::MakeDisplayName(Document->FilePath);
	Document->Mode = FNotepadDocumentUtils::DetectModeFromPath(Document->FilePath);
	FNotepadDocumentHistoryService::MarkSaved(Document);

	RebuildTabStrip();
	RefreshDocumentControls();
	SetStatus(FText::Format(LOCTEXT("FileSavedStatus", "Saved {0}"), FText::FromString(Document->DisplayName)));
	return true;
}

bool SNotepadWidget::CloseDocument(const TSharedPtr<FNotepadDocument>& Document)
{
	if (!Document.IsValid())
	{
		return true;
	}

	TSharedPtr<FNotepadDocumentGroup> OwningGroup = FindGroupForDocument(Document);
	SyncDocumentFromOwningEditor(Document);

	if (Document->bDirty)
	{
		const EAppReturnType::Type UserChoice = FMessageDialog::Open(
			EAppMsgType::YesNoCancel,
			FText::Format(LOCTEXT("SaveDirtyDocumentPrompt", "Save changes to {0} before closing?"), FText::FromString(Document->DisplayName)));

		if (UserChoice == EAppReturnType::Cancel)
		{
			return false;
		}

		if (UserChoice == EAppReturnType::Yes && !SaveDocument(Document, false))
		{
			return false;
		}
	}

	Documents.Remove(Document);

	if (OwningGroup.IsValid())
	{
		const int32 ClosedIndex = OwningGroup->Documents.IndexOfByKey(Document);
		OwningGroup->Documents.Remove(Document);

		if (OwningGroup->ActiveDocument == Document)
		{
			OwningGroup->ActiveDocument.Reset();
			if (OwningGroup->Documents.Num() > 0)
			{
				OwningGroup->ActiveDocument = OwningGroup->Documents[FMath::Clamp(ClosedIndex, 0, OwningGroup->Documents.Num() - 1)];
			}
		}

		if (OwningGroup->Documents.Num() == 0)
		{
			const int32 RemovedGroupIndex = FindGroupIndex(OwningGroup);
			DocumentGroups.Remove(OwningGroup);

			if (ActiveGroup == OwningGroup)
			{
				ActiveGroup.Reset();
				ActiveDocument.Reset();

				if (DocumentGroups.Num() > 0)
				{
					ActiveGroup = DocumentGroups[FMath::Clamp(RemovedGroupIndex, 0, DocumentGroups.Num() - 1)];
					ActiveDocument = ActiveGroup->ActiveDocument;
				}
			}

			RebuildDocumentGroups();
		}
	}

	if (Documents.Num() == 0)
	{
		CreateNewDocumentInGroup(EUNotepadDocumentMode::Text, GetOrCreateActiveGroup());
	}
	else if (ActiveDocument == Document)
	{
		SetActiveGroup(ActiveGroup.IsValid() ? ActiveGroup : GetOrCreateActiveGroup(), true);
	}

	RebuildTabStrip();
	RefreshDocumentControls();

	SetStatus(FText::Format(LOCTEXT("ClosedDocumentStatus", "Closed {0}"), FText::FromString(Document->DisplayName)));
	return true;
}

void SNotepadWidget::SetActiveDocument(const TSharedPtr<FNotepadDocument>& Document, TSharedPtr<FNotepadDocumentGroup> Group)
{
	if (!Document.IsValid())
	{
		return;
	}

	Group = Group.IsValid() ? Group : FindGroupForDocument(Document);
	if (!Group.IsValid())
	{
		return;
	}

	if (ActiveGroup == Group && ActiveDocument == Document)
	{
		if (Group->EditorWidget.IsValid())
		{
			Group->EditorWidget->FocusEditor();
		}
		RefreshDocumentControls();
		return;
	}

	SyncDocumentFromGroupEditor(ActiveGroup);
	if (ActiveGroup != Group)
	{
		SyncDocumentFromGroupEditor(Group);
	}

	ActiveGroup = Group;
	ActiveDocument = Document;
	Group->ActiveDocument = Document;

	RefreshGroupEditor(Group, true);
	RebuildTabStrip();
	RefreshDocumentControls();
}

void SNotepadWidget::SetActiveGroup(TSharedPtr<FNotepadDocumentGroup> Group, bool bFocusEditor)
{
	if (!Group.IsValid())
	{
		return;
	}

	if (ActiveGroup != Group)
	{
		SyncDocumentFromGroupEditor(ActiveGroup);
	}

	ActiveGroup = Group;
	ActiveDocument = Group->ActiveDocument;

	RefreshGroupEditor(Group, bFocusEditor);
	RebuildTabStrip();
	RefreshDocumentControls();
}

void SNotepadWidget::SyncActiveDocumentFromEditor()
{
	SyncDocumentFromGroupEditor(ActiveGroup);
}

void SNotepadWidget::SyncDocumentFromGroupEditor(TSharedPtr<FNotepadDocumentGroup> Group)
{
	if (Group.IsValid() && Group->ActiveDocument.IsValid() && Group->EditorWidget.IsValid())
	{
		FNotepadDocumentHistoryService::SetContent(Group->ActiveDocument, Group->EditorWidget->GetText(), false);
	}
}

void SNotepadWidget::SyncDocumentFromOwningEditor(const TSharedPtr<FNotepadDocument>& Document)
{
	TSharedPtr<FNotepadDocumentGroup> OwningGroup = FindGroupForDocument(Document);
	if (OwningGroup.IsValid() && OwningGroup->ActiveDocument == Document)
	{
		SyncDocumentFromGroupEditor(OwningGroup);
	}
}

void SNotepadWidget::SyncAllDocumentGroupEditors()
{
	for (const TSharedPtr<FNotepadDocumentGroup>& Group : DocumentGroups)
	{
		SyncDocumentFromGroupEditor(Group);
	}
}

void SNotepadWidget::RefreshAfterDocumentContentChange(const TSharedPtr<FNotepadDocument>& Document, bool bFocusEditor)
{
	if (!Document.IsValid())
	{
		return;
	}

	TSharedPtr<FNotepadDocumentGroup> Group = FindGroupForDocument(Document);
	if (Group.IsValid() && Group->ActiveDocument == Document)
	{
		RefreshGroupEditor(Group, bFocusEditor);
	}

	RebuildTabStrip();
	RefreshDocumentControls();
	UpdateActiveSearchHighlight();
}

void SNotepadWidget::FillFileMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("NewTextFile", "New Text"),
		LOCTEXT("NewTextFileTooltip", "Create a new text tab"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::CreateNewDocument, EUNotepadDocumentMode::Text)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NewCodeFile", "New Code"),
		LOCTEXT("NewCodeFileTooltip", "Create a new code tab"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::CreateNewDocument, EUNotepadDocumentMode::Code)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NewJsonFile", "New JSON"),
		LOCTEXT("NewJsonFileTooltip", "Create a new JSON tab"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::CreateNewDocument, EUNotepadDocumentMode::Json)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NewCsvFile", "New CSV"),
		LOCTEXT("NewCsvFileTooltip", "Create a new CSV tab"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::CreateNewDocument, EUNotepadDocumentMode::Csv)));

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenFile", "Open..."),
		LOCTEXT("OpenFileTooltip", "Open one or more editable files"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.FolderOpen"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::OpenTextFileDialog)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SaveFile", "Save"),
		LOCTEXT("SaveFileTooltip", "Save the active tab"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"),
		FUIAction(FExecuteAction::CreateLambda([this]() { SaveActiveDocument(false); })));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SaveFileAs", "Save As..."),
		LOCTEXT("SaveFileAsTooltip", "Save the active tab to another file"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.SaveAs"),
		FUIAction(FExecuteAction::CreateLambda([this]() { SaveActiveDocument(true); })));

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CloseActiveTab", "Close Tab"),
		LOCTEXT("CloseActiveTabTooltip", "Close the active tab"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.X"),
		FUIAction(FExecuteAction::CreateLambda([this]() { CloseDocument(ActiveDocument); })));
}

void SNotepadWidget::FillToolsMenu(FMenuBuilder& MenuBuilder)
{
	const FUNotepadEditorCommands& Commands = FUNotepadEditorCommands::Get();

	MenuBuilder.AddMenuEntry(Commands.Undo);
	MenuBuilder.AddMenuEntry(Commands.Redo);
	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(Commands.ShowFindReplace);
	MenuBuilder.AddMenuEntry(Commands.ShowReplace);
	MenuBuilder.AddMenuEntry(Commands.FindNext);
	MenuBuilder.AddMenuEntry(Commands.FindPrevious);
	MenuBuilder.AddMenuEntry(Commands.GoToLine);

	MenuBuilder.BeginSection("UNotepadTextActions", LOCTEXT("TextActionsSection", "Text Actions"));
	MenuBuilder.AddMenuEntry(Commands.ToggleLineComment);
	MenuBuilder.AddMenuEntry(Commands.DuplicateLineOrSelection);
	MenuBuilder.AddMenuEntry(Commands.MoveLineUp);
	MenuBuilder.AddMenuEntry(Commands.MoveLineDown);
	MenuBuilder.AddMenuSeparator();
	MenuBuilder.AddMenuEntry(Commands.TrimTrailingWhitespace);
	MenuBuilder.AddMenuEntry(Commands.ConvertTabsToSpaces);
	MenuBuilder.AddMenuEntry(Commands.ConvertSpacesToTabs);
	MenuBuilder.AddMenuEntry(Commands.EnsureFinalNewline);
	MenuBuilder.AddMenuSeparator();
	MenuBuilder.AddMenuEntry(Commands.NormalizeLineEndingsLf);
	MenuBuilder.AddMenuEntry(Commands.NormalizeLineEndingsCrlf);
	MenuBuilder.EndSection();

	MenuBuilder.AddMenuEntry(Commands.OpenHeaderSourcePair);
	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ValidateDocument", "Validate"),
		LOCTEXT("ValidateDocumentTooltip", "Validate the active document for its current mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Check"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::ValidateActiveDocument)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("FormatDocument", "Format"),
		LOCTEXT("FormatDocumentTooltip", "Format JSON or normalize CSV"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::FormatActiveDocument)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CompileProject", "Compile"),
		LOCTEXT("CompileProjectTooltip", "Compile the current Unreal project code"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Compile"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::CompileProject)));

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(Commands.OpenSettings);

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ToggleLineNumbers", "Toggle Line Numbers"),
		LOCTEXT("ToggleLineNumbersTooltip", "Show or hide editor line numbers"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.List"),
		FUIAction(FExecuteAction::CreateLambda([this]() { OnToggleLineNumbersClicked(); })));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ToggleWhitespace", "Toggle Whitespace"),
		LOCTEXT("ToggleWhitespaceTooltip", "Show or hide space and tab markers"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Eye"),
		FUIAction(FExecuteAction::CreateLambda([this]() { OnToggleWhitespaceClicked(); })));
}

void SNotepadWidget::FillWindowMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ToggleSolutionExplorer", "Solution Explorer"),
		LOCTEXT("ToggleSolutionExplorerTooltip", "Show or hide the Solution Explorer panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.FolderOpen"),
		FUIAction(
			FExecuteAction::CreateLambda([this]() { OnToggleSolutionExplorerClicked(); }),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda([this]()
			{
				return bShowSolutionExplorer;
			})),
		NAME_None,
		EUserInterfaceActionType::ToggleButton);

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NewVerticalTabGroup", "New Vertical Tab Group"),
		LOCTEXT("NewVerticalTabGroupTooltip", "Move the active tab to a new side-by-side document group"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Layout"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::CreateNewDocumentGroup, ENotepadDocumentGroupLayout::Vertical)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("NewHorizontalTabGroup", "New Horizontal Tab Group"),
		LOCTEXT("NewHorizontalTabGroupTooltip", "Move the active tab to a new stacked document group"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Layout"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::CreateNewDocumentGroup, ENotepadDocumentGroupLayout::Horizontal)));

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MoveTabToPreviousGroup", "Move Tab to Previous Group"),
		LOCTEXT("MoveTabToPreviousGroupTooltip", "Move the active tab to the previous document group"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.ArrowLeft"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::MoveActiveDocumentToAdjacentGroup, -1)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MoveTabToNextGroup", "Move Tab to Next Group"),
		LOCTEXT("MoveTabToNextGroupTooltip", "Move the active tab to the next document group"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.ArrowRight"),
		FUIAction(FExecuteAction::CreateSP(this, &SNotepadWidget::MoveActiveDocumentToAdjacentGroup, 1)));
}

TSharedRef<SWidget> SNotepadWidget::CreateStatusBar()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::UI::PanelColor)
		.Padding(FMargin(8.0f, 3.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SNotepadWidget::GetStatusText)
				.ColorAndOpacity(UNotepad::UI::MutedTextColor)
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SNotepadWidget::GetLastMessageText)
				.ColorAndOpacity(this, &SNotepadWidget::GetLastMessageColor)
			]
		];
}

TSharedRef<SWidget> SNotepadWidget::BuildFindReplacePanel()
{
	return SNew(SBorder)
		.Visibility(this, &SNotepadWidget::GetFindReplaceVisibility)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::UI::RaisedPanelColor)
		.Padding(FMargin(8.0f, 5.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("FindLabel", "Find"))
				.ColorAndOpacity(UNotepad::UI::MutedTextColor)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(240.0f)
				[
					SAssignNew(FindTextBox, SEditableTextBox)
					.Text(this, &SNotepadWidget::GetFindText)
					.SelectAllTextWhenFocused(true)
					.OnTextChanged(this, &SNotepadWidget::HandleFindTextChanged)
					.OnTextCommitted(this, &SNotepadWidget::HandleFindTextCommitted)
					.OnKeyDownHandler(this, &SNotepadWidget::HandleShortcutKeyDown)
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(7.0f, 2.0f))
				.ToolTipText(LOCTEXT("FindPreviousTooltip", "Find previous match"))
				.OnClicked(this, &SNotepadWidget::OnFindPreviousClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FindPreviousButton", "Prev"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(7.0f, 2.0f))
				.ToolTipText(LOCTEXT("FindNextTooltip", "Find next match"))
				.OnClicked(this, &SNotepadWidget::OnFindNextClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FindNextButton", "Next"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ReplaceLabel", "Replace"))
				.ColorAndOpacity(UNotepad::UI::MutedTextColor)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(240.0f)
				[
					SAssignNew(ReplaceTextBox, SEditableTextBox)
					.Text(this, &SNotepadWidget::GetReplaceText)
					.SelectAllTextWhenFocused(true)
					.OnTextChanged(this, &SNotepadWidget::HandleReplaceTextChanged)
					.OnTextCommitted(this, &SNotepadWidget::HandleReplaceTextCommitted)
					.OnKeyDownHandler(this, &SNotepadWidget::HandleShortcutKeyDown)
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(7.0f, 2.0f))
				.ToolTipText(LOCTEXT("ReplaceCurrentTooltip", "Replace the current match"))
				.OnClicked(this, &SNotepadWidget::OnReplaceClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ReplaceCurrentButton", "Replace"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(7.0f, 2.0f))
				.ToolTipText(LOCTEXT("ReplaceAllTooltip", "Replace all matches in the active document"))
				.OnClicked(this, &SNotepadWidget::OnReplaceAllClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ReplaceAllButton", "All"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SNotepadWidget::GetFindCaseSensitiveState)
				.OnCheckStateChanged(this, &SNotepadWidget::HandleFindCaseSensitiveChanged)
				.ToolTipText(LOCTEXT("CaseSensitiveTooltip", "Match case"))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CaseSensitiveLabel", "Aa"))
					.ColorAndOpacity(UNotepad::UI::TextColor)
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ContentPadding(FMargin(7.0f, 2.0f))
				.ToolTipText(LOCTEXT("CloseFindReplaceTooltip", "Close find and replace"))
				.OnClicked(this, &SNotepadWidget::OnCloseFindReplaceClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CloseFindReplaceButton", "x"))
				]
			]
		];
}

TSharedRef<SWidget> SNotepadWidget::BuildGoToLinePanel()
{
	return SNew(SBorder)
		.Visibility(this, &SNotepadWidget::GetGoToLineVisibility)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::UI::RaisedPanelColor)
		.Padding(FMargin(8.0f, 5.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 5.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("GoToLineLabel", "Line"))
				.ColorAndOpacity(UNotepad::UI::MutedTextColor)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(120.0f)
				[
					SAssignNew(GoToLineTextBox, SEditableTextBox)
					.Text(this, &SNotepadWidget::GetGoToLineText)
					.SelectAllTextWhenFocused(true)
					.OnTextChanged(this, &SNotepadWidget::HandleGoToLineTextChanged)
					.OnTextCommitted(this, &SNotepadWidget::HandleGoToLineTextCommitted)
					.OnKeyDownHandler(this, &SNotepadWidget::HandleShortcutKeyDown)
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(7.0f, 2.0f))
				.ToolTipText(LOCTEXT("GoToLineTooltip", "Go to the entered line number"))
				.OnClicked(this, &SNotepadWidget::OnGoToLineClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("GoToLineButton", "Go"))
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ContentPadding(FMargin(7.0f, 2.0f))
				.ToolTipText(LOCTEXT("CloseGoToLineTooltip", "Close go to line"))
				.OnClicked(this, &SNotepadWidget::OnCloseGoToLineClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CloseGoToLineButton", "x"))
				]
			]
		];
}

TSharedRef<SWidget> SNotepadWidget::BuildWorkspaceContent()
{
	if (bShowSolutionExplorer)
	{
		return SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			+ SSplitter::Slot()
			.Value(0.22f)
			.MinSize(220.0f)
			[
				BuildSolutionExplorer()
			]

			+ SSplitter::Slot()
			.Value(0.78f)
			[
				SAssignNew(DocumentGroupsHost, SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(UNotepad::UI::BackgroundColor)
				.Padding(FMargin(8.0f))
			];
	}

	SourceTreeView.Reset();

	return SAssignNew(DocumentGroupsHost, SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::UI::BackgroundColor)
		.Padding(FMargin(8.0f));
}

TSharedRef<SWidget> SNotepadWidget::BuildSolutionExplorer()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::UI::PanelColor)
		.Padding(FMargin(1.0f, 8.0f, 0.0f, 8.0f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(UNotepad::UI::RaisedPanelColor)
				.Padding(FMargin(8.0f, 5.0f))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SolutionExplorerTitle", "Solution Explorer"))
						.ColorAndOpacity(UNotepad::UI::TextColor)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.ContentPadding(FMargin(7.0f, 2.0f))
						.ToolTipText(LOCTEXT("RefreshSolutionExplorerTooltip", "Refresh source file tree"))
						.OnClicked(this, &SNotepadWidget::OnRefreshSolutionExplorerClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("RefreshSolutionExplorer", "Refresh"))
							.ColorAndOpacity(UNotepad::UI::TextColor)
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(UNotepad::UI::BackgroundColor)
				.Padding(FMargin(4.0f))
				[
					SAssignNew(SourceTreeView, STreeView<TSharedPtr<FNotepadSourceTreeItem>>)
					.TreeItemsSource(&SourceTreeRoots)
					.OnGenerateRow(this, &SNotepadWidget::GenerateSourceTreeRow)
					.OnGetChildren(this, &SNotepadWidget::GetSourceTreeChildren)
					.OnMouseButtonDoubleClick(this, &SNotepadWidget::HandleSourceTreeDoubleClick)
					.SelectionMode(ESelectionMode::Single)
				]
			]
		];
}

void SNotepadWidget::RebuildWorkspaceContent()
{
	if (!WorkspaceHost.IsValid())
	{
		return;
	}

	WorkspaceHost->SetContent(BuildWorkspaceContent());
	RebuildDocumentGroups();
	RefreshDocumentControls();

	if (bShowSolutionExplorer)
	{
		RefreshSolutionExplorer();
	}
}

TSharedRef<SWidget> SNotepadWidget::BuildDocumentGroup(TSharedPtr<FNotepadDocumentGroup> Group)
{
	TSharedPtr<SNotepadTabStrip> NewTabStrip;
	TSharedPtr<SNotepadEditor> NewEditor;

	TSharedRef<SWidget> GroupWidget = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor_Lambda([this, Group]()
		{
			return Group == ActiveGroup ? UNotepad::UI::ActiveTabColor : UNotepad::UI::RaisedPanelColor;
		})
		.Padding(1.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(NewTabStrip, SNotepadTabStrip)
				.OnDocumentSelected(FNotepadDocumentAction::CreateLambda([this, Group](TSharedPtr<FNotepadDocument> Document)
				{
					SetActiveDocument(Document, Group);
				}))
				.OnDocumentClosed(FNotepadDocumentAction::CreateLambda([this](TSharedPtr<FNotepadDocument> Document)
				{
					CloseDocument(Document);
				}))
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(UNotepad::UI::BackgroundColor)
				.Padding(FMargin(7.0f))
				[
					SAssignNew(NewEditor, SNotepadEditor)
					.OnTextChanged_Lambda([this, Group](const FText& NewText)
					{
						HandleEditorTextChanged(Group, NewText);
					})
					.OnShortcutKeyDown(this, &SNotepadWidget::HandleShortcutKeyDown)
				]
			]
		];

	Group->TabStripWidget = NewTabStrip;
	Group->EditorWidget = NewEditor;

	if (Group->EditorWidget.IsValid())
	{
		Group->EditorWidget->SetShowLineNumbers(bShowLineNumbers);
		Group->EditorWidget->SetShowWhitespace(bShowWhitespace);
		Group->EditorWidget->SetTabSize(UUNotepadSettings::Get()->GetClampedTabSize());
	}

	RebuildGroupTabStrip(Group);
	RefreshGroupEditor(Group);
	return GroupWidget;
}

void SNotepadWidget::RefreshSolutionExplorer()
{
	SourceTreeRoots.Reset();

	const FString ProjectSourceDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source"));
	if (IFileManager::Get().DirectoryExists(*ProjectSourceDir))
	{
		TSharedPtr<FNotepadSourceTreeItem> ProjectSourceRoot = BuildSourceTreeFromDirectory(ProjectSourceDir, TEXT("Source"));
		if (ProjectSourceRoot.IsValid() && ProjectSourceRoot->Children.Num() > 0)
		{
			SourceTreeRoots.Add(ProjectSourceRoot);
		}
	}

	const FString ProjectPluginsDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());
	if (IFileManager::Get().DirectoryExists(*ProjectPluginsDir))
	{
		TArray<FString> PluginDescriptors;
		IFileManager::Get().FindFilesRecursive(PluginDescriptors, *ProjectPluginsDir, TEXT("*.uplugin"), true, false);
		PluginDescriptors.Sort([](const FString& Left, const FString& Right)
		{
			return Left.Compare(Right, ESearchCase::IgnoreCase) < 0;
		});

		TSharedPtr<FNotepadSourceTreeItem> PluginsRoot = MakeShared<FNotepadSourceTreeItem>();
		PluginsRoot->Name = TEXT("Plugins");
		PluginsRoot->FilePath = ProjectPluginsDir;
		PluginsRoot->bDirectory = true;

		for (const FString& PluginDescriptor : PluginDescriptors)
		{
			const FString PluginRootDir = FPaths::GetPath(PluginDescriptor);
			const FString PluginSourceDir = PluginRootDir / TEXT("Source");
			if (!IFileManager::Get().DirectoryExists(*PluginSourceDir))
			{
				continue;
			}

			TSharedPtr<FNotepadSourceTreeItem> PluginSourceRoot = BuildSourceTreeFromDirectory(PluginSourceDir, FPaths::GetBaseFilename(PluginDescriptor));
			if (PluginSourceRoot.IsValid() && PluginSourceRoot->Children.Num() > 0)
			{
				PluginsRoot->Children.Add(PluginSourceRoot);
			}
		}

		if (PluginsRoot->Children.Num() > 0)
		{
			SortSourceTreeChildren(PluginsRoot);
			SourceTreeRoots.Add(PluginsRoot);
		}
	}

	int32 SourceFileCount = 0;
	for (const TSharedPtr<FNotepadSourceTreeItem>& Root : SourceTreeRoots)
	{
		SourceFileCount += CountSourceTreeFiles(Root);
	}

	if (SourceTreeView.IsValid())
	{
		SourceTreeView->RequestTreeRefresh();
		for (const TSharedPtr<FNotepadSourceTreeItem>& Root : SourceTreeRoots)
		{
			SourceTreeView->SetItemExpansion(Root, true);

			if (Root.IsValid() && Root->Name == TEXT("Plugins"))
			{
				for (const TSharedPtr<FNotepadSourceTreeItem>& PluginRoot : Root->Children)
				{
					SourceTreeView->SetItemExpansion(PluginRoot, true);
				}
			}
		}
	}

	SetStatus(FText::Format(LOCTEXT("SolutionExplorerRefreshedStatus", "Solution Explorer: {0} source files"), FText::AsNumber(SourceFileCount)));
}

TSharedPtr<FNotepadSourceTreeItem> SNotepadWidget::BuildSourceTreeFromDirectory(const FString& DirectoryPath, const FString& DisplayName) const
{
	FString NormalizedRootPath = FPaths::ConvertRelativePathToFull(DirectoryPath);
	FPaths::NormalizeDirectoryName(NormalizedRootPath);

	if (!IFileManager::Get().DirectoryExists(*NormalizedRootPath))
	{
		return nullptr;
	}

	TSharedPtr<FNotepadSourceTreeItem> Root = MakeShared<FNotepadSourceTreeItem>();
	Root->Name = DisplayName;
	Root->FilePath = NormalizedRootPath;
	Root->bDirectory = true;

	TArray<FString> SourceFiles;
	for (const FString& Extension : UUNotepadSettings::Get()->GetNormalizedSourceFileExtensions())
	{
		const FString Pattern = FString::Printf(TEXT("*.%s"), *Extension);
		IFileManager::Get().FindFilesRecursive(SourceFiles, *NormalizedRootPath, *Pattern, true, false, false);
	}

	SourceFiles.Sort([](const FString& Left, const FString& Right)
	{
		return Left.Compare(Right, ESearchCase::IgnoreCase) < 0;
	});

	for (const FString& SourceFile : SourceFiles)
	{
		AddFileToSourceTree(Root, NormalizedRootPath, SourceFile);
	}

	SortSourceTreeChildren(Root);
	return Root;
}

void SNotepadWidget::AddFileToSourceTree(TSharedPtr<FNotepadSourceTreeItem> Root, const FString& RootPath, const FString& FilePath) const
{
	if (!Root.IsValid() || !IsSolutionExplorerSourceFile(FilePath))
	{
		return;
	}

	FString NormalizedFilePath = FPaths::ConvertRelativePathToFull(FilePath);
	FPaths::NormalizeFilename(NormalizedFilePath);

	FString RelativePath = NormalizedFilePath;
	if (!FPaths::MakePathRelativeTo(RelativePath, *RootPath))
	{
		return;
	}
	FPaths::NormalizeFilename(RelativePath);

	TArray<FString> PathParts;
	RelativePath.ParseIntoArray(PathParts, TEXT("/"), true);
	if (PathParts.Num() == 0)
	{
		return;
	}

	TSharedPtr<FNotepadSourceTreeItem> Parent = Root;
	for (int32 Index = 0; Index < PathParts.Num() - 1; ++Index)
	{
		Parent = FindOrAddSourceTreeDirectory(Parent, PathParts[Index]);
		if (!Parent.IsValid())
		{
			return;
		}
	}

	for (const TSharedPtr<FNotepadSourceTreeItem>& ExistingChild : Parent->Children)
	{
		if (ExistingChild.IsValid() && !ExistingChild->bDirectory && ExistingChild->FilePath.Equals(NormalizedFilePath, ESearchCase::IgnoreCase))
		{
			return;
		}
	}

	TSharedPtr<FNotepadSourceTreeItem> FileItem = MakeShared<FNotepadSourceTreeItem>();
	FileItem->Name = PathParts.Last();
	FileItem->FilePath = NormalizedFilePath;
	FileItem->bDirectory = false;
	Parent->Children.Add(FileItem);
}

TSharedRef<ITableRow> SNotepadWidget::GenerateSourceTreeRow(TSharedPtr<FNotepadSourceTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bDirectory = Item.IsValid() && Item->bDirectory;
	const FString DisplayName = Item.IsValid() ? Item->Name : FString();
	const FString ToolTip = Item.IsValid() ? Item->FilePath : FString();

	return SNew(STableRow<TSharedPtr<FNotepadSourceTreeItem>>, OwnerTable)
		.Padding(FMargin(2.0f, 1.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(DisplayName))
			.ToolTipText(FText::FromString(ToolTip))
			.ColorAndOpacity(bDirectory ? UNotepad::UI::TextColor : UNotepad::UI::MutedTextColor)
		];
}

void SNotepadWidget::GetSourceTreeChildren(TSharedPtr<FNotepadSourceTreeItem> Item, TArray<TSharedPtr<FNotepadSourceTreeItem>>& OutChildren) const
{
	if (Item.IsValid())
	{
		OutChildren.Append(Item->Children);
	}
}

void SNotepadWidget::HandleSourceTreeDoubleClick(TSharedPtr<FNotepadSourceTreeItem> Item)
{
	if (!Item.IsValid())
	{
		return;
	}

	if (Item->bDirectory)
	{
		if (SourceTreeView.IsValid())
		{
			SourceTreeView->SetItemExpansion(Item, true);
		}
		return;
	}

	LoadDocumentFromFile(Item->FilePath);
}

FReply SNotepadWidget::OnRefreshSolutionExplorerClicked()
{
	RefreshSolutionExplorer();
	return FReply::Handled();
}

TSharedPtr<FNotepadDocumentGroup> SNotepadWidget::CreateDocumentGroup(int32 InsertIndex)
{
	TSharedPtr<FNotepadDocumentGroup> Group = MakeShared<FNotepadDocumentGroup>();

	if (DocumentGroups.IsValidIndex(InsertIndex))
	{
		DocumentGroups.Insert(Group, InsertIndex);
	}
	else
	{
		DocumentGroups.Add(Group);
	}

	if (!ActiveGroup.IsValid())
	{
		ActiveGroup = Group;
	}

	return Group;
}

TSharedPtr<FNotepadDocumentGroup> SNotepadWidget::GetOrCreateActiveGroup()
{
	if (!ActiveGroup.IsValid())
	{
		if (DocumentGroups.Num() == 0)
		{
			CreateDocumentGroup();
			RebuildDocumentGroups();
		}
		else
		{
			ActiveGroup = DocumentGroups[0];
		}
	}

	return ActiveGroup;
}

TSharedPtr<FNotepadDocumentGroup> SNotepadWidget::FindGroupForDocument(const TSharedPtr<FNotepadDocument>& Document) const
{
	if (!Document.IsValid())
	{
		return nullptr;
	}

	for (const TSharedPtr<FNotepadDocumentGroup>& Group : DocumentGroups)
	{
		if (Group.IsValid() && Group->Documents.Contains(Document))
		{
			return Group;
		}
	}

	return nullptr;
}

int32 SNotepadWidget::FindGroupIndex(TSharedPtr<FNotepadDocumentGroup> Group) const
{
	return DocumentGroups.IndexOfByKey(Group);
}

void SNotepadWidget::CreateNewDocumentGroup(ENotepadDocumentGroupLayout Layout)
{
	TSharedPtr<FNotepadDocumentGroup> SourceGroup = GetOrCreateActiveGroup();
	if (!SourceGroup.IsValid())
	{
		return;
	}

	SyncAllDocumentGroupEditors();
	CurrentGroupLayout = Layout;

	const int32 SourceIndex = FindGroupIndex(SourceGroup);
	TSharedPtr<FNotepadDocumentGroup> NewGroup = CreateDocumentGroup(SourceIndex == INDEX_NONE ? INDEX_NONE : SourceIndex + 1);
	RebuildDocumentGroups();

	if (ActiveDocument.IsValid() && SourceGroup->Documents.Num() > 1)
	{
		MoveDocumentToGroup(ActiveDocument, SourceGroup, NewGroup);
	}
	else
	{
		CreateNewDocumentInGroup(EUNotepadDocumentMode::Text, NewGroup);
	}

	const FText LayoutText = Layout == ENotepadDocumentGroupLayout::Vertical
		? LOCTEXT("VerticalGroupLayout", "vertical")
		: LOCTEXT("HorizontalGroupLayout", "horizontal");
	SetStatus(FText::Format(LOCTEXT("NewDocumentGroupStatus", "Created {0} document group"), LayoutText));
}

bool SNotepadWidget::MoveDocumentToGroup(const TSharedPtr<FNotepadDocument>& Document, TSharedPtr<FNotepadDocumentGroup> SourceGroup, TSharedPtr<FNotepadDocumentGroup> TargetGroup)
{
	if (!Document.IsValid() || !SourceGroup.IsValid() || !TargetGroup.IsValid() || SourceGroup == TargetGroup)
	{
		return false;
	}

	SyncDocumentFromOwningEditor(Document);

	const int32 SourceDocumentIndex = SourceGroup->Documents.IndexOfByKey(Document);
	if (SourceDocumentIndex == INDEX_NONE)
	{
		return false;
	}

	SourceGroup->Documents.Remove(Document);
	TargetGroup->Documents.Add(Document);

	if (SourceGroup->ActiveDocument == Document)
	{
		SourceGroup->ActiveDocument.Reset();
		if (SourceGroup->Documents.Num() > 0)
		{
			SourceGroup->ActiveDocument = SourceGroup->Documents[FMath::Clamp(SourceDocumentIndex, 0, SourceGroup->Documents.Num() - 1)];
		}
	}

	if (SourceGroup->Documents.Num() == 0)
	{
		DocumentGroups.Remove(SourceGroup);
		RebuildDocumentGroups();
	}
	else
	{
		RefreshGroupEditor(SourceGroup);
	}

	SetActiveDocument(Document, TargetGroup);
	return true;
}

void SNotepadWidget::MoveActiveDocumentToAdjacentGroup(int32 Direction)
{
	if (!ActiveDocument.IsValid() || DocumentGroups.Num() < 2)
	{
		SetStatus(LOCTEXT("NoAdjacentGroupStatus", "No other document group is available"), true);
		return;
	}

	TSharedPtr<FNotepadDocumentGroup> SourceGroup = FindGroupForDocument(ActiveDocument);
	const int32 SourceIndex = FindGroupIndex(SourceGroup);
	if (SourceIndex == INDEX_NONE)
	{
		return;
	}

	const int32 GroupCount = DocumentGroups.Num();
	const int32 TargetIndex = (SourceIndex + Direction + GroupCount) % GroupCount;
	TSharedPtr<FNotepadDocumentGroup> TargetGroup = DocumentGroups[TargetIndex];

	const FString DocumentName = ActiveDocument->DisplayName;
	if (MoveDocumentToGroup(ActiveDocument, SourceGroup, TargetGroup))
	{
		SetStatus(FText::Format(LOCTEXT("MovedDocumentGroupStatus", "Moved {0} to another document group"), FText::FromString(DocumentName)));
	}
}

void SNotepadWidget::RebuildDocumentGroups()
{
	if (!DocumentGroupsHost.IsValid())
	{
		return;
	}

	if (DocumentGroups.Num() == 0)
	{
		DocumentGroupsHost->SetContent(SNullWidget::NullWidget);
		return;
	}

	const EOrientation SplitterOrientation = CurrentGroupLayout == ENotepadDocumentGroupLayout::Vertical
		? Orient_Horizontal
		: Orient_Vertical;

	SAssignNew(DocumentGroupsSplitter, SSplitter)
		.Orientation(SplitterOrientation);

	const float SlotValue = 1.0f / static_cast<float>(DocumentGroups.Num());
	for (const TSharedPtr<FNotepadDocumentGroup>& Group : DocumentGroups)
	{
		if (!Group.IsValid())
		{
			continue;
		}

		DocumentGroupsSplitter->AddSlot()
		.Value(SlotValue)
		[
			BuildDocumentGroup(Group)
		];
	}

	DocumentGroupsHost->SetContent(DocumentGroupsSplitter.ToSharedRef());
}

void SNotepadWidget::RebuildGroupTabStrip(TSharedPtr<FNotepadDocumentGroup> Group)
{
	if (Group.IsValid() && Group->TabStripWidget.IsValid())
	{
		Group->TabStripWidget->SetDocuments(Group->Documents, Group->ActiveDocument);
	}
}

void SNotepadWidget::RebuildTabStrip()
{
	for (const TSharedPtr<FNotepadDocumentGroup>& Group : DocumentGroups)
	{
		RebuildGroupTabStrip(Group);
	}
}

void SNotepadWidget::RefreshGroupEditor(TSharedPtr<FNotepadDocumentGroup> Group, bool bFocusEditor)
{
	if (!Group.IsValid() || !Group->EditorWidget.IsValid())
	{
		return;
	}

	if (Group->ActiveDocument.IsValid())
	{
		Group->EditorWidget->SetMode(Group->ActiveDocument->Mode);
		Group->EditorWidget->SetText(Group->ActiveDocument->Content);
	}
	else
	{
		Group->EditorWidget->SetMode(EUNotepadDocumentMode::Text);
		Group->EditorWidget->SetText(FString());
	}

	Group->EditorWidget->SetShowLineNumbers(bShowLineNumbers);
	Group->EditorWidget->SetShowWhitespace(bShowWhitespace);
	Group->EditorWidget->SetTabSize(UUNotepadSettings::Get()->GetClampedTabSize());

	if (bFocusEditor)
	{
		Group->EditorWidget->FocusEditor();
	}
}

void SNotepadWidget::RefreshDocumentControls()
{
	if (ToolbarWidget.IsValid() && ActiveDocument.IsValid())
	{
		ToolbarWidget->SetMode(ActiveDocument->Mode);
		ToolbarWidget->SetLineNumbersEnabled(bShowLineNumbers);
		ToolbarWidget->SetWhitespaceEnabled(bShowWhitespace);
	}
}

FReply SNotepadWidget::OnNewClicked()
{
	CreateNewDocument(EUNotepadDocumentMode::Text);
	return FReply::Handled();
}

FReply SNotepadWidget::OnOpenClicked()
{
	OpenTextFileDialog();
	return FReply::Handled();
}

FReply SNotepadWidget::OnSaveClicked()
{
	SaveActiveDocument(false);
	return FReply::Handled();
}

FReply SNotepadWidget::OnSaveAsClicked()
{
	SaveActiveDocument(true);
	return FReply::Handled();
}

FReply SNotepadWidget::OnCloseActiveTabClicked()
{
	CloseDocument(ActiveDocument);
	return FReply::Handled();
}

FReply SNotepadWidget::OnFormatClicked()
{
	FormatActiveDocument();
	return FReply::Handled();
}

FReply SNotepadWidget::OnValidateClicked()
{
	ValidateActiveDocument();
	return FReply::Handled();
}

FReply SNotepadWidget::OnCompileClicked()
{
	CompileProject();
	return FReply::Handled();
}

FReply SNotepadWidget::OnFindNextClicked()
{
	FindNext(false);
	return FReply::Handled();
}

FReply SNotepadWidget::OnFindPreviousClicked()
{
	FindNext(true);
	return FReply::Handled();
}

FReply SNotepadWidget::OnReplaceClicked()
{
	ReplaceCurrent();
	return FReply::Handled();
}

FReply SNotepadWidget::OnReplaceAllClicked()
{
	ReplaceAll();
	return FReply::Handled();
}

FReply SNotepadWidget::OnCloseFindReplaceClicked()
{
	HideFindReplacePanel();
	return FReply::Handled();
}

FReply SNotepadWidget::OnGoToLineClicked()
{
	GoToLineFromInput();
	return FReply::Handled();
}

FReply SNotepadWidget::OnCloseGoToLineClicked()
{
	HideGoToLinePanel();
	return FReply::Handled();
}

FReply SNotepadWidget::OnToggleLineNumbersClicked()
{
	bShowLineNumbers = !bShowLineNumbers;
	for (const TSharedPtr<FNotepadDocumentGroup>& Group : DocumentGroups)
	{
		if (Group.IsValid() && Group->EditorWidget.IsValid())
		{
			Group->EditorWidget->SetShowLineNumbers(bShowLineNumbers);
		}
	}
	RefreshDocumentControls();
	SetStatus(bShowLineNumbers ? LOCTEXT("LineNumbersShown", "Line numbers shown") : LOCTEXT("LineNumbersHidden", "Line numbers hidden"));
	return FReply::Handled();
}

FReply SNotepadWidget::OnToggleWhitespaceClicked()
{
	bShowWhitespace = !bShowWhitespace;
	for (const TSharedPtr<FNotepadDocumentGroup>& Group : DocumentGroups)
	{
		if (Group.IsValid() && Group->EditorWidget.IsValid())
		{
			Group->EditorWidget->SetShowWhitespace(bShowWhitespace);
		}
	}
	RefreshDocumentControls();
	SetStatus(bShowWhitespace ? LOCTEXT("WhitespaceShown", "Whitespace markers shown") : LOCTEXT("WhitespaceHidden", "Whitespace markers hidden"));
	return FReply::Handled();
}

FReply SNotepadWidget::OnToggleSolutionExplorerClicked()
{
	SyncAllDocumentGroupEditors();
	bShowSolutionExplorer = !bShowSolutionExplorer;
	RebuildWorkspaceContent();
	SetStatus(bShowSolutionExplorer ? LOCTEXT("SolutionExplorerShown", "Solution Explorer shown") : LOCTEXT("SolutionExplorerHidden", "Solution Explorer hidden"));
	return FReply::Handled();
}

FReply SNotepadWidget::OnNewVerticalGroupClicked()
{
	CreateNewDocumentGroup(ENotepadDocumentGroupLayout::Vertical);
	return FReply::Handled();
}

FReply SNotepadWidget::OnNewHorizontalGroupClicked()
{
	CreateNewDocumentGroup(ENotepadDocumentGroupLayout::Horizontal);
	return FReply::Handled();
}

FReply SNotepadWidget::OnDocumentTabClicked(TSharedPtr<FNotepadDocument> Document)
{
	SetActiveDocument(Document);
	return FReply::Handled();
}

FReply SNotepadWidget::OnCloseDocumentClicked(TSharedPtr<FNotepadDocument> Document)
{
	CloseDocument(Document);
	return FReply::Handled();
}

void SNotepadWidget::HandleEditorTextChanged(TSharedPtr<FNotepadDocumentGroup> Group, const FText& NewText)
{
	if (!Group.IsValid() || !Group->ActiveDocument.IsValid())
	{
		return;
	}

	ActiveGroup = Group;
	ActiveDocument = Group->ActiveDocument;

	const bool bWasDirty = ActiveDocument->bDirty;
	const bool bChanged = FNotepadDocumentHistoryService::SetContent(ActiveDocument, NewText.ToString(), !bSuppressEditorChangeHistory);
	if (bChanged && ActiveDocument->bDirty != bWasDirty)
	{
		RebuildTabStrip();
	}
	RefreshDocumentControls();
}

void SNotepadWidget::HandleModeChanged(EUNotepadDocumentMode NewMode)
{
	if (!ActiveDocument.IsValid())
	{
		return;
	}

	ActiveDocument->Mode = NewMode;
	if (ActiveGroup.IsValid() && ActiveGroup->EditorWidget.IsValid())
	{
		ActiveGroup->EditorWidget->SetMode(ActiveDocument->Mode);
	}
	RefreshDocumentControls();
	SetStatus(FText::Format(LOCTEXT("ModeChangedStatus", "Mode changed to {0}"), FText::FromString(FNotepadDocumentUtils::GetModeLabel(ActiveDocument->Mode))));
}

void SNotepadWidget::FormatActiveDocument()
{
	if (!ActiveDocument.IsValid())
	{
		return;
	}

	SyncActiveDocumentFromEditor();

	FString FormattedText = ActiveDocument->Content;
	FString Error;
	if (!FNotepadFormatService::Format(ActiveDocument->Mode, FormattedText, Error))
	{
		SetStatus(FText::FromString(Error), true);
		return;
	}

	if (!FNotepadDocumentHistoryService::SetContent(ActiveDocument, FormattedText, true))
	{
		SetStatus(LOCTEXT("FormatNoChangeStatus", "Document is already formatted"));
		return;
	}

	RefreshAfterDocumentContentChange(ActiveDocument);
	SetStatus(LOCTEXT("FormattedStatus", "Formatted active document"));
}

void SNotepadWidget::ValidateActiveDocument()
{
	if (!ActiveDocument.IsValid())
	{
		return;
	}

	SyncActiveDocumentFromEditor();

	FString Error;
	const ENotepadValidationResult Result = FNotepadFormatService::Validate(ActiveDocument->Mode, ActiveDocument->Content, Error);
	if (Result == ENotepadValidationResult::Invalid)
	{
		SetStatus(FText::FromString(Error), true);
		return;
	}

	if (Result == ENotepadValidationResult::NotSupported)
	{
		SetStatus(FText::Format(
			LOCTEXT("PlainDocumentValidationStatus", "{0}: {1} lines, {2} characters"),
			FText::FromString(FNotepadDocumentUtils::GetModeLabel(ActiveDocument->Mode)),
			FText::AsNumber(FNotepadDocumentUtils::CountLines(ActiveDocument->Content)),
			FText::AsNumber(ActiveDocument->Content.Len())));
		return;
	}

	SetStatus(LOCTEXT("ValidationPassed", "Validation passed"));
}

void SNotepadWidget::CompileProject()
{
	if (ActiveDocument.IsValid())
	{
		SyncActiveDocumentFromEditor();
		if (ActiveDocument->bDirty && !ActiveDocument->FilePath.IsEmpty() && !SaveActiveDocument(false))
		{
			return;
		}
	}

#if WITH_LIVE_CODING
	if (ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(LIVE_CODING_MODULE_NAME))
	{
		if (LiveCoding->IsEnabledByDefault())
		{
			if (LiveCoding->IsCompiling())
			{
				SetStatus(LOCTEXT("LiveCodingCompileAlreadyRunning", "Live Coding compile is already running"), true);
				return;
			}

			LiveCoding->EnableForSession(true);
			if (LiveCoding->IsEnabledForSession())
			{
				LiveCoding->Compile();
				SetStatus(LOCTEXT("LiveCodingCompileStarted", "Live Coding compile started"));
			}
			else
			{
				FText EnableErrorText = LiveCoding->GetEnableErrorText();
				if (EnableErrorText.IsEmpty())
				{
					EnableErrorText = LOCTEXT("LiveCodingEnableFailed", "Live Coding cannot be enabled while hot-reloaded modules are active. Close the editor and build from your IDE before restarting.");
				}

				SetStatus(EnableErrorText, true);
				FMessageDialog::Open(EAppMsgType::Ok, EnableErrorText);
			}
			return;
		}
	}
#endif

	IHotReloadInterface& HotReloadSupport = FModuleManager::LoadModuleChecked<IHotReloadInterface>("HotReload");
	if (HotReloadSupport.IsCurrentlyCompiling())
	{
		SetStatus(LOCTEXT("HotReloadCompileAlreadyRunning", "Compile is already running"), true);
		return;
	}

	HotReloadSupport.DoHotReloadFromEditor(EHotReloadFlags::None);
	SetStatus(LOCTEXT("HotReloadCompileStarted", "Hot reload compile started"));
}

void SNotepadWidget::OpenSettings()
{
	const UUNotepadSettings* Settings = UUNotepadSettings::Get();
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
	}
}

void SNotepadWidget::UndoActiveDocumentChange()
{
	if (!ActiveDocument.IsValid())
	{
		SetStatus(LOCTEXT("NoActiveDocumentForUndo", "No active document"), true);
		return;
	}

	SyncActiveDocumentFromEditor();
	if (!FNotepadDocumentHistoryService::CanUndo(ActiveDocument))
	{
		SetStatus(LOCTEXT("NothingToUndoStatus", "Nothing to undo"), true);
		return;
	}

	FNotepadDocumentHistoryService::Undo(ActiveDocument);
	RefreshAfterDocumentContentChange(ActiveDocument);
	SetStatus(LOCTEXT("UndoStatus", "Undid document change"));
}

void SNotepadWidget::RedoActiveDocumentChange()
{
	if (!ActiveDocument.IsValid())
	{
		SetStatus(LOCTEXT("NoActiveDocumentForRedo", "No active document"), true);
		return;
	}

	SyncActiveDocumentFromEditor();
	if (!FNotepadDocumentHistoryService::CanRedo(ActiveDocument))
	{
		SetStatus(LOCTEXT("NothingToRedoStatus", "Nothing to redo"), true);
		return;
	}

	FNotepadDocumentHistoryService::Redo(ActiveDocument);
	RefreshAfterDocumentContentChange(ActiveDocument);
	SetStatus(LOCTEXT("RedoStatus", "Redid document change"));
}

bool SNotepadWidget::ApplyEditorTextAction(TFunctionRef<bool(SNotepadEditor&)> Action, const FText& SuccessMessage, const FText& NoChangeMessage, bool bRequiresCodeMode, bool bNoChangeIsError)
{
	if (!ActiveDocument.IsValid())
	{
		SetStatus(LOCTEXT("NoActiveDocumentForTextAction", "No active document"), true);
		return false;
	}

	if (bRequiresCodeMode && ActiveDocument->Mode != EUNotepadDocumentMode::Code)
	{
		SetStatus(LOCTEXT("TextActionRequiresCodeMode", "This action is available in Code mode only"), true);
		return false;
	}

	TSharedPtr<SNotepadEditor> Editor = GetActiveEditor();
	if (!Editor.IsValid())
	{
		SetStatus(LOCTEXT("NoActiveEditorForTextAction", "No active editor"), true);
		return false;
	}

	SyncActiveDocumentFromEditor();
	const FString PreviousContent = ActiveDocument->Content;
	bool bActionChangedText = false;
	{
		TGuardValue<bool> SuppressHistoryGuard(bSuppressEditorChangeHistory, true);
		bActionChangedText = Action(*Editor);
	}

	if (!bActionChangedText)
	{
		SetStatus(NoChangeMessage, bNoChangeIsError);
		return false;
	}

	const FString NewContent = Editor->GetText();
	if (NewContent == PreviousContent)
	{
		SetStatus(NoChangeMessage, bNoChangeIsError);
		return false;
	}

	ActiveDocument->Content = PreviousContent;
	FNotepadDocumentHistoryService::SetContent(ActiveDocument, NewContent, true);
	RefreshAfterDocumentContentChange(ActiveDocument);
	UpdateActiveSearchHighlight();
	SetStatus(SuccessMessage);
	return true;
}

void SNotepadWidget::ToggleLineComment()
{
	ApplyEditorTextAction(
		[](SNotepadEditor& Editor)
		{
			return Editor.ToggleLineComment(TEXT("//"));
		},
		LOCTEXT("ToggleLineCommentStatus", "Toggled line comment"),
		LOCTEXT("ToggleLineCommentNoChangeStatus", "No code line comment changed"),
		true,
		true);
}

void SNotepadWidget::DuplicateLineOrSelection()
{
	ApplyEditorTextAction(
		[](SNotepadEditor& Editor)
		{
			return Editor.DuplicateLineOrSelection();
		},
		LOCTEXT("DuplicateLineStatus", "Duplicated line or selection"),
		LOCTEXT("DuplicateLineNoChangeStatus", "Nothing to duplicate"),
		false,
		true);
}

void SNotepadWidget::MoveCurrentLine(int32 Direction)
{
	ApplyEditorTextAction(
		[Direction](SNotepadEditor& Editor)
		{
			return Editor.MoveCurrentLine(Direction);
		},
		Direction < 0 ? LOCTEXT("MoveLineUpStatus", "Moved line up") : LOCTEXT("MoveLineDownStatus", "Moved line down"),
		Direction < 0 ? LOCTEXT("MoveLineUpNoChangeStatus", "Line cannot move up") : LOCTEXT("MoveLineDownNoChangeStatus", "Line cannot move down"),
		false,
		true);
}

void SNotepadWidget::TrimTrailingWhitespace()
{
	ApplyEditorTextAction(
		[](SNotepadEditor& Editor)
		{
			return Editor.TrimTrailingWhitespace();
		},
		LOCTEXT("TrimTrailingWhitespaceStatus", "Trimmed trailing whitespace"),
		LOCTEXT("TrimTrailingWhitespaceNoChangeStatus", "No trailing whitespace found"));
}

void SNotepadWidget::ConvertTabsToSpaces()
{
	ApplyEditorTextAction(
		[](SNotepadEditor& Editor)
		{
			return Editor.ConvertTabsToSpaces();
		},
		LOCTEXT("ConvertTabsToSpacesStatus", "Converted tabs to spaces"),
		LOCTEXT("ConvertTabsToSpacesNoChangeStatus", "No tabs found"));
}

void SNotepadWidget::ConvertSpacesToTabs()
{
	ApplyEditorTextAction(
		[](SNotepadEditor& Editor)
		{
			return Editor.ConvertSpacesToTabs();
		},
		LOCTEXT("ConvertSpacesToTabsStatus", "Converted leading spaces to tabs"),
		LOCTEXT("ConvertSpacesToTabsNoChangeStatus", "No leading spaces to convert"));
}

void SNotepadWidget::EnsureFinalNewline()
{
	ApplyEditorTextAction(
		[](SNotepadEditor& Editor)
		{
			return Editor.EnsureFinalNewline();
		},
		LOCTEXT("EnsureFinalNewlineStatus", "Ensured final newline"),
		LOCTEXT("EnsureFinalNewlineNoChangeStatus", "Document already has a final newline"));
}

void SNotepadWidget::NormalizeLineEndings(ENotepadLineEnding LineEnding)
{
	const FText LineEndingLabel = LineEnding == ENotepadLineEnding::CRLF ? LOCTEXT("CrlfLineEndingLabel", "CRLF") : LOCTEXT("LfLineEndingLabel", "LF");
	ApplyEditorTextAction(
		[LineEnding](SNotepadEditor& Editor)
		{
			return Editor.NormalizeLineEndings(LineEnding);
		},
		FText::Format(LOCTEXT("NormalizeLineEndingsStatus", "Converted line endings to {0}"), LineEndingLabel),
		FText::Format(LOCTEXT("NormalizeLineEndingsNoChangeStatus", "Line endings are already {0}"), LineEndingLabel));
}

void SNotepadWidget::ShowFindReplacePanel(bool bFocusReplace)
{
	bShowFindReplace = true;

	if (TSharedPtr<SNotepadEditor> Editor = GetActiveEditor())
	{
		const FString SelectedText = Editor->GetSelectedText();
		if (!SelectedText.IsEmpty() && !SelectedText.Contains(TEXT("\n")) && !SelectedText.Contains(TEXT("\r")))
		{
			FindText = FText::FromString(SelectedText);
		}
	}

	TSharedPtr<SEditableTextBox> TextBoxToFocus = bFocusReplace ? ReplaceTextBox : FindTextBox;
	if (TextBoxToFocus.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(TextBoxToFocus, EFocusCause::SetDirectly);
		TextBoxToFocus->SelectAllText();
	}

	SetStatus(LOCTEXT("FindReplaceShownStatus", "Find / Replace shown"));
}

void SNotepadWidget::HideFindReplacePanel()
{
	bShowFindReplace = false;

	if (TSharedPtr<SNotepadEditor> Editor = GetActiveEditor())
	{
		Editor->ClearSearch();
		Editor->FocusEditor();
	}

	SetStatus(LOCTEXT("FindReplaceHiddenStatus", "Find / Replace hidden"));
}

void SNotepadWidget::ShowGoToLinePanel()
{
	bShowGoToLine = true;
	if (GoToLineText.ToString().IsEmpty())
	{
		GoToLineText = FText::FromString(TEXT("1"));
	}

	if (GoToLineTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(GoToLineTextBox, EFocusCause::SetDirectly);
		GoToLineTextBox->SelectAllText();
	}

	SetStatus(LOCTEXT("GoToLineShownStatus", "Go To Line shown"));
}

void SNotepadWidget::HideGoToLinePanel()
{
	bShowGoToLine = false;

	if (TSharedPtr<SNotepadEditor> Editor = GetActiveEditor())
	{
		Editor->FocusEditor();
	}

	SetStatus(LOCTEXT("GoToLineHiddenStatus", "Go To Line hidden"));
}

void SNotepadWidget::GoToLineFromInput()
{
	TSharedPtr<SNotepadEditor> Editor = GetActiveEditor();
	if (!Editor.IsValid())
	{
		SetStatus(LOCTEXT("NoEditorForGoToLineStatus", "No active editor to navigate"), true);
		return;
	}

	FString LineText = GoToLineText.ToString();
	LineText.TrimStartAndEndInline();

	int32 LineNumber = 0;
	if (!LexTryParseString(LineNumber, *LineText) || LineNumber < 1)
	{
		SetStatus(LOCTEXT("InvalidGoToLineStatus", "Enter a valid line number"), true);
		return;
	}

	const int32 LineCount = Editor->GetLineCount();
	if (!Editor->GoToLine(LineNumber))
	{
		SetStatus(FText::Format(LOCTEXT("GoToLineOutOfRangeStatus", "Line must be between 1 and {0}"), FText::AsNumber(LineCount)), true);
		return;
	}

	bShowGoToLine = false;
	SetStatus(FText::Format(LOCTEXT("GoToLineStatus", "Moved to line {0}"), FText::AsNumber(LineNumber)));
}

bool SNotepadWidget::OpenHeaderSourcePair()
{
	if (!ActiveDocument.IsValid())
	{
		SetStatus(LOCTEXT("NoDocumentForHeaderSourcePairStatus", "No active document"), true);
		return false;
	}

	SyncActiveDocumentFromEditor();

	if (ActiveDocument->FilePath.IsEmpty())
	{
		SetStatus(LOCTEXT("UnsavedHeaderSourcePairStatus", "Save the document before opening its header/source pair"), true);
		return false;
	}

	const FString PairPath = FindHeaderSourcePairPath(ActiveDocument->FilePath);
	if (PairPath.IsEmpty())
	{
		SetStatus(FText::Format(LOCTEXT("HeaderSourcePairMissingStatus", "No header/source pair found for {0}"), FText::FromString(ActiveDocument->DisplayName)), true);
		return false;
	}

	return LoadDocumentFromFile(PairPath);
}

bool SNotepadWidget::FindNext(bool bReverse)
{
	if (!bShowFindReplace)
	{
		ShowFindReplacePanel(false);
	}

	const FString SearchText = FindText.ToString();
	if (SearchText.IsEmpty())
	{
		SetStatus(LOCTEXT("FindTextEmptyStatus", "Enter text to find"), true);
		return false;
	}

	TSharedPtr<SNotepadEditor> Editor = GetActiveEditor();
	if (!Editor.IsValid())
	{
		SetStatus(LOCTEXT("NoEditorForFindStatus", "No active editor to search"), true);
		return false;
	}

	SyncActiveDocumentFromEditor();

	const ESearchCase::Type SearchCase = GetFindSearchCase();
	Editor->BeginSearch(SearchText, SearchCase, bReverse);

	if (!Editor->IsSelectedTextMatching(SearchText, SearchCase))
	{
		SetStatus(FText::Format(LOCTEXT("FindNoMatchesStatus", "No matches for \"{0}\""), FindText), true);
		return false;
	}

	SetStatus(FText::Format(
		bReverse ? LOCTEXT("FindPreviousStatus", "Found previous \"{0}\"") : LOCTEXT("FindNextStatus", "Found next \"{0}\""),
		FindText));
	return true;
}

void SNotepadWidget::ReplaceCurrent()
{
	if (!bShowFindReplace)
	{
		ShowFindReplacePanel(true);
	}

	const FString SearchText = FindText.ToString();
	if (SearchText.IsEmpty())
	{
		SetStatus(LOCTEXT("ReplaceFindTextEmptyStatus", "Enter text to replace"), true);
		return;
	}

	TSharedPtr<SNotepadEditor> Editor = GetActiveEditor();
	if (!Editor.IsValid())
	{
		SetStatus(LOCTEXT("NoEditorForReplaceStatus", "No active editor to replace in"), true);
		return;
	}

	SyncActiveDocumentFromEditor();

	const ESearchCase::Type SearchCase = GetFindSearchCase();
	const FString ReplacementText = ReplaceText.ToString();
	if (!Editor->ReplaceSelectedText(SearchText, ReplacementText, SearchCase))
	{
		Editor->BeginSearch(SearchText, SearchCase, false);
		if (!Editor->ReplaceSelectedText(SearchText, ReplacementText, SearchCase))
		{
			SetStatus(FText::Format(LOCTEXT("ReplaceNoMatchesStatus", "No matches for \"{0}\""), FindText), true);
			return;
		}
	}

	SyncActiveDocumentFromEditor();
	if (ActiveDocument.IsValid())
	{
		FNotepadDocumentHistoryService::UpdateDirtyState(ActiveDocument);
	}

	RebuildTabStrip();
	RefreshDocumentControls();
	Editor->BeginSearch(SearchText, SearchCase, false);
	SetStatus(FText::Format(LOCTEXT("ReplaceCurrentStatus", "Replaced \"{0}\""), FindText));
}

void SNotepadWidget::ReplaceAll()
{
	if (!bShowFindReplace)
	{
		ShowFindReplacePanel(true);
	}

	const FString SearchText = FindText.ToString();
	if (SearchText.IsEmpty())
	{
		SetStatus(LOCTEXT("ReplaceAllFindTextEmptyStatus", "Enter text to replace"), true);
		return;
	}

	TSharedPtr<SNotepadEditor> Editor = GetActiveEditor();
	if (!Editor.IsValid())
	{
		SetStatus(LOCTEXT("NoEditorForReplaceAllStatus", "No active editor to replace in"), true);
		return;
	}

	SyncActiveDocumentFromEditor();
	const FString PreviousContent = ActiveDocument.IsValid() ? ActiveDocument->Content : FString();

	const int32 ReplacementCount = Editor->ReplaceAllText(SearchText, ReplaceText.ToString(), GetFindSearchCase());
	if (ReplacementCount == 0)
	{
		SetStatus(FText::Format(LOCTEXT("ReplaceAllNoMatchesStatus", "No matches for \"{0}\""), FindText), true);
		return;
	}

	SyncActiveDocumentFromEditor();
	if (ActiveDocument.IsValid())
	{
		const FString NewContent = ActiveDocument->Content;
		ActiveDocument->Content = PreviousContent;
		FNotepadDocumentHistoryService::SetContent(ActiveDocument, NewContent, true);
	}

	RebuildTabStrip();
	RefreshDocumentControls();
	UpdateActiveSearchHighlight();
	SetStatus(FText::Format(LOCTEXT("ReplaceAllStatus", "Replaced {0} matches"), FText::AsNumber(ReplacementCount)));
}

void SNotepadWidget::UpdateActiveSearchHighlight()
{
	TSharedPtr<SNotepadEditor> Editor = GetActiveEditor();
	if (!Editor.IsValid())
	{
		return;
	}

	const FString SearchText = FindText.ToString();
	if (SearchText.IsEmpty())
	{
		Editor->ClearSearch();
		return;
	}

	Editor->BeginSearch(SearchText, GetFindSearchCase(), false);
}

void SNotepadWidget::HandleFindTextChanged(const FText& NewText)
{
	FindText = NewText;
	UpdateActiveSearchHighlight();
}

void SNotepadWidget::HandleReplaceTextChanged(const FText& NewText)
{
	ReplaceText = NewText;
}

void SNotepadWidget::HandleFindTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	FindText = NewText;
	if (CommitType == ETextCommit::OnEnter)
	{
		FindNext(false);
	}
}

void SNotepadWidget::HandleReplaceTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	ReplaceText = NewText;
	if (CommitType == ETextCommit::OnEnter)
	{
		ReplaceCurrent();
	}
}

void SNotepadWidget::HandleGoToLineTextChanged(const FText& NewText)
{
	GoToLineText = NewText;
}

void SNotepadWidget::HandleGoToLineTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	GoToLineText = NewText;
	if (CommitType == ETextCommit::OnEnter)
	{
		GoToLineFromInput();
	}
}

void SNotepadWidget::HandleFindCaseSensitiveChanged(ECheckBoxState NewState)
{
	bFindCaseSensitive = NewState == ECheckBoxState::Checked;
	UpdateActiveSearchHighlight();
}

FText SNotepadWidget::GetFindText() const
{
	return FindText;
}

FText SNotepadWidget::GetReplaceText() const
{
	return ReplaceText;
}

FText SNotepadWidget::GetGoToLineText() const
{
	return GoToLineText;
}

EVisibility SNotepadWidget::GetFindReplaceVisibility() const
{
	return bShowFindReplace ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SNotepadWidget::GetGoToLineVisibility() const
{
	return bShowGoToLine ? EVisibility::Visible : EVisibility::Collapsed;
}

ECheckBoxState SNotepadWidget::GetFindCaseSensitiveState() const
{
	return bFindCaseSensitive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

TSharedPtr<SNotepadEditor> SNotepadWidget::GetActiveEditor() const
{
	return ActiveGroup.IsValid() ? ActiveGroup->EditorWidget : nullptr;
}

ESearchCase::Type SNotepadWidget::GetFindSearchCase() const
{
	return bFindCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
}

FString SNotepadWidget::FindHeaderSourcePairPath(const FString& FilePath) const
{
	FString NormalizedPath = FNotepadFileService::NormalizeFilePath(FilePath);
	FPaths::NormalizeFilename(NormalizedPath);

	const bool bHeader = IsHeaderSourceFile(NormalizedPath);
	const bool bSource = IsSourceSourceFile(NormalizedPath);
	if (!bHeader && !bSource)
	{
		return FString();
	}

	const TArray<FString> TargetExtensions = bHeader
		? TArray<FString>{ TEXT("cpp"), TEXT("cc"), TEXT("cxx") }
		: TArray<FString>{ TEXT("h"), TEXT("hh"), TEXT("hpp"), TEXT("hxx") };

	const FString BaseFilename = FPaths::GetBaseFilename(NormalizedPath);
	TArray<FString> CandidatePaths;

	const auto AddCandidate = [&CandidatePaths](const FString& BasePathWithoutExtension, const FString& Extension)
	{
		FString CandidatePath = BasePathWithoutExtension + TEXT(".") + Extension;
		FPaths::NormalizeFilename(CandidatePath);
		CandidatePaths.AddUnique(CandidatePath);
	};

	const FString SameDirectoryBase = FPaths::GetPath(NormalizedPath) / BaseFilename;
	for (const FString& Extension : TargetExtensions)
	{
		AddCandidate(SameDirectoryBase, Extension);
	}

	const FString SiblingPath = bHeader
		? ReplacePathSegment(NormalizedPath, TEXT("Public"), TEXT("Private"))
		: ReplacePathSegment(NormalizedPath, TEXT("Private"), TEXT("Public"));
	if (!SiblingPath.Equals(NormalizedPath, ESearchCase::IgnoreCase))
	{
		const FString SiblingBase = FPaths::GetPath(SiblingPath) / FPaths::GetBaseFilename(SiblingPath);
		for (const FString& Extension : TargetExtensions)
		{
			AddCandidate(SiblingBase, Extension);
		}
	}

	for (const FString& CandidatePath : CandidatePaths)
	{
		if (IFileManager::Get().FileExists(*CandidatePath))
		{
			return CandidatePath;
		}
	}

	const FString ModuleSourceRoot = FindModuleSourceRoot(NormalizedPath);
	if (ModuleSourceRoot.IsEmpty() || !IFileManager::Get().DirectoryExists(*ModuleSourceRoot))
	{
		return FString();
	}

	TArray<FString> RecursiveCandidates;
	for (const FString& Extension : TargetExtensions)
	{
		const FString Pattern = BaseFilename + TEXT(".") + Extension;
		IFileManager::Get().FindFilesRecursive(RecursiveCandidates, *ModuleSourceRoot, *Pattern, true, false, false);
	}

	RecursiveCandidates.Sort([](const FString& Left, const FString& Right)
	{
		return Left.Compare(Right, ESearchCase::IgnoreCase) < 0;
	});

	for (FString& CandidatePath : RecursiveCandidates)
	{
		CandidatePath = FNotepadFileService::NormalizeFilePath(CandidatePath);
		if (!CandidatePath.Equals(NormalizedPath, ESearchCase::IgnoreCase) && IFileManager::Get().FileExists(*CandidatePath))
		{
			return CandidatePath;
		}
	}

	return FString();
}

TSharedPtr<FNotepadSourceTreeItem> SNotepadWidget::FindOrAddSourceTreeDirectory(TSharedPtr<FNotepadSourceTreeItem> Parent, const FString& DirectoryName)
{
	if (!Parent.IsValid() || DirectoryName.IsEmpty())
	{
		return nullptr;
	}

	for (const TSharedPtr<FNotepadSourceTreeItem>& Child : Parent->Children)
	{
		if (Child.IsValid() && Child->bDirectory && Child->Name.Equals(DirectoryName, ESearchCase::IgnoreCase))
		{
			return Child;
		}
	}

	TSharedPtr<FNotepadSourceTreeItem> DirectoryItem = MakeShared<FNotepadSourceTreeItem>();
	DirectoryItem->Name = DirectoryName;
	DirectoryItem->FilePath = Parent->FilePath / DirectoryName;
	FPaths::NormalizeDirectoryName(DirectoryItem->FilePath);
	DirectoryItem->bDirectory = true;

	Parent->Children.Add(DirectoryItem);
	return DirectoryItem;
}

bool SNotepadWidget::IsSolutionExplorerSourceFile(const FString& FilePath)
{
	return UUNotepadSettings::Get()->IsSourceFileExtension(FPaths::GetExtension(FilePath));
}

bool SNotepadWidget::IsHeaderSourceFile(const FString& FilePath)
{
	const FString Extension = FPaths::GetExtension(FilePath).ToLower();
	return Extension == TEXT("h")
		|| Extension == TEXT("hh")
		|| Extension == TEXT("hpp")
		|| Extension == TEXT("hxx")
		|| Extension == TEXT("inl")
		|| Extension == TEXT("ipp");
}

bool SNotepadWidget::IsSourceSourceFile(const FString& FilePath)
{
	const FString Extension = FPaths::GetExtension(FilePath).ToLower();
	return Extension == TEXT("cpp")
		|| Extension == TEXT("cc")
		|| Extension == TEXT("cxx");
}

FString SNotepadWidget::ReplacePathSegment(const FString& FilePath, const FString& FromSegment, const FString& ToSegment)
{
	FString NormalizedPath = FilePath;
	FPaths::NormalizeFilename(NormalizedPath);

	const FString FromPattern = TEXT("/") + FromSegment + TEXT("/");
	const FString ToPattern = TEXT("/") + ToSegment + TEXT("/");
	NormalizedPath.ReplaceInline(*FromPattern, *ToPattern, ESearchCase::IgnoreCase);
	return NormalizedPath;
}

FString SNotepadWidget::FindModuleSourceRoot(const FString& FilePath)
{
	FString NormalizedPath = FilePath;
	FPaths::NormalizeFilename(NormalizedPath);

	TArray<FString> PathParts;
	NormalizedPath.ParseIntoArray(PathParts, TEXT("/"), true);

	for (int32 Index = PathParts.Num() - 1; Index >= 0; --Index)
	{
		if (!PathParts[Index].Equals(TEXT("Source"), ESearchCase::IgnoreCase) || Index + 1 >= PathParts.Num())
		{
			continue;
		}

		FString ModuleSourceRoot;
		for (int32 PathIndex = 0; PathIndex <= Index + 1; ++PathIndex)
		{
			if (!ModuleSourceRoot.IsEmpty())
			{
				ModuleSourceRoot += TEXT("/");
			}
			ModuleSourceRoot += PathParts[PathIndex];
		}

		return ModuleSourceRoot;
	}

	return FPaths::GetPath(NormalizedPath);
}

void SNotepadWidget::SortSourceTreeChildren(TSharedPtr<FNotepadSourceTreeItem> Item)
{
	if (!Item.IsValid())
	{
		return;
	}

	Item->Children.Sort([](const TSharedPtr<FNotepadSourceTreeItem>& Left, const TSharedPtr<FNotepadSourceTreeItem>& Right)
	{
		if (!Left.IsValid())
		{
			return false;
		}
		if (!Right.IsValid())
		{
			return true;
		}
		if (Left->bDirectory != Right->bDirectory)
		{
			return Left->bDirectory;
		}

		return Left->Name.Compare(Right->Name, ESearchCase::IgnoreCase) < 0;
	});

	for (const TSharedPtr<FNotepadSourceTreeItem>& Child : Item->Children)
	{
		SortSourceTreeChildren(Child);
	}
}

int32 SNotepadWidget::CountSourceTreeFiles(const TSharedPtr<FNotepadSourceTreeItem>& Item)
{
	if (!Item.IsValid())
	{
		return 0;
	}

	if (!Item->bDirectory)
	{
		return 1;
	}

	int32 FileCount = 0;
	for (const TSharedPtr<FNotepadSourceTreeItem>& Child : Item->Children)
	{
		FileCount += CountSourceTreeFiles(Child);
	}
	return FileCount;
}

void SNotepadWidget::SetStatus(const FText& Message, bool bIsError)
{
	LastStatusMessage = Message;
	bLastStatusIsError = bIsError;
}

FText SNotepadWidget::GetStatusText() const
{
	if (!ActiveDocument.IsValid())
	{
		return LOCTEXT("NoActiveDocumentStatus", "No document");
	}

	const FString DirtyText = ActiveDocument->bDirty ? TEXT("modified") : TEXT("saved");
	const FString PathText = ActiveDocument->FilePath.IsEmpty() ? TEXT("unsaved") : ActiveDocument->FilePath;

	return FText::Format(
		LOCTEXT("DocumentStatusFormat", "{0} | {1} | {2} | {3} lines | {4}"),
		FText::FromString(ActiveDocument->DisplayName),
		FText::FromString(FNotepadDocumentUtils::GetModeLabel(ActiveDocument->Mode)),
		FText::FromString(DirtyText),
		FText::AsNumber(FNotepadDocumentUtils::CountLines(ActiveDocument->Content)),
		FText::FromString(PathText));
}

FText SNotepadWidget::GetLastMessageText() const
{
	return LastStatusMessage;
}

FSlateColor SNotepadWidget::GetLastMessageColor() const
{
	return FSlateColor(bLastStatusIsError ? FLinearColor(0.95f, 0.28f, 0.25f, 1.0f) : FLinearColor(0.46f, 0.78f, 0.52f, 1.0f));
}

#undef LOCTEXT_NAMESPACE
