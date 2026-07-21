// Copyright 2025 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "TextureChannelPackerTypes.generated.h"

class UTexture;
class UTexture2D;

/**
 * @brief Supported texture channel packing operations.
 */
UENUM()
enum class ETextureChannelPackerOperation : uint8
{
	/** Pack multiple source textures into a single texture. */
	Pack UMETA(DisplayName = "Pack"),

	/** Repack channels from an existing packed texture. */
	Repack UMETA(DisplayName = "Repack"),

	/** Extract individual channels into separate textures. */
	Unpack UMETA(DisplayName = "Unpack"),

	/** Copy channels without preforming a full packing operation. */
	Copy UMETA(DisplayName = "Copy Channels")
};

/**
 * @brief Texture channel identifiers.
 */
UENUM()
enum class ETextureChannelPackerChannel : uint8
{
	Red UMETA(DisplayName = "R"),
	Green UMETA(DisplayName = "G"),
	Blue UMETA(DisplayName = "B"),
	Alpha UMETA(DisplayName = "A")
};

/**
 * @brief Preview display modes.
 */
enum class ETextureChannelPackerPreviewMode : uint8
{
	Composite,
	Red,
	Green,
	Blue,
	Alpha
};

/**
 * @brief Output pixel formats supported by the packer.
 */
UENUM()
enum class ETextureChannelPackerOutputFormat : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	G8 UMETA(DisplayName = "G8"),
	G16 UMETA(DisplayName = "G16"),
	R16F UMETA(DisplayName = "R16F"),
	R32F UMETA(DisplayName = "R32F"),
	BGRA8 UMETA(DisplayName = "BGRA8"),
	RGBA16 UMETA(DisplayName = "RGBA16"),
	RGBA16F UMETA(DisplayName = "RGBA16F"),
	RGBA32F UMETA(DisplayName = "RGBA32F")
};

/**
 * @brief Output texture creation settings.
 */
USTRUCT()
struct TEXTURECHANNELPACKER_API FTextureChannelPackerOutputSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Texture")
	bool bUseFirstInputResolution = true;

	UPROPERTY(EditAnywhere, Category = "Texture", meta = (ClampMin = "1", ClampMax = "16384", EditCondition = "!bUseFirstInputResolution"))
	int32 Resolution = 2048;

	UPROPERTY(EditAnywhere, Category = "Texture")
	ETextureChannelPackerOutputFormat SourceFormat = ETextureChannelPackerOutputFormat::Auto;

	UPROPERTY(EditAnywhere, Category = "Texture")
	TEnumAsByte<TextureCompressionSettings> CompressionSettings = TC_Masks;

	UPROPERTY(EditAnywhere, Category = "Texture")
	TEnumAsByte<TextureMipGenSettings> MipGenSettings = TMGS_FromTextureGroup;

	UPROPERTY(EditAnywhere, Category = "Texture")
	TEnumAsByte<TextureFilter> Filter = TF_Default;

	UPROPERTY(EditAnywhere, Category = "Texture")
	TEnumAsByte<TextureGroup> LODGroup = TEXTUREGROUP_World;

	UPROPERTY(EditAnywhere, Category = "Texture")
	bool bSRGB = false;

	UPROPERTY(EditAnywhere, Category = "Texture")
	bool bVirtualTextureStreaming = false;
};

/**
 * @brief Describes a single channel mapping inside a preset.
 *
 * Specifies which source channel should be copied,
 * how it is identified, and where it should be written.
 */
USTRUCT()
struct TEXTURECHANNELPACKER_API FTextureChannelPackerPresetChannel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Channel")
	ETextureChannelPackerChannel SourceChannel = ETextureChannelPackerChannel::Red;

	UPROPERTY(EditAnywhere, Category = "Channel")
	ETextureChannelPackerChannel OutputChannel = ETextureChannelPackerChannel::Red;

	UPROPERTY(EditAnywhere, Category = "Channel")
	FString SourcePattern;

	UPROPERTY(EditAnywhere, Category = "Channel")
	FString OutputSuffix;

	UPROPERTY(EditAnywhere, Category = "Channel")
	bool bUseConstant = false;

	UPROPERTY(EditAnywhere, Category = "Channel", meta = (ClampMin = "0", ClampMax = "255"))
	uint8 ConstantValue = 0;
};

