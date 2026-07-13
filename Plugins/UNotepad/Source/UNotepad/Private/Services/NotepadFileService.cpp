// Fill out your copyright notice in the Description page of Project Settings.

#include "Services/NotepadFileService.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool FNotepadFileService::LoadFileToString(const FString& FilePath, FString& OutText, FString& OutError)
{
	const FString NormalizedPath = NormalizeFilePath(FilePath);
	if (!FFileHelper::LoadFileToString(OutText, *NormalizedPath))
	{
		OutError = FString::Printf(TEXT("Failed to read %s"), *NormalizedPath);
		return false;
	}

	return true;
}

bool FNotepadFileService::SaveStringToFile(const FString& FilePath, const FString& Text, FString& OutError)
{
	const FString NormalizedPath = NormalizeFilePath(FilePath);
	if (!FFileHelper::SaveStringToFile(Text, *NormalizedPath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		OutError = FString::Printf(TEXT("Failed to save %s"), *NormalizedPath);
		return false;
	}

	return true;
}

FString FNotepadFileService::NormalizeFilePath(const FString& FilePath)
{
	FString NormalizedPath = FPaths::ConvertRelativePathToFull(FilePath);
	FPaths::NormalizeFilename(NormalizedPath);
	return NormalizedPath;
}
