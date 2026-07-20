// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "Styling/SlateTypes.h"
#include "Types/NotepadDocumentTypes.h"
#include "Widgets/SCompoundWidget.h"

class FNotepadSyntaxHighlighterMarshaller;
class SMultiLineEditableTextBox;
class SNotepadSyncedText;
class SScrollBar;
class SWidget;

enum class ENotepadLineEnding : uint8;



/**
 * @brief Main text editor widget used by UNotepad.
 *
 * Provides text editing, syntax highlighting, search and replace,
 * synchronized line numbers, and various document editing utilities.
 */
class UNOTEPAD_API SNotepadEditor final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNotepadEditor) {}

		/** Called whenever the document text changes. */
		SLATE_EVENT(FOnTextChanged, OnTextChanged)

		/** Called before the editor processes shortcut keys. */
		SLATE_EVENT(FOnKeyDown, OnShortcutKeyDown)
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the editor widget.
	 *
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs);
	
	/**
	 * @brief Updates the editor every frame.
	 *
	 * @param AllottedGeometry Geometry allocated to the widget.
	 * @param InCurrentTime Current application time.
	 * @param InDeltaTime Time elapsed since the previous frame.
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/**
	 * @brief Replaces the document contents.
	 *
	 * @param NewText New text to display.
	 */
	void SetText(const FString& NewText);

	/**
	 * @brief Gets the current document text.
	 *
	 * @return Document text.
	 */
	FString GetText() const;

	/**
	 * @brief Sets the active document mode.
	 *
	 * @param NewMode New document mode.
	 */
	void SetMode(EUNotepadDocumentMode NewMode);

	/**
	 * @brief Gives keyboard focus to the editor.
	 */
	void FocusEditor();

	/**
	 * @brief Shows or hides line numbers.
	 *
	 * @param bShow True to display line numbers.
	 */
	void SetShowLineNumbers(bool bShow);

	/**
	 * @brief Shows or hides whitespace characters.
	 *
	 * @param bShow True to render whitespace characters.
	 */
	void SetShowWhitespace(bool bShow);

	/**
	 * @brief Sets the editor tab width.
	 *
	 * @param InTabSize Number of spaces represented by a tab.
	 */
	void SetTabSize(int32 InTabSize);

	/**
	 * @brief Returns whether line numbers are currently visible.
	 *
	 * @return True if line numbers are enabled.
	 */
	bool IsShowingLineNumbers() const;

	/**
	 * @brief Returns whether whitespace characters are currently visible.
	 *
	 * @return True if whitespace visualization is enabled.
	 */
	bool IsShowingWhitespace() const;

	/**
	 * @brief Returns the total number of lines.
	 *
	 * @return Number of lines in the document.
	 */
	int32 GetLineCount() const;

	/**
	 * @brief Moves the caret to the specified line.
	 *
	 * @param LineNumber One-based line number.
	 *
	 * @return True if the line exists.
	 */
	bool GoToLine(int32 LineNumber);

	/**
	 * @brief Starts a search operation.
	 *
	 * @param SearchText Text to search for.
	 * @param SearchCase Search case sensitivity.
	 * @param bReverse True to search backwards.
	 */
	void BeginSearch(const FString& SearchText, ESearchCase::Type SearchCase, bool bReverse);

	/**
	 * @brief Continues the current search.
	 *
	 * @param bReverse True to search backwards.
	 */
	void AdvanceSearch(bool bReverse);

	/**
	 * @brief Clears the current search state.
	 */
	void ClearSearch();

	/**
	 * @brief Returns the currently selected text.
	 *
	 * @return Selected text.
	 */
	FString GetSelectedText() const;

	/**
	 * @brief Checks whether the current selection matches the specified text.
	 *
	 * @param SearchText Text to compare.
	 * @param SearchCase Search case sensitivity.
	 *
	 * @return True if the selection matches.
	 */
	bool IsSelectedTextMatching(const FString& SearchText, ESearchCase::Type SearchCase) const;

	/**
	 * @brief Replaces the selected occurrence.
	 *
	 * @param SearchText Expected selected text.
	 * @param ReplacementText Replacement text.
	 * @param SearchCase Search case sensitivity.
	 *
	 * @return True if the replacement was performed.
	 */
	bool ReplaceSelectedText(const FString& SearchText, const FString& ReplacementText, ESearchCase::Type SearchCase);

	/**
	 * @brief Replaces all matching occurrences.
	 *
	 * @param SearchText Text to search for.
	 * @param ReplacementText Replacement text.
	 * @param SearchCase Search case sensitivity.
	 *
	 * @return Number of replacements performed.
	 */
	int32 ReplaceAllText(const FString& SearchText, const FString& ReplacementText, ESearchCase::Type SearchCase);

	/**
	 * @brief Toggles line comments for the current selection.
	 *
	 * @param CommentPrefix Comment prefix (for example "//").
	 *
	 * @return True if the operation modified the document.
	 */
	bool ToggleLineComment(const FString& CommentPrefix);

	/**
	 * @brief Duplicates the current line or selection.
	 *
	 * @return True if the operation succeeded.
	 */
	bool DuplicateLineOrSelection();

	/**
	 * @brief Moves the current line.
	 *
	 * @param Direction Negative to move up, positive to move down.
	 *
	 * @return True if the line was moved.
	 */
	bool MoveCurrentLine(int32 Direction);

	/**
	 * @brief Removes trailing whitespace.
	 *
	 * @return True if any changes were made.
	 */
	bool TrimTrailingWhitespace();

	/**
	 * @brief Converts all tabs to spaces.
	 *
	 * @return True if the document was modified.
	 */
	bool ConvertTabsToSpaces();

	/**
	 * @brief Converts indentation spaces to tabs.
	 *
	 * @return True if the document was modified.
	 */
	bool ConvertSpacesToTabs();

	/**
	 * @brief Ensures that the document ends with a newline.
	 *
	 * @return True if the document was modified.
	 */
	bool EnsureFinalNewline();

	/**
	 * @brief Normalizes line endings.
	 *
	 * @param LineEnding Target line ending style.
	 *
	 * @return True if the document was modified.
	 */
	bool NormalizeLineEndings(ENotepadLineEnding LineEnding);

private:
	/**
	 * @brief Handles keyboard input before it is processed by the editor.
	 *
	 * @param MyGeometry Geometry of the editor widget.
	 * @param InKeyEvent Keyboard event.
	 *
	 * @return Slate reply indicating whether the event was handled.
	 */
	FReply HandleEditorKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);

	/**
	 * @brief Forwards a mouse wheel event to the editor.
	 *
	 * @param MouseEvent Mouse wheel event.
	 *
	 * @return Slate reply indicating whether the event was handled.
	 */
	FReply ForwardMouseWheelToEditor(const FPointerEvent& MouseEvent);

	/**
	 * @brief Handles text changes in the editor.
	 *
	 * @param NewText Updated document text.
	 */
	void HandleTextChanged(const FText& NewText);

	/**
	 * @brief Handles horizontal scrolling.
	 *
	 * @param ScrollOffsetFraction Horizontal scroll position in the range [0, 1].
	 */
	void HandleEditorHorizontalScroll(float ScrollOffsetFraction);

	/**
	 * @brief Handles vertical scrolling.
	 *
	 * @param ScrollOffsetFraction Vertical scroll position in the range [0, 1].
	 */
	void HandleEditorVerticalScroll(float ScrollOffsetFraction);

	/**
	 * @brief Updates editor decorations after the document changes.
	 *
	 * Refreshes line numbers and syntax highlighting.
	 *
	 * @param Text Current document text.
	 */
	void UpdateDecorations(const FString& Text);

	/**
	 * @brief Synchronizes the scroll position of decoration widgets.
	 */
	void SyncDecorationScroll();

	/**
	 * @brief Returns the visibility state of the line number panel.
	 *
	 * @return Slate visibility for the line number widget.
	 */
	EVisibility GetLineNumberVisibility() const;

	/** Delegate invoked when the document text changes. */
	FOnTextChanged OnTextChanged;

	/** Delegate invoked for editor shortcut key handling. */
	FOnKeyDown OnShortcutKeyDown;

	/** Primary editable text widget. */
	TSharedPtr<SMultiLineEditableTextBox> EditorTextBox;

	/** Read-only widget displaying synchronized line numbers. */
	TSharedPtr<SNotepadSyncedText> LineNumberTextBox;

	/** Vertical scrollbar shared with the editor. */
	TSharedPtr<SScrollBar> VerticalScrollBar;

	/** Horizontal scrollbar shared with the editor. */
	TSharedPtr<SScrollBar> HorizontalScrollBar;

	/** Syntax highlighting marshaller. */
	TSharedPtr<FNotepadSyntaxHighlighterMarshaller> SyntaxMarshaller;

	/** Font used by the editor text widget. */
	FSlateFontInfo EditorFontInfo;

	/** Text style used to render line numbers. */
	FTextBlockStyle LineNumberTextStyle;

	/** Current document mode. */
	EUNotepadDocumentMode Mode = EUNotepadDocumentMode::Text;

	/** Number of spaces represented by a tab. */
	int32 TabSize = 4;

	/** Whether line numbers are currently displayed. */
	bool bShowLineNumbers = true;

	/** Whether whitespace characters are currently rendered. */
	bool bShowWhitespace = true;

	/** True while the document text is being updated programmatically. */
	bool bIsSettingText = false;

	/** Prevents recursive decoration scroll synchronization. */
	bool bIsSyncingDecorationScroll = false;

	/** Last horizontal scrollbar position. */
	float LastHorizontalScrollFraction = 0.0f;

	/** Last vertical scrollbar position. */
	float LastVerticalScrollFraction = 0.0f;
};
