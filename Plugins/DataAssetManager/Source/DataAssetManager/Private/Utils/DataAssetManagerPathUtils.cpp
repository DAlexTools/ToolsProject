// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Utils/DataAssetManagerPathUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Types/DataAssetManagerConstants.h"

namespace DataAssetManager::PathConstants
{
	static const FName Developers(TEXT("/Game/Developers"));
	static const FName Root(TEXT("/Game"));
}

FString FDataAssetManagerPathUtils::GetDevelopersPath()
{
	return DataAssetManager::PathConstants::Developers.ToString();
}

FString FDataAssetManagerPathUtils::GetRootPath()
{
	return DataAssetManager::PathConstants::Root.ToString();
}

TTuple<FString, FString> FDataAssetManagerPathUtils::GetNormalizedAndProjectPath(const FString& InPath)
{
	return { Normalize(InPath), FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir().LeftChop(1)) };
}

FString FDataAssetManagerPathUtils::Normalize(const FString& InPath)
{
	if (InPath.IsEmpty())
	{
		return FString{};
	}

	if (!(InPath.StartsWith(TEXT("/")) || InPath.StartsWith(TEXT("\\")) || (InPath.Len() > 2 && InPath[1] == ':')))
	{
		return FString{};
	}

	FString Path{ FPaths::ConvertRelativePathToFull(InPath).TrimStartAndEnd() };
	FPaths::RemoveDuplicateSlashes(Path);
	FPaths::CollapseRelativeDirectories(Path);

	if (FPaths::GetExtension(Path).IsEmpty())
	{
		FPaths::NormalizeDirectoryName(Path);
	}
	else
	{
		FPaths::NormalizeFilename(Path);
	}

	if (Path.EndsWith(TEXT("/")) || Path.EndsWith(TEXT("\\")))
	{
		Path = Path.LeftChop(1);
	}

	return Path;
}

FString FDataAssetManagerPathUtils::Convert(const FString& InPath, bool bToAbsolute)
{
	const TTuple<FString, FString> PathsData = GetNormalizedAndProjectPath(InPath);
	const FString& PathNormalized = PathsData.Get<0>();
	const FString& PathProjectContent = PathsData.Get<1>();

	if (PathNormalized.IsEmpty())
	{
		return FString{};
	}

	const FString LocalPathRoot = GetRootPath();
	const bool bIsRoot = PathNormalized.StartsWith(LocalPathRoot);
	const bool bIsProject = PathNormalized.StartsWith(PathProjectContent);

	if (bToAbsolute)
	{
		if (bIsProject)
		{
			return PathNormalized;
		}

		if (bIsRoot)
		{
			FString Path = PathNormalized;
			Path.RemoveFromStart(LocalPathRoot);
			return Path.IsEmpty() ? PathProjectContent : PathProjectContent / Path;
		}
	}
	else
	{
		if (bIsRoot)
		{
			return PathNormalized;
		}

		if (bIsProject)
		{
			FString Path = PathNormalized;
			Path.RemoveFromStart(PathProjectContent);
			return Path.IsEmpty() ? LocalPathRoot : LocalPathRoot / Path;
		}
	}

	return FString{};
}

FString FDataAssetManagerPathUtils::ConvertToAbsolute(const FString& InPath)
{
	return Convert(InPath, true);
}

FString FDataAssetManagerPathUtils::ConvertToRelative(const FString& InPath)
{
	return Convert(InPath, false);
}

FString FDataAssetManagerPathUtils::GetExternalActorsPath()
{
	return FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalActorsFolderName());
}

FString FDataAssetManagerPathUtils::GetExternalObjectsPath()
{
	return FString::Printf(TEXT("/Game/%s"), FPackagePath::GetExternalObjectsFolderName());
}

bool FDataAssetManagerPathUtils::IsExternalFolder(const FString& InPath)
{
	return InPath.StartsWith(GetExternalActorsPath()) || InPath.StartsWith(GetExternalObjectsPath());
}

bool FDataAssetManagerPathUtils::IsFolderEmpty(const FString& InPath)
{
	if (InPath.IsEmpty())
	{
		return false;
	}

	const FName RelativePath = FName(*ConvertToRelative(InPath));
	const FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(DataAssetManager::ModuleName::AssetRegistry);
	if (AssetRegistry.Get().HasAssets(RelativePath, true))
	{
		return false;
	}

	const FString AbsolutePath = ConvertToAbsolute(InPath);
	if (AbsolutePath.IsEmpty())
	{
		return false;
	}

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *AbsolutePath, TEXT("*"), true, false);

	return Files.Num() == 0;
}
