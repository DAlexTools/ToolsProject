// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/SNotepadEditor.h"

#include "Framework/Text/TextLayout.h"
#include "Settings/UNotepadSettings.h"
#include "Services/NotepadDocumentUtils.h"
#include "Services/NotepadTextActionService.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Syntax/NotepadSyntaxHighlighterMarshaller.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SMultiLineEditableText.h"

class SNotepadSyncedText : public SMultiLineEditableText
{
public:
	void SetMouseWheelForwarder(TFunction<FReply(const FPointerEvent&)> InMouseWheelForwarder)
	{
		MouseWheelForwarder = MoveTemp(InMouseWheelForwarder);
	}

	void SyncHorizontalScroll(float ScrollOffsetFraction)
	{
		OnHScrollBarMoved(ScrollOffsetFraction);
	}

	void SyncVerticalScroll(float ScrollOffsetFraction)
	{
		OnVScrollBarMoved(ScrollOffsetFraction);
	}

	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseWheelForwarder)
		{
			return MouseWheelForwarder(MouseEvent);
		}

		return SMultiLineEditableText::OnMouseWheel(MyGeometry, MouseEvent);
	}

private:
	TFunction<FReply(const FPointerEvent&)> MouseWheelForwarder;
};

namespace UNotepad::Editor
{
	const FLinearColor EditorBackground(0.020f, 0.023f, 0.028f, 1.0f);
	const FLinearColor GutterBackground(0.030f, 0.034f, 0.040f, 1.0f);
	const FLinearColor LineNumberColor(0.420f, 0.470f, 0.540f, 1.0f);
	const FLinearColor TextColor(0.840f, 0.870f, 0.910f, 1.0f);
	const FMargin TextMargin(4.0f);
}

void SNotepadEditor::Construct(const FArguments& InArgs)
{
	OnTextChanged = InArgs._OnTextChanged;
	OnShortcutKeyDown = InArgs._OnShortcutKeyDown;

	const UUNotepadSettings* Settings = UUNotepadSettings::Get();
	EditorFontInfo = FCoreStyle::GetDefaultFontStyle("Mono", Settings->GetClampedEditorFontSize());
	TabSize = Settings->GetClampedTabSize();
	LineNumberTextStyle = FTextBlockStyle()
		.SetFont(EditorFontInfo)
		.SetColorAndOpacity(FSlateColor(UNotepad::Editor::LineNumberColor));
	SyntaxMarshaller = FNotepadSyntaxHighlighterMarshaller::Create(Mode, EditorFontInfo);
	SyntaxMarshaller->SetTabSize(TabSize);

	SAssignNew(VerticalScrollBar, SScrollBar)
		.Orientation(Orient_Vertical)
		.Visibility(EVisibility::Collapsed);

	SAssignNew(HorizontalScrollBar, SScrollBar)
		.Orientation(Orient_Horizontal)
		.Visibility(EVisibility::Collapsed);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(UNotepad::Editor::EditorBackground)
		.Padding(0.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(56.0f)
					.Visibility(this, &SNotepadEditor::GetLineNumberVisibility)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(UNotepad::Editor::GutterBackground)
						.Padding(FMargin(4.0f, 4.0f, 4.0f, 4.0f))
						[
							SAssignNew(LineNumberTextBox, SNotepadSyncedText)
							.Text(FText::FromString(TEXT("1")))
							.TextStyle(&LineNumberTextStyle)
							.Font(EditorFontInfo)
							.IsReadOnly(true)
							.AllowContextMenu(false)
							.AutoWrapText(false)
							.LineHeightPercentage(1.0f)
							.Justification(ETextJustify::Right)
							.Margin(FMargin(0.0f))
							.VScrollBar(VerticalScrollBar)
							.HScrollBar(SNew(SScrollBar).Orientation(Orient_Horizontal).Visibility(EVisibility::Collapsed))
						
						]
					]
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(EditorTextBox, SMultiLineEditableTextBox)
					.Text(FText::GetEmpty())
					.HintText(NSLOCTEXT("UNotepadEditor", "EditorHintText", "Start typing..."))
					.Font(EditorFontInfo)
					.Marshaller(SyntaxMarshaller)
					.ForegroundColor(UNotepad::Editor::TextColor)
					.BackgroundColor(FLinearColor::Transparent)
					.Padding(FMargin(0.0f))
					.Margin(UNotepad::Editor::TextMargin)
					.AllowContextMenu(true)
					.AlwaysShowScrollbars(false)
					.AutoWrapText(false)
					.LineHeightPercentage(1.0f)
					.HScrollBar(HorizontalScrollBar)
					.VScrollBar(VerticalScrollBar)
					.OnHScrollBarUserScrolled(this, &SNotepadEditor::HandleEditorHorizontalScroll)
					.OnVScrollBarUserScrolled(this, &SNotepadEditor::HandleEditorVerticalScroll)
					.OnKeyDownHandler(this, &SNotepadEditor::HandleEditorKeyDown)
					.OnTextChanged(this, &SNotepadEditor::HandleTextChanged)
				]
			]
		]
	];

	if (LineNumberTextBox.IsValid())
	{
		LineNumberTextBox->SetMouseWheelForwarder([this](const FPointerEvent& MouseEvent)
		{
			return ForwardMouseWheelToEditor(MouseEvent);
		});
	}

	SyntaxMarshaller->SetShowWhitespace(bShowWhitespace);
}

