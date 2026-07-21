// Copyright 2025 DimAlek. All Rights Reserved.

#include "Services/TextureChannelPackerPathService.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"

namespace TexturePathUtils
{
	static const FName Root(TEXT("/Game"));

	/**
	 * @brief Returns the project content root path.
	 * 
	 * @return Long package path for the project content root.
	 */
	FString GetRootPath()
	{
		return Root.ToString();
	}

	/**
	 * @brief Normalizes an absolute filesystem path or long package path.
	 * 
	 * @param InPath Path to normalize.
	 * @return Normalized path, or an empty string when the input is not supported.
	 */
	FString Normalize(const FString& InPath)
	{
		if (InPath.IsEmpty())
		{
			return FString{};
		}

		if (!(InPath.StartsWith(TEXT("/")) || InPath.StartsWith(TEXT("\\")) || (InPath.Len() > 2 && InPath[1] == ':')))
		{
			return FString{};
		}

		FString Path = FPaths::ConvertRelativePathToFull(InPath).TrimStartAndEnd();
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

	/**
	 * @brief Normalizes an input path and resolves the absolute project content path.
	 * 
	 * @param InPath Path to normalize.
	 * @return Tuple containing the normalized input path and absolute project content path.
	 */
	TTuple<FString, FString> GetNormalizedAndProjectPath(const FString& InPath)
	{
		return { Normalize(InPath), FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir().LeftChop(1)) };
	}

	/**
	 * @brief Converts a path between project-relative long package form and absolute filesystem form.
	 * 
	 * @param InPath Path to convert.
	 * @param bToAbsolute true to convert to absolute filesystem path, false to convert to long package path.
	 * @return Converted path, or an empty string when conversion is not possible.
	 */
	FString Convert(const FString& InPath, bool bToAbsolute)
	{
		const TTuple<FString, FString> PathsData = TexturePathUtils::GetNormalizedAndProjectPath(InPath);
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

	/**
	 * @brief Converts an absolute project content path to a long package path.
	 * 
	 * @param InPath Path to convert.
	 * @return Long package path, or an empty string when conversion fails.
	 */
	FString ConvertToRelative(const FString& InPath)
	{
		return Convert(InPath, false);
	}
}

FString FTextureChannelPackerPathService::NormalizeOutputPath(const FString& InPath)
{
	FString NormalizedPath = TexturePathUtils::ConvertToRelative(InPath);
	if (NormalizedPath.IsEmpty())
	{
		NormalizedPath = TexturePathUtils::Normalize(InPath);
	}

	if (NormalizedPath.IsEmpty() || !NormalizedPath.StartsWith(TEXT("/Game")))
	{
		NormalizedPath = TexturePathUtils::GetRootPath();
	}

	if (NormalizedPath.EndsWith(TEXT("/")))
	{
		NormalizedPath.LeftChopInline(1);
	}

	return NormalizedPath;
}

FString FTextureChannelPackerPathService::SanitizeAssetName(const FString& InAssetName)
{
	const FString TrimmedName = InAssetName.TrimStartAndEnd();
	const FString SanitizedName = ObjectTools::SanitizeObjectName(TrimmedName.IsEmpty() ? TEXT("T_Packed_Texture") : TrimmedName);

	return SanitizedName.IsEmpty() ? TEXT("T_Packed_Texture") : SanitizedName;
}

FString FTextureChannelPackerPathService::BuildPackageName(const FString& OutputPackagePath, const FString& AssetName)
{
	return NormalizeOutputPath(OutputPackagePath) / SanitizeAssetName(AssetName);
}

bool FTextureChannelPackerPathService::MakeUniqueAssetPackageName(const FString& OutputPackagePath, const FString& AssetName, FString& OutPackageName, FString& OutAssetName)
{
	const FString BasePackageName = BuildPackageName(OutputPackagePath, AssetName);
	if (!FPackageName::IsValidLongPackageName(BasePackageName))
	{
		return false;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, TEXT(""), OutPackageName, OutAssetName);

	return !OutPackageName.IsEmpty() && !OutAssetName.IsEmpty();
}
