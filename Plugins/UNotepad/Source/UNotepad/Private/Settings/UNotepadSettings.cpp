// Fill out your copyright notice in the Description page of Project Settings.

#include "Settings/UNotepadSettings.h"

#define LOCTEXT_NAMESPACE "UNotepadSettings"

UUNotepadSettings::UUNotepadSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("UNotepad");

	SourceFileExtensions = {
		TEXT("h"),
		TEXT("hh"),
		TEXT("hpp"),
		TEXT("hxx"),
		TEXT("inl"),
		TEXT("ipp"),
		TEXT("cpp"),
		TEXT("cc"),
		TEXT("cxx"),
		TEXT("cs")
	};
}

const UUNotepadSettings* UUNotepadSettings::Get()
{
	return GetDefault<UUNotepadSettings>();
}

#if WITH_EDITOR
FText UUNotepadSettings::GetSectionText() const
{
	return LOCTEXT("SettingsDisplayName", "UNotepad");
}
#endif

float UUNotepadSettings::GetClampedEditorFontSize() const
{
	return FMath::Clamp(EditorFontSize, 6.0f, 32.0f);
}

int32 UUNotepadSettings::GetClampedTabSize() const
{
	return FMath::Clamp(TabSize, 1, 16);
}

bool UUNotepadSettings::IsSourceFileExtension(const FString& Extension) const
{
	const FString NormalizedExtension = NormalizeExtension(Extension);
	for (const FString& SourceExtension : SourceFileExtensions)
	{
		if (NormalizeExtension(SourceExtension).Equals(NormalizedExtension, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

TArray<FString> UUNotepadSettings::GetNormalizedSourceFileExtensions() const
{
	TArray<FString> NormalizedExtensions;
	for (const FString& SourceExtension : SourceFileExtensions)
	{
		const FString NormalizedExtension = NormalizeExtension(SourceExtension);
		if (!NormalizedExtension.IsEmpty())
		{
			NormalizedExtensions.AddUnique(NormalizedExtension);
		}
	}

	return NormalizedExtensions;
}

FString UUNotepadSettings::NormalizeExtension(FString Extension)
{
	Extension.TrimStartAndEndInline();
	Extension.RemoveFromStart(TEXT("."));
	return Extension.ToLower();
}

#undef LOCTEXT_NAMESPACE
