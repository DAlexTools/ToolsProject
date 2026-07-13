// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UNOTEPAD_API FNotepadFileService
{
public:
	static bool LoadFileToString(const FString& FilePath, FString& OutText, FString& OutError);
	static bool SaveStringToFile(const FString& FilePath, const FString& Text, FString& OutError);
	static FString NormalizeFilePath(const FString& FilePath);
};
