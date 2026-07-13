// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UNotepadSettings.generated.h"

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "UNotepad"))
class UNOTEPAD_API UUNotepadSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUNotepadSettings();

	static const UUNotepadSettings* Get();

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
#endif

	UPROPERTY(EditAnywhere, config, Category = "Editor", meta = (ClampMin = "6.0", ClampMax = "32.0", UIMin = "8.0", UIMax = "18.0"))
	float EditorFontSize = 10.0f;

	UPROPERTY(EditAnywhere, config, Category = "Editor", meta = (ClampMin = "1", ClampMax = "16", UIMin = "2", UIMax = "8"))
	int32 TabSize = 4;

	UPROPERTY(EditAnywhere, config, Category = "Editor")
	bool bShowLineNumbersByDefault = true;

	UPROPERTY(EditAnywhere, config, Category = "Editor")
	bool bShowWhitespaceByDefault = true;

	UPROPERTY(EditAnywhere, config, Category = "Solution Explorer")
	bool bShowSolutionExplorerByDefault = true;

	UPROPERTY(EditAnywhere, config, Category = "Solution Explorer")
	TArray<FString> SourceFileExtensions;

	float GetClampedEditorFontSize() const;
	int32 GetClampedTabSize() const;
	bool IsSourceFileExtension(const FString& Extension) const;
	TArray<FString> GetNormalizedSourceFileExtensions() const;

private:
	static FString NormalizeExtension(FString Extension);
};