void SNotepadEditor::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (VerticalScrollBar.IsValid())
	{
		const float CurrentVerticalScrollFraction = VerticalScrollBar->DistanceFromTop();
		if (!FMath::IsNearlyEqual(CurrentVerticalScrollFraction, LastVerticalScrollFraction))
		{
			LastVerticalScrollFraction = CurrentVerticalScrollFraction;
			SyncDecorationScroll();
		}
	}
}

void SNotepadEditor::SetText(const FString& NewText)
{
	bIsSettingText = true;
	if (EditorTextBox.IsValid())
	{
		EditorTextBox->SetText(FText::FromString(NewText));
		EditorTextBox->Refresh();
	}
	bIsSettingText = false;

	UpdateDecorations(NewText);
}

FString SNotepadEditor::GetText() const
{
	return EditorTextBox.IsValid() ? EditorTextBox->GetText().ToString() : FString();
}

void SNotepadEditor::SetMode(EUNotepadDocumentMode NewMode)
{
	Mode = NewMode;
	if (SyntaxMarshaller.IsValid())
	{
		SyntaxMarshaller->SetMode(Mode);
	}
	if (EditorTextBox.IsValid())
	{
		EditorTextBox->Refresh();
	}
}

void SNotepadEditor::FocusEditor()
{
	if (EditorTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(EditorTextBox, EFocusCause::SetDirectly);
	}
}

void SNotepadEditor::SetShowLineNumbers(bool bShow)
{
	bShowLineNumbers = bShow;
	UpdateDecorations(GetText());
}

void SNotepadEditor::SetShowWhitespace(bool bShow)
{
	bShowWhitespace = bShow;
	if (SyntaxMarshaller.IsValid())
	{
		SyntaxMarshaller->SetShowWhitespace(bShowWhitespace);
	}
	if (EditorTextBox.IsValid())
	{
		EditorTextBox->Refresh();
	}
}

void SNotepadEditor::SetTabSize(int32 InTabSize)
{
	TabSize = FMath::Clamp(InTabSize, 1, 16);
	if (SyntaxMarshaller.IsValid())
	{
		SyntaxMarshaller->SetTabSize(TabSize);
	}
	if (EditorTextBox.IsValid())
	{
		EditorTextBox->Refresh();
	}
}

bool SNotepadEditor::IsShowingLineNumbers() const
{
	return bShowLineNumbers;
}

bool SNotepadEditor::IsShowingWhitespace() const
{
	return bShowWhitespace;
}

int32 SNotepadEditor::GetLineCount() const
{
	return FNotepadDocumentUtils::CountLines(GetText());
}

bool SNotepadEditor::GoToLine(int32 LineNumber)
{
	if (!EditorTextBox.IsValid() || LineNumber < 1)
	{
		return false;
	}

	const int32 LineCount = GetLineCount();
	if (LineNumber > LineCount)
	{
		return false;
	}

	const FTextLocation LineLocation(LineNumber - 1, 0);
	EditorTextBox->ClearSelection();
	EditorTextBox->GoTo(LineLocation);
	EditorTextBox->ScrollTo(LineLocation);
	FocusEditor();
	return true;
}

void SNotepadEditor::BeginSearch(const FString& SearchText, ESearchCase::Type SearchCase, bool bReverse)
{
	if (EditorTextBox.IsValid())
	{
		EditorTextBox->BeginSearch(FText::FromString(SearchText), SearchCase, bReverse);
	}
}

void SNotepadEditor::AdvanceSearch(bool bReverse)
{
	if (EditorTextBox.IsValid())
	{
		EditorTextBox->AdvanceSearch(bReverse);
	}
}

void SNotepadEditor::ClearSearch()
{
	if (EditorTextBox.IsValid())
	{
		EditorTextBox->BeginSearch(FText::GetEmpty(), ESearchCase::IgnoreCase, false);
	}
}

