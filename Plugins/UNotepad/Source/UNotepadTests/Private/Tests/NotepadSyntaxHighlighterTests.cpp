// Copyright Epic Games, Inc. All Rights Reserved.

#include "Syntax/NotepadSyntaxHighlighterMarshaller.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/Text/IRun.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Misc/AutomationTest.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	TArray<FString> GetSyntaxRunNames(EUNotepadDocumentMode Mode, const FString& Text, bool bShowWhitespace = true)
	{
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 10.0f);
		const TSharedRef<FNotepadSyntaxHighlighterMarshaller> Marshaller = FNotepadSyntaxHighlighterMarshaller::Create(Mode, Font);
		Marshaller->SetShowWhitespace(bShowWhitespace);
		Marshaller->SetTabSize(4);

		const TSharedRef<STextBlock> Owner = SNew(STextBlock);
		const FTextBlockStyle DefaultTextStyle = FTextBlockStyle().SetFont(Font);
		const TSharedRef<FSlateTextLayout> Layout = FSlateTextLayout::Create(&Owner.Get(), DefaultTextStyle);
		Marshaller->SetText(Text, *Layout);

		TArray<FString> RunNames;
		for (const FTextLayout::FLineModel& Line : Layout->GetLineModels())
		{
			for (const FTextLayout::FRunModel& Run : Line.Runs)
			{
				RunNames.Add(Run.GetRun()->GetRunInfo().Name);
			}
		}
		return RunNames;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadSyntaxHighlighterCodeTest,
	"UNotepad.SyntaxHighlighter.Code.CreatesExpectedRunTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadSyntaxHighlighterCodeTest::RunTest(const FString& Parameters)
{
	const TArray<FString> RunNames = GetSyntaxRunNames(
		EUNotepadDocumentMode::Code,
		TEXT("#include \"A.h\"\nint32 Foo(int32 X) { return X + 1; }\n// comment\nDECLARE_DELEGATE(FThingDelegate);"));

	TestTrue(TEXT("Code highlighter should mark preprocessors"), RunNames.Contains(TEXT("UNotepad.Preprocessor")));
	TestTrue(TEXT("Code highlighter should mark keywords"), RunNames.Contains(TEXT("UNotepad.Keyword")));
	TestTrue(TEXT("Code highlighter should mark functions"), RunNames.Contains(TEXT("UNotepad.Function")));
	TestTrue(TEXT("Code highlighter should mark numbers"), RunNames.Contains(TEXT("UNotepad.Number")));
	TestTrue(TEXT("Code highlighter should mark comments"), RunNames.Contains(TEXT("UNotepad.Comment")));
	TestTrue(TEXT("Code highlighter should mark delegates"), RunNames.Contains(TEXT("UNotepad.Delegate")));
	TestTrue(TEXT("Code highlighter should mark punctuation"), RunNames.Contains(TEXT("UNotepad.Punctuation")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadSyntaxHighlighterJsonCsvTextTest,
	"UNotepad.SyntaxHighlighter.Modes.CreateExpectedRunTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadSyntaxHighlighterJsonCsvTextTest::RunTest(const FString& Parameters)
{
	const TArray<FString> JsonRunNames = GetSyntaxRunNames(EUNotepadDocumentMode::Json, TEXT("{\"ok\": true, \"n\": -12.5}"));
	TestTrue(TEXT("JSON highlighter should mark strings"), JsonRunNames.Contains(TEXT("UNotepad.String")));
	TestTrue(TEXT("JSON highlighter should mark literals as keywords"), JsonRunNames.Contains(TEXT("UNotepad.Keyword")));
	TestTrue(TEXT("JSON highlighter should mark numbers"), JsonRunNames.Contains(TEXT("UNotepad.Number")));
	TestTrue(TEXT("JSON highlighter should mark punctuation"), JsonRunNames.Contains(TEXT("UNotepad.Punctuation")));

	const TArray<FString> CsvRunNames = GetSyntaxRunNames(EUNotepadDocumentMode::Csv, TEXT("Name,\"A,B\""));
	TestTrue(TEXT("CSV highlighter should mark normal cells"), CsvRunNames.Contains(TEXT("UNotepad.Normal")));
	TestTrue(TEXT("CSV highlighter should mark quoted cells"), CsvRunNames.Contains(TEXT("UNotepad.String")));
	TestTrue(TEXT("CSV highlighter should mark separators"), CsvRunNames.Contains(TEXT("UNotepad.Punctuation")));

	const TArray<FString> TextRunNames = GetSyntaxRunNames(EUNotepadDocumentMode::Text, TEXT("A \tB"));
	TestTrue(TEXT("Text highlighter should mark visible spaces"), TextRunNames.Contains(TEXT("UNotepad.Space")));
	TestTrue(TEXT("Text highlighter should mark tabs"), TextRunNames.Contains(TEXT("UNotepad.Tab")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUNotepadSyntaxHighlighterLiveUpdateTest,
	"UNotepad.SyntaxHighlighter.RequiresLiveUpdate.ReturnsTrue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUNotepadSyntaxHighlighterLiveUpdateTest::RunTest(const FString& Parameters)
{
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 10.0f);
	const TSharedRef<FNotepadSyntaxHighlighterMarshaller> Marshaller = FNotepadSyntaxHighlighterMarshaller::Create(EUNotepadDocumentMode::Text, Font);

	TestTrue(TEXT("UNotepad syntax marshaller should request live updates"), Marshaller->RequiresLiveUpdate());

	return true;
}

#endif
