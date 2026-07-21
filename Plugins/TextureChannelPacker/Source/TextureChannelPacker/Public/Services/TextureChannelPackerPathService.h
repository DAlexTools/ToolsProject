// Copyright 2025 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class TEXTURECHANNELPACKER_API FTextureChannelPackerPathService final
{
public:
	static FString NormalizeOutputPath(const FString& InPath);
	static FString SanitizeAssetName(const FString& InAssetName);
	static FString BuildPackageName(const FString& OutputPackagePath, const FString& AssetName);
	static bool MakeUniqueAssetPackageName(const FString& OutputPackagePath, const FString& AssetName, FString& OutPackageName, FString& OutAssetName);
};
