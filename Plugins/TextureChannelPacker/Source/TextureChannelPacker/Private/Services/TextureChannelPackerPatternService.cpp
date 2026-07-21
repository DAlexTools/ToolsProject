// Copyright 2025 DimAlek. All Rights Reserved.

#include "Services/TextureChannelPackerPatternService.h"
#include "Engine/Texture.h"
#include "Services/TextureChannelPackerPathService.h"

/**
 * Internal helper functions used for texture name pattern processing 
 */
namespace TextureChannelPackerPattern
{
	/**
	 * Splits a pattern list into individual patterns.
	 * 
	 * Supports both ';' and ',' as separators and trims whitespace from 
	 * each resulting pattern.
	 */
	static TArray<FString> SplitPatterns(const FString& PatternList)
	{
		TArray<FString> Patterns;
		PatternList.ParseIntoArray(Patterns, TEXT(";"), true);
		if (Patterns.Num() == 0)
		{
			PatternList.ParseIntoArray(Patterns, TEXT(","), true);
		}

		for (FString& Pattern : Patterns)
		{
			Pattern.TrimStartAndEndInline();
		}

		return Patterns;
	}
}

bool FTextureChannelPackerPatternService::DoesNameMatchPattern(const FString& TextureName, const FString& PatternList)
{
	if (PatternList.TrimStartAndEnd().IsEmpty())
	{
		return false;
	}

	for (const FString& Pattern : TextureChannelPackerPattern::SplitPatterns(PatternList))
	{
		if (Pattern.IsEmpty())
		{
			continue;
		}

		if (TextureName.MatchesWildcard(Pattern) ||
			TextureName.Contains(Pattern, ESearchCase::IgnoreCase) ||
			TextureName.EndsWith(Pattern, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

FString FTextureChannelPackerPatternService::BuildBaseNameFromTextures(const TArray<UTexture*>& Textures, const FString& FallbackName)
{
	TArray<FString> Names;
	for (const UTexture* Texture : Textures)
	{
		if (Texture)
		{
			Names.Add(Texture->GetName());
		}
	}

	if (Names.Num() == 0)
	{
		return FTextureChannelPackerPathService::SanitizeAssetName(FallbackName);
	}

	FString CommonPrefix = Names[0];
	for (int32 Index = 1; Index < Names.Num(); ++Index)
	{
		const FString& Name = Names[Index];
		int32 CommonLength = 0;
		const int32 MaxLength = FMath::Min(CommonPrefix.Len(), Name.Len());
		for (int32 CharIndex = 0; CharIndex < MaxLength; ++CharIndex)
		{
			if (CommonPrefix[CharIndex] != Name[CharIndex])
			{
				break;
			}

			++CommonLength;
		}

		CommonPrefix = CommonPrefix.Left(CommonLength);
	}

	CommonPrefix.TrimStartAndEndInline();
	while (CommonPrefix.EndsWith(TEXT("_")) || CommonPrefix.EndsWith(TEXT("-")) || CommonPrefix.EndsWith(TEXT(".")))
	{
		CommonPrefix.LeftChopInline(1);
	}

	if (CommonPrefix.Len() < 3)
	{
		CommonPrefix = Names[0];
	}

	if (!CommonPrefix.StartsWith(TEXT("T_")))
	{
		CommonPrefix = TEXT("T_") + CommonPrefix;
	}

	return FTextureChannelPackerPathService::SanitizeAssetName(CommonPrefix);
}

void FTextureChannelPackerPatternService::ApplyPresetPatterns(const FTextureChannelPackerPreset& Preset, const TArray<UTexture*>& Textures, FTextureChannelPackerRequest& Request)
{
	Request.Operation = Preset.Operation;
	Request.OutputSettings = Preset.OutputSettings;
	Request.Mappings.Reset();
	Request.UnpackOutputs.Reset();

	for (const FTextureChannelPackerPresetChannel& PresetChannel : Preset.Channels)
	{
		UTexture* MatchingTexture = nullptr;
		for (UTexture* Texture : Textures)
		{
			if (Texture && DoesNameMatchPattern(Texture->GetName(), PresetChannel.SourcePattern))
			{
				MatchingTexture = Texture;
				break;
			}
		}

		if (Preset.Operation == ETextureChannelPackerOperation::Unpack)
		{
			if (MatchingTexture)
			{
				Request.UnpackSourceTexture = MatchingTexture;
			}

			FTextureChannelPackerUnpackOutput Output;
			Output.SourceChannel = PresetChannel.SourceChannel;
			Output.OutputSuffix = PresetChannel.OutputSuffix;
			Output.bEnabled = true;
			Request.UnpackOutputs.Add(Output);
			continue;
		}

		FTextureChannelPackerMapping Mapping;
		Mapping.OutputChannel = PresetChannel.OutputChannel;
		Mapping.Source.Channel = PresetChannel.SourceChannel;
		Mapping.Source.Texture = MatchingTexture;
		Mapping.Source.bUseConstant = PresetChannel.bUseConstant;
		Mapping.Source.ConstantValue = PresetChannel.ConstantValue;
		Mapping.SourcePattern = PresetChannel.SourcePattern;

		if (Preset.Operation == ETextureChannelPackerOperation::Repack && !MatchingTexture && Textures.Num() > 0)
		{
			Mapping.Source.Texture = Textures[0];
		}

		Request.Mappings.Add(Mapping);
	}

	const FString BaseName = BuildBaseNameFromTextures(Textures, Request.OutputBaseName);
	Request.OutputBaseName = FTextureChannelPackerPathService::SanitizeAssetName(BaseName + Preset.OutputSuffix);
}
