// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UNotepadSettings.generated.h"

/**
 * Editor settings for the UNotepad plugin.
 *
 * Stores configurable editor preferences such as font size, tab width,
 * default visibility options, and supported source file extensions.
 *
 * These settings are available in:
 * Project Settings -> Plugins -> UNotepad
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "UNotepad"))
class UNOTEPAD_API UUNotepadSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Creates the default settings object. */
	UUNotepadSettings();

	/**
	 * Returns the global UNotepad settings object.
	 *
	 * @return Read-only settings instance.
	 */
	static const UUNotepadSettings* Get();

#if WITH_EDITOR
	/**
	 * Returns the display name of the settings section in Project Settings.
	 *
	 * @return Localized section title.
	 */
	virtual FText GetSectionText() const override;
#endif

	/**
	 * Font size used by the text editor.
	 *
	 * Value is clamped to a supported range before use.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Editor", meta = (ClampMin = "6.0", ClampMax = "32.0", UIMin = "8.0", UIMax = "18.0"))
	float EditorFontSize = 10.0f;

	/**
	 * Number of spaces represented by a tab.
	 *
	 * Value is clamped to a supported range before use.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Editor", meta = (ClampMin = "1", ClampMax = "16", UIMin = "2", UIMax = "8"))
	int32 TabSize = 4;

	/** Whether line numbers are visible by default when opening a document. */
	UPROPERTY(EditAnywhere, config, Category = "Editor")
	bool bShowLineNumbersByDefault = true;

	/** Whether whitespace characters are rendered by default. */
	UPROPERTY(EditAnywhere, config, Category = "Editor")
	bool bShowWhitespaceByDefault = true;

	/** Whether the Solution Explorer panel is visible by default. */
	UPROPERTY(EditAnywhere, config, Category = "Solution Explorer")
	bool bShowSolutionExplorerByDefault = true;

	/**
	 * List of source file extensions recognized by UNotepad.
	 *
	 * Extensions may be specified with or without a leading dot and are
	 * normalized before comparisons.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Solution Explorer")
	TArray<FString> SourceFileExtensions;

	/**
	 * Returns the configured editor font size clamped to the supported range.
	 *
	 * @return Valid font size.
	 */
	float GetClampedEditorFontSize() const;

	/**
	 * Returns the configured tab size clamped to the supported range.
	 *
	 * @return Valid tab width.
	 */
	int32 GetClampedTabSize() const;

	/**
	 * Determines whether the specified file extension is recognized as a source file.
	 *
	 * @param Extension File extension to test (with or without a leading dot).
	 * @return True if the extension is contained in the configured source extension list.
	 */
	bool IsSourceFileExtension(const FString& Extension) const;

	/**
	 * Returns the configured source file extensions in normalized form.
	 *
	 * @return Array of normalized extensions.
	 */
	TArray<FString> GetNormalizedSourceFileExtensions() const;

private:
	/**
	 * Converts a file extension into its normalized form.
	 *
	 * Normalization removes redundant formatting, ensures consistent casing,
	 * and guarantees a leading dot.
	 *
	 * @param Extension Extension to normalize.
	 * @return Normalized extension string.
	 */
	static FString NormalizeExtension(FString Extension);
};