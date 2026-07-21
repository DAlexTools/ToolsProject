// Copyright 2025 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Models/TextureChannelPackerTypes.h"

class UTexture;
class UTexture2D;

class TEXTURECHANNELPACKER_API FTextureChannelPackerTextureService final
{
public:
	static bool CanReadTexture(const UTexture* Texture, FText& OutError);
	static FTextureChannelPackerResult Execute(const FTextureChannelPackerRequest& Request);
	static FTextureChannelPackerPreviewResult BuildPreview(const FTextureChannelPackerRequest& Request, ETextureChannelPackerPreviewMode PreviewMode);
	static FIntPoint GetTextureSourceSize(const UTexture* Texture);
};
