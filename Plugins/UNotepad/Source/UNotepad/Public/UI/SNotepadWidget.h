// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Types/NotepadDocumentTypes.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SCompoundWidget.h"

class FMenuBuilder;
class FUICommandList;
class ITableRow;
class SBorder;
class SEditableTextBox;
class SNotepadEditor;
class SNotepadTabStrip;
class SNotepadToolbar;
class SSplitter;
class STableViewBase;

template<typename ItemType>
class STreeView;

enum class ENotepadLineEnding : uint8;

enum class ENotepadDocumentGroupLayout : uint8
{
	Vertical,
	Horizontal
};

struct FNotepadDocumentGroup final
{
	TArray<TSharedPtr<FNotepadDocument>> Documents;
	TSharedPtr<FNotepadDocument> ActiveDocument;
	TSharedPtr<SNotepadTabStrip> TabStripWidget;
	TSharedPtr<SNotepadEditor> EditorWidget;
};

struct FNotepadSourceTreeItem final
{
	FString Name;
	FString FilePath;
	bool bDirectory = false;
	TArray<TSharedPtr<FNotepadSourceTreeItem>> Children;
};

class UNOTEPAD_API SNotepadWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNotepadWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	bool OpenFile(const FString& FilePath);

private:
	FString DocumentsRootPath;
	TArray<TSharedPtr<FNotepadDocument>> Documents;
	TSharedPtr<FNotepadDocument> ActiveDocument;
	TArray<TSharedPtr<FNotepadDocumentGroup>> DocumentGroups;
	TSharedPtr<FNotepadDocumentGroup> ActiveGroup;

	TSharedPtr<SNotepadToolbar> ToolbarWidget;
	TSharedPtr<FUICommandList> CommandList;
	TSharedPtr<SBorder> WorkspaceHost;
	TSharedPtr<SBorder> DocumentGroupsHost;
	TSharedPtr<SSplitter> DocumentGroupsSplitter;
	TSharedPtr<SEditableTextBox> FindTextBox;
	TSharedPtr<SEditableTextBox> ReplaceTextBox;
	TSharedPtr<SEditableTextBox> GoToLineTextBox;
	TArray<TSharedPtr<FNotepadSourceTreeItem>> SourceTreeRoots;
	TSharedPtr<STreeView<TSharedPtr<FNotepadSourceTreeItem>>> SourceTreeView;

	FText LastStatusMessage;
	FText FindText;
	FText ReplaceText;
	FText GoToLineText;
	bool bLastStatusIsError = false;
	int32 UntitledCounter = 1;
	bool bShowLineNumbers = true;
	bool bShowWhitespace = true;
	bool bShowSolutionExplorer = true;
	bool bShowFindReplace = false;
	bool bShowGoToLine = false;
	bool bFindCaseSensitive = false;
	bool bSuppressEditorChangeHistory = false;
	ENotepadDocumentGroupLayout CurrentGroupLayout = ENotepadDocumentGroupLayout::Vertical;

	void BindCommands();
	FReply HandleShortcutKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	TSharedPtr<FNotepadDocument> CreateNewDocumentInGroup(EUNotepadDocumentMode Mode, TSharedPtr<FNotepadDocumentGroup> Group);
	void CreateNewDocument(EUNotepadDocumentMode Mode = EUNotepadDocumentMode::Text);
	void OpenTextFileDialog();
	bool LoadDocumentFromFile(const FString& FilePath);
	bool SaveActiveDocument(bool bSaveAs);
	bool SaveDocument(const TSharedPtr<FNotepadDocument>& Document, bool bSaveAs);
	bool CloseDocument(const TSharedPtr<FNotepadDocument>& Document);
	void SetActiveDocument(const TSharedPtr<FNotepadDocument>& Document, TSharedPtr<FNotepadDocumentGroup> Group = nullptr);
	void SetActiveGroup(TSharedPtr<FNotepadDocumentGroup> Group, bool bFocusEditor = false);
	void SyncActiveDocumentFromEditor();
	void SyncDocumentFromGroupEditor(TSharedPtr<FNotepadDocumentGroup> Group);
	void SyncDocumentFromOwningEditor(const TSharedPtr<FNotepadDocument>& Document);
	void SyncAllDocumentGroupEditors();
	void RefreshAfterDocumentContentChange(const TSharedPtr<FNotepadDocument>& Document, bool bFocusEditor = true);

	void FillFileMenu(FMenuBuilder& MenuBuilder);
	void FillToolsMenu(FMenuBuilder& MenuBuilder);
	void FillWindowMenu(FMenuBuilder& MenuBuilder);
	TSharedRef<SWidget> CreateStatusBar();
	TSharedRef<SWidget> BuildFindReplacePanel();
	TSharedRef<SWidget> BuildGoToLinePanel();
	TSharedRef<SWidget> BuildWorkspaceContent();
	TSharedRef<SWidget> BuildSolutionExplorer();
	TSharedRef<SWidget> BuildDocumentGroup(TSharedPtr<FNotepadDocumentGroup> Group);
	void RebuildWorkspaceContent();
	void RefreshSolutionExplorer();
	TSharedPtr<FNotepadSourceTreeItem> BuildSourceTreeFromDirectory(const FString& DirectoryPath, const FString& DisplayName) const;
	void AddFileToSourceTree(TSharedPtr<FNotepadSourceTreeItem> Root, const FString& RootPath, const FString& FilePath) const;
	TSharedRef<ITableRow> GenerateSourceTreeRow(TSharedPtr<FNotepadSourceTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void GetSourceTreeChildren(TSharedPtr<FNotepadSourceTreeItem> Item, TArray<TSharedPtr<FNotepadSourceTreeItem>>& OutChildren) const;
	void HandleSourceTreeDoubleClick(TSharedPtr<FNotepadSourceTreeItem> Item);
	FReply OnRefreshSolutionExplorerClicked();
	TSharedPtr<FNotepadDocumentGroup> CreateDocumentGroup(int32 InsertIndex = INDEX_NONE);
	TSharedPtr<FNotepadDocumentGroup> GetOrCreateActiveGroup();
	TSharedPtr<FNotepadDocumentGroup> FindGroupForDocument(const TSharedPtr<FNotepadDocument>& Document) const;
	int32 FindGroupIndex(TSharedPtr<FNotepadDocumentGroup> Group) const;
	void CreateNewDocumentGroup(ENotepadDocumentGroupLayout Layout);
	bool MoveDocumentToGroup(const TSharedPtr<FNotepadDocument>& Document, TSharedPtr<FNotepadDocumentGroup> SourceGroup, TSharedPtr<FNotepadDocumentGroup> TargetGroup);
	void MoveActiveDocumentToAdjacentGroup(int32 Direction);
	void RebuildDocumentGroups();
	void RebuildGroupTabStrip(TSharedPtr<FNotepadDocumentGroup> Group);
	void RebuildTabStrip();
	void RefreshGroupEditor(TSharedPtr<FNotepadDocumentGroup> Group, bool bFocusEditor = false);
	void RefreshDocumentControls();

	FReply OnNewClicked();
	FReply OnOpenClicked();
	FReply OnSaveClicked();
	FReply OnSaveAsClicked();
	FReply OnCloseActiveTabClicked();
	FReply OnFormatClicked();
	FReply OnValidateClicked();
	FReply OnCompileClicked();
	FReply OnFindNextClicked();
	FReply OnFindPreviousClicked();
	FReply OnReplaceClicked();
	FReply OnReplaceAllClicked();
	FReply OnCloseFindReplaceClicked();
	FReply OnGoToLineClicked();
	FReply OnCloseGoToLineClicked();
	FReply OnToggleLineNumbersClicked();
	FReply OnToggleWhitespaceClicked();
	FReply OnToggleSolutionExplorerClicked();
	FReply OnNewVerticalGroupClicked();
	FReply OnNewHorizontalGroupClicked();
	FReply OnDocumentTabClicked(TSharedPtr<FNotepadDocument> Document);
	FReply OnCloseDocumentClicked(TSharedPtr<FNotepadDocument> Document);

	void HandleEditorTextChanged(TSharedPtr<FNotepadDocumentGroup> Group, const FText& NewText);
	void HandleModeChanged(EUNotepadDocumentMode NewMode);

	void FormatActiveDocument();
	void ValidateActiveDocument();
	void CompileProject();
	void OpenSettings();
	void UndoActiveDocumentChange();
	void RedoActiveDocumentChange();
	bool ApplyEditorTextAction(TFunctionRef<bool(SNotepadEditor&)> Action, const FText& SuccessMessage, const FText& NoChangeMessage, bool bRequiresCodeMode = false, bool bNoChangeIsError = false);
	void ToggleLineComment();
	void DuplicateLineOrSelection();
	void MoveCurrentLine(int32 Direction);
	void TrimTrailingWhitespace();
	void ConvertTabsToSpaces();
	void ConvertSpacesToTabs();
	void EnsureFinalNewline();
	void NormalizeLineEndings(ENotepadLineEnding LineEnding);
	void ShowFindReplacePanel(bool bFocusReplace = false);
	void HideFindReplacePanel();
	void ShowGoToLinePanel();
	void HideGoToLinePanel();
	void GoToLineFromInput();
	bool OpenHeaderSourcePair();
	bool FindNext(bool bReverse);
	void ReplaceCurrent();
	void ReplaceAll();
	void UpdateActiveSearchHighlight();
	void HandleFindTextChanged(const FText& NewText);
	void HandleReplaceTextChanged(const FText& NewText);
	void HandleFindTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void HandleReplaceTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void HandleGoToLineTextChanged(const FText& NewText);
	void HandleGoToLineTextCommitted(const FText& NewText, ETextCommit::Type CommitType);
	void HandleFindCaseSensitiveChanged(ECheckBoxState NewState);
	FText GetFindText() const;
	FText GetReplaceText() const;
	FText GetGoToLineText() const;
	EVisibility GetFindReplaceVisibility() const;
	EVisibility GetGoToLineVisibility() const;
	ECheckBoxState GetFindCaseSensitiveState() const;
	TSharedPtr<SNotepadEditor> GetActiveEditor() const;
	ESearchCase::Type GetFindSearchCase() const;
	FString FindHeaderSourcePairPath(const FString& FilePath) const;

	void SetStatus(const FText& Message, bool bIsError = false);
	FText GetStatusText() const;
	FText GetLastMessageText() const;
	FSlateColor GetLastMessageColor() const;

	static TSharedPtr<FNotepadSourceTreeItem> FindOrAddSourceTreeDirectory(TSharedPtr<FNotepadSourceTreeItem> Parent, const FString& DirectoryName);
	static bool IsSolutionExplorerSourceFile(const FString& FilePath);
	static bool IsHeaderSourceFile(const FString& FilePath);
	static bool IsSourceSourceFile(const FString& FilePath);
	static FString ReplacePathSegment(const FString& FilePath, const FString& FromSegment, const FString& ToSegment);
	static FString FindModuleSourceRoot(const FString& FilePath);
	static void SortSourceTreeChildren(TSharedPtr<FNotepadSourceTreeItem> Item);
	static int32 CountSourceTreeFiles(const TSharedPtr<FNotepadSourceTreeItem>& Item);
};