/**
 * @brief Preset descrigin a complete packing or unpacking operation.
 *
 * Combines channel mappings, output naming rules and
 * texture creati0n settings into a reusable configuration.
 */
USTRUCT()
struct TEXTURECHANNELPACKER_API FTextureChannelPackerPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Preset")
	FString Name;

	UPROPERTY(EditAnywhere, Category = "Preset")
	ETextureChannelPackerOperation Operation = ETextureChannelPackerOperation::Pack;

	UPROPERTY(EditAnywhere, Category = "Preset")
	FString OutputSuffix = TEXT("_Packed");

	UPROPERTY(EditAnywhere, Category = "Preset")
	TArray<FTextureChannelPackerPresetChannel> Channels;

	UPROPERTY(EditAnywhere, Category = "Preset")
	FTextureChannelPackerOutputSettings OutputSettings;
};

/**
 * @brief Source channel description used during processing.
 *
 * References a source texture together with the channel
 * that should be sampled, or a constant value when enabled.
 */
struct TEXTURECHANNELPACKER_API FTextureChannelPackerSource
{
	TWeakObjectPtr<UTexture> Texture;
	ETextureChannelPackerChannel Channel = ETextureChannelPackerChannel::Red;
	bool bUseConstant = false;
	uint8 ConstantValue = 0;
};

/**
 * @brief Maps a source channel to an output channel.
 */
struct TEXTURECHANNELPACKER_API FTextureChannelPackerMapping
{
	FTextureChannelPackerSource Source;
	ETextureChannelPackerChannel OutputChannel = ETextureChannelPackerChannel::Red;
	FString SourcePattern;
};

/**
 * @brief Describes an output texture generated during an unpack operation.
 */
struct TEXTURECHANNELPACKER_API FTextureChannelPackerUnpackOutput
{
	ETextureChannelPackerChannel SourceChannel = ETextureChannelPackerChannel::Red;
	FString OutputSuffix;
	bool bEnabled = true;
};

struct FChannelRowState
{
	TWeakObjectPtr<UTexture> Texture;
	ETextureChannelPackerChannel SourceChannel = ETextureChannelPackerChannel::Red;
	ETextureChannelPackerChannel OutputChannel = ETextureChannelPackerChannel::Red;
	bool bUseConstant = false;
	int32 ConstantValue = 0;
	FString SourcePattern;
};

/**
 * @brief Request describing a texture channel packing operation.
 *
 * Contains all information required to execute a pack,
 * repack, unpack or copy operation.
 */
struct TEXTURECHANNELPACKER_API FTextureChannelPackerRequest
{
	ETextureChannelPackerOperation Operation = ETextureChannelPackerOperation::Pack;
	FString OutputPackagePath = TEXT("/Game");
	FString OutputBaseName = TEXT("T_Packed_Texture");
	FTextureChannelPackerOutputSettings OutputSettings;
	TArray<FTextureChannelPackerMapping> Mappings;
	TWeakObjectPtr<UTexture> UnpackSourceTexture;
	TArray<FTextureChannelPackerUnpackOutput> UnpackOutputs;
};

/**
 * @brief Result returned after executing a texture packing operation.
 *
 * Contains execution status, a descriptive message and
 * references to any textures created during the operation.
 */
struct TEXTURECHANNELPACKER_API FTextureChannelPackerResult
{
	bool bSuccess = false;
	FText Message;
	TArray<TWeakObjectPtr<UTexture2D>> CreatedTextures;
};

/**
 * @brief Result returned after generating a preview image.
 *
 * Contains the generated preview texture together with
 * its dimensions and execution status.
 */
struct TEXTURECHANNELPACKER_API FTextureChannelPackerPreviewResult
{
	bool bSuccess = false;
	FText Message;
	UTexture2D* PreviewTexture = nullptr;
	FIntPoint PreviewSize = FIntPoint::ZeroValue;
};