FString SNotepadEditor::GetSelectedText() const
{
	return EditorTextBox.IsValid() ? EditorTextBox->GetSelectedText().ToString() : FString();
}

bool SNotepadEditor::IsSelectedTextMatching(const FString& SearchText, ESearchCase::Type SearchCase) const
{
	return !SearchText.IsEmpty()
		&& GetSelectedText().Len() == SearchText.Len()
		&& GetSelectedText().Equals(SearchText, SearchCase);
}

bool SNotepadEditor::ReplaceSelectedText(const FString& SearchText, const FString& ReplacementText, ESearchCase::Type SearchCase)
{
	if (!EditorTextBox.IsValid() || !IsSelectedTextMatching(SearchText, SearchCase))
	{
		return false;
	}

	EditorTextBox->InsertTextAtCursor(ReplacementText);
	return true;
}

int32 SNotepadEditor::ReplaceAllText(const FString& SearchText, const FString& ReplacementText, ESearchCase::Type SearchCase)
{
	if (!EditorTextBox.IsValid() || SearchText.IsEmpty())
	{
		return 0;
	}

	const FString OriginalText = GetText();
	FString ReplacedText;
	int32 SearchStartIndex = 0;
	int32 ReplacementCount = 0;

	while (SearchStartIndex < OriginalText.Len())
	{
		const int32 MatchIndex = OriginalText.Find(SearchText, SearchCase, ESearchDir::FromStart, SearchStartIndex);
		if (MatchIndex == INDEX_NONE)
		{
			break;
		}

		ReplacedText += OriginalText.Mid(SearchStartIndex, MatchIndex - SearchStartIndex);
		ReplacedText += ReplacementText;
		SearchStartIndex = MatchIndex + SearchText.Len();
		++ReplacementCount;
	}

	if (ReplacementCount == 0)
	{
		return 0;
	}

	ReplacedText += OriginalText.Mid(SearchStartIndex);
	SetText(ReplacedText);
	return ReplacementCount;
}

bool SNotepadEditor::ToggleLineComment(const FString& CommentPrefix)
{
	if (!EditorTextBox.IsValid())
	{
		return false;
	}

	if (EditorTextBox->AnyTextSelected())
	{
		FString SelectedText = EditorTextBox->GetSelectedText().ToString();
		if (!FNotepadTextActionService::ToggleLineCommentForSelection(SelectedText, CommentPrefix))
		{
			return false;
		}

		EditorTextBox->InsertTextAtCursor(SelectedText);
		return true;
	}

	FString Text = GetText();
	const int32 CursorLineIndex = EditorTextBox->GetCursorLocation().GetLineIndex();
	if (!FNotepadTextActionService::ToggleLineComment(Text, CursorLineIndex, CursorLineIndex, CommentPrefix))
	{
		return false;
	}

	SetText(Text);
	GoToLine(CursorLineIndex + 1);
	return true;
}

bool SNotepadEditor::DuplicateLineOrSelection()
{
	if (!EditorTextBox.IsValid())
	{
		return false;
	}

	if (EditorTextBox->AnyTextSelected())
	{
		const FString SelectedText = EditorTextBox->GetSelectedText().ToString();
		if (SelectedText.IsEmpty())
		{
			return false;
		}

		EditorTextBox->InsertTextAtCursor(SelectedText + SelectedText);
		return true;
	}

	FString Text = GetText();
	const int32 CursorLineIndex = EditorTextBox->GetCursorLocation().GetLineIndex();
	if (!FNotepadTextActionService::DuplicateLine(Text, CursorLineIndex))
	{
		return false;
	}

	SetText(Text);
	GoToLine(CursorLineIndex + 2);
	return true;
}

bool SNotepadEditor::MoveCurrentLine(int32 Direction)
{
	if (!EditorTextBox.IsValid())
	{
		return false;
	}

	FString Text = GetText();
	const int32 CursorLineIndex = EditorTextBox->GetCursorLocation().GetLineIndex();
	int32 NewLineIndex = CursorLineIndex;
	if (!FNotepadTextActionService::MoveLine(Text, CursorLineIndex, Direction, NewLineIndex))
	{
		return false;
	}

	SetText(Text);
	GoToLine(NewLineIndex + 1);
	return true;
}

bool SNotepadEditor::TrimTrailingWhitespace()
{
	if (!EditorTextBox.IsValid())
	{
		return false;
	}

	FString Text = GetText();
	const int32 CursorLineIndex = EditorTextBox->GetCursorLocation().GetLineIndex();
	if (!FNotepadTextActionService::TrimTrailingWhitespace(Text))
	{
		return false;
	}

	SetText(Text);
	GoToLine(FMath::Min(CursorLineIndex + 1, GetLineCount()));
	return true;
}

