// Copyright Epic Games, Inc. All Rights Reserved.

#include "Services/NotepadTextActionService.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadToggleLineCommentCommentsAndUncommentsCodeLinesTest,
	"UNotepad.TextActions.ToggleLineComment.CommentsAndUncommentsCodeLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadToggleLineCommentCommentsAndUncommentsCodeLinesTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("int32 Value;\n\treturn Value;\n");

	TestTrue(TEXT("ToggleLineComment should comment non-blank lines"), FNotepadTextActionService::ToggleLineComment(Text, 0, 1, TEXT("//")));
	TestEqual(TEXT("Comment prefix should be inserted after indentation"), Text, FString(TEXT("// int32 Value;\n\t// return Value;\n")));

	TestTrue(TEXT("ToggleLineComment should uncomment already commented lines"), FNotepadTextActionService::ToggleLineComment(Text, 0, 1, TEXT("//")));
	TestEqual(TEXT("Original code should be restored"), Text, FString(TEXT("int32 Value;\n\treturn Value;\n")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadToggleLineCommentSelectionIgnoresTrailingEmptyLineTest,
	"UNotepad.TextActions.ToggleLineComment.SelectionIgnoresTrailingEmptyLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadToggleLineCommentSelectionIgnoresTrailingEmptyLineTest::RunTest(const FString& Parameters)
{
	FString SelectedText = TEXT("Alpha\n");

	TestTrue(TEXT("ToggleLineCommentForSelection should comment selected text"), FNotepadTextActionService::ToggleLineCommentForSelection(SelectedText, TEXT("//")));
	TestEqual(TEXT("Trailing selected newline should not create a commented phantom line"), SelectedText, FString(TEXT("// Alpha\n")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadDuplicateLineTest,
	"UNotepad.TextActions.DuplicateLine.InsertsCopyAfterLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadDuplicateLineTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("A\nB");

	TestTrue(TEXT("DuplicateLine should insert a copy after the requested line"), FNotepadTextActionService::DuplicateLine(Text, 0));
	TestEqual(TEXT("Line should be duplicated in place"), Text, FString(TEXT("A\nA\nB")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadMoveLineRespectsTrailingFinalNewlineTest,
	"UNotepad.TextActions.MoveLine.RespectsTrailingFinalNewline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadMoveLineRespectsTrailingFinalNewlineTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("A\nB\n");
	int32 NewLineIndex = INDEX_NONE;

	TestFalse(TEXT("Last editable line should not move into trailing empty EOF line"), FNotepadTextActionService::MoveLine(Text, 1, 1, NewLineIndex));
	TestEqual(TEXT("Text should remain unchanged when move is not possible"), Text, FString(TEXT("A\nB\n")));

	TestTrue(TEXT("Second line should move up"), FNotepadTextActionService::MoveLine(Text, 1, -1, NewLineIndex));
	TestEqual(TEXT("Lines should be swapped while preserving final newline"), Text, FString(TEXT("B\nA\n")));
	TestEqual(TEXT("New line index should point at moved line"), NewLineIndex, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadTrimTrailingWhitespaceTest,
	"UNotepad.TextActions.TrimTrailingWhitespace.RemovesSpacesAndTabs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadTrimTrailingWhitespaceTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("A \t\nB\t");

	TestTrue(TEXT("TrimTrailingWhitespace should remove trailing spaces and tabs"), FNotepadTextActionService::TrimTrailingWhitespace(Text));
	TestEqual(TEXT("Trailing whitespace should be removed on every line"), Text, FString(TEXT("A\nB")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadConvertTabsToSpacesTest,
	"UNotepad.TextActions.ConvertTabsToSpaces.PreservesTabColumns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadConvertTabsToSpacesTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("\tA\n  \tB");

	TestTrue(TEXT("ConvertTabsToSpaces should replace tabs"), FNotepadTextActionService::ConvertTabsToSpaces(Text, 4));
	TestEqual(TEXT("Tabs should expand to the next configured tab stop"), Text, FString(TEXT("    A\n    B")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadConvertLeadingSpacesToTabsTest,
	"UNotepad.TextActions.ConvertLeadingSpacesToTabs.ConvertsIndentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadConvertLeadingSpacesToTabsTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("    A\n      B\n  C");

	TestTrue(TEXT("ConvertLeadingSpacesToTabs should convert full leading tab stops"), FNotepadTextActionService::ConvertLeadingSpacesToTabs(Text, 4));
	TestEqual(TEXT("Only leading full tab stops should become tabs"), Text, FString(TEXT("\tA\n\t  B\n  C")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadEnsureFinalNewlinePreservesDetectedEndingTest,
	"UNotepad.TextActions.EnsureFinalNewline.PreservesDetectedEnding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadEnsureFinalNewlinePreservesDetectedEndingTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("A\r\nB");

	TestTrue(TEXT("EnsureFinalNewline should append a missing newline"), FNotepadTextActionService::EnsureFinalNewline(Text));
	TestEqual(TEXT("The detected CRLF style should be used"), Text, FString(TEXT("A\r\nB\r\n")));
	TestFalse(TEXT("EnsureFinalNewline should no-op when newline already exists"), FNotepadTextActionService::EnsureFinalNewline(Text));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadNormalizeLineEndingsTest,
	"UNotepad.TextActions.NormalizeLineEndings.ConvertsMixedEndings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadNormalizeLineEndingsTest::RunTest(const FString& Parameters)
{
	FString Text = TEXT("A\r\nB\rC\n");

	TestTrue(TEXT("NormalizeLineEndings should convert mixed endings to LF"), FNotepadTextActionService::NormalizeLineEndings(Text, ENotepadLineEnding::LF));
	TestEqual(TEXT("All line endings should be LF"), Text, FString(TEXT("A\nB\nC\n")));
	TestFalse(TEXT("NormalizeLineEndings should no-op when already LF"), FNotepadTextActionService::NormalizeLineEndings(Text, ENotepadLineEnding::LF));

	TestTrue(TEXT("NormalizeLineEndings should convert LF to CRLF"), FNotepadTextActionService::NormalizeLineEndings(Text, ENotepadLineEnding::CRLF));
	TestEqual(TEXT("All line endings should be CRLF"), Text, FString(TEXT("A\r\nB\r\nC\r\n")));

	return true;
}

#endif