bool SNotepadEditor::ConvertTabsToSpaces()
{
	if (!EditorTextBox.IsValid())
	{
		return false;
	}

	FString Text = GetText();
	const int32 CursorLineIndex = EditorTextBox->GetCursorLocation().GetLineIndex();
	if (!FNotepadTextActionService::ConvertTabsToSpaces(Text, TabSize))
	{
		return false;
	}

	SetText(Text);
	GoToLine(FMath::Min(CursorLineIndex + 1, GetLineCount()));
	return true;
}

bool SNotepadEditor::ConvertSpacesToTabs()
{
	if (!EditorTextBox.IsValid())
	{
		return false;
	}

	FString Text = GetText();
	const int32 CursorLineIndex = EditorTextBox->GetCursorLocation().GetLineIndex();
	if (!FNotepadTextActionService::ConvertLeadingSpacesToTabs(Text, TabSize))
	{
		return false;
	}

	SetText(Text);
	GoToLine(FMath::Min(CursorLineIndex + 1, GetLineCount()));
	return true;
}

bool SNotepadEditor::EnsureFinalNewline()
{
	if (!EditorTextBox.IsValid())
	{
		return false;
	}

	FString Text = GetText();
	const int32 CursorLineIndex = EditorTextBox->GetCursorLocation().GetLineIndex();
	if (!FNotepadTextActionService::EnsureFinalNewline(Text))
	{
		return false;
	}

	SetText(Text);
	GoToLine(FMath::Min(CursorLineIndex + 1, GetLineCount()));
	return true;
}

bool SNotepadEditor::NormalizeLineEndings(ENotepadLineEnding LineEnding)
{
	if (!EditorTextBox.IsValid())
	{
		return false;
	}

	FString Text = GetText();
	const int32 CursorLineIndex = EditorTextBox->GetCursorLocation().GetLineIndex();
	if (!FNotepadTextActionService::NormalizeLineEndings(Text, LineEnding))
	{
		return false;
	}

	SetText(Text);
	GoToLine(FMath::Min(CursorLineIndex + 1, GetLineCount()));
	return true;
}

FReply SNotepadEditor::HandleEditorKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (OnShortcutKeyDown.IsBound())
	{
		const FReply ShortcutReply = OnShortcutKeyDown.Execute(MyGeometry, InKeyEvent);
		if (ShortcutReply.IsEventHandled())
		{
			return ShortcutReply;
		}
	}

	if (InKeyEvent.GetKey() == EKeys::Tab && !InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown())
	{
		if (EditorTextBox.IsValid())
		{
			EditorTextBox->InsertTextAtCursor(TEXT("\t"));
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SNotepadEditor::ForwardMouseWheelToEditor(const FPointerEvent& MouseEvent)
{
	if (!EditorTextBox.IsValid())
	{
		return FReply::Unhandled();
	}

	const FReply Reply = EditorTextBox->OnMouseWheel(EditorTextBox->GetTickSpaceGeometry(), MouseEvent);
	SyncDecorationScroll();
	return Reply;
}

void SNotepadEditor::HandleTextChanged(const FText& NewText)
{
	const FString Text = NewText.ToString();
	UpdateDecorations(Text);

	if (!bIsSettingText)
	{
		OnTextChanged.ExecuteIfBound(NewText);
	}
}

void SNotepadEditor::HandleEditorHorizontalScroll(float ScrollOffsetFraction)
{
	LastHorizontalScrollFraction = ScrollOffsetFraction;
	SyncDecorationScroll();
}

void SNotepadEditor::HandleEditorVerticalScroll(float ScrollOffsetFraction)
{
	LastVerticalScrollFraction = ScrollOffsetFraction;
	SyncDecorationScroll();
}

void SNotepadEditor::UpdateDecorations(const FString& Text)
{
	if (LineNumberTextBox.IsValid())
	{
		LineNumberTextBox->SetText(FText::FromString(FNotepadDocumentUtils::BuildLineNumberText(FNotepadDocumentUtils::CountLines(Text))));
		LineNumberTextBox->Refresh();
	}

	SyncDecorationScroll();
}

void SNotepadEditor::SyncDecorationScroll()
{
	if (bIsSyncingDecorationScroll)
	{
		return;
	}

	TGuardValue<bool> SyncGuard(bIsSyncingDecorationScroll, true);

	if (LineNumberTextBox.IsValid())
	{
		LineNumberTextBox->SyncVerticalScroll(LastVerticalScrollFraction);
	}

}

EVisibility SNotepadEditor::GetLineNumberVisibility() const
{
	return bShowLineNumbers ? EVisibility::Visible : EVisibility::Collapsed;
}
