// Copyright 2025 DimAlek. All Rights Reserved.

#include "Services/TextureChannelPackerTextureService.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Logging/LogMacros.h"
#include "Math/Float16.h"
#include "Misc/ScopedSlowTask.h"
#include "ScopedTransaction.h"
#include "Services/TextureChannelPackerPathService.h"

#define LOCTEXT_NAMESPACE "TextureChannelPackerTextureService"

DEFINE_LOG_CATEGORY_STATIC(LogTextureChannelPacker, Log, All);

namespace TextureChannelPackerTexture
{
	enum class EChannelValueType : uint8
	{
		UInt8,
		UInt16,
		Float16,
		Float32
	};

	struct FChannelData
	{
		int32 Width = 0;
		int32 Height = 0;
		EChannelValueType ValueType = EChannelValueType::UInt8;
		TArray<uint8> Values;
	};

	struct FResolvedOutputChannel
	{
		ETextureChannelPackerChannel OutputChannel = ETextureChannelPackerChannel::Red;
		bool bHasData = false;
		bool bUseConstant = false;
		uint8 ConstantValue = 0;
		FChannelData Data;
	};

	static int32 GetChannelIndex(const ETextureChannelPackerChannel Channel)
	{
		switch (Channel)
		{
			case ETextureChannelPackerChannel::Red:
				{
					return 0;
				}
			case ETextureChannelPackerChannel::Green:
				{
					return 1;
				}
			case ETextureChannelPackerChannel::Blue:
				{
					return 2;
				}
			case ETextureChannelPackerChannel::Alpha:
				{
					return 3;
				}
			default:
				{
					return 0;
				}
		}
	}

	static int32 GetBytesPerSample(const EChannelValueType ValueType)
	{
		switch (ValueType)
		{
			case EChannelValueType::UInt8:
				{
					return sizeof(uint8);
				}
			case EChannelValueType::UInt16:
				{
					return sizeof(uint16);
				}
			case EChannelValueType::Float16:
				{
					return sizeof(FFloat16);
				}
			case EChannelValueType::Float32:
				{
					return sizeof(float);
				}
			default:
				{
					return sizeof(uint8);
				}
		}
	}

	static void InitializeChannel(FChannelData& ChannelData, const int32 Width, const int32 Height, const EChannelValueType ValueType)
	{
		ChannelData.Width = Width;
		ChannelData.Height = Height;
		ChannelData.ValueType = ValueType;
		ChannelData.Values.SetNumUninitialized(Width * Height * GetBytesPerSample(ValueType));
	}

	static void SetValue(FChannelData& ChannelData, const int32 PixelIndex, const double Value)
	{
		uint8* Destination = ChannelData.Values.GetData() + PixelIndex * GetBytesPerSample(ChannelData.ValueType);
		switch (ChannelData.ValueType)
		{
			case EChannelValueType::UInt8:
				{
					const uint8 ConvertedValue = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Value * 255.0), 0, 255));
					FMemory::Memcpy(Destination, &ConvertedValue, sizeof(ConvertedValue));
					break;
				}
			case EChannelValueType::UInt16:
				{
					const uint16 ConvertedValue = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Value * 65535.0), 0, 65535));
					FMemory::Memcpy(Destination, &ConvertedValue, sizeof(ConvertedValue));
					break;
				}
			case EChannelValueType::Float16:
				{
					const FFloat16 ConvertedValue(static_cast<float>(Value));
					FMemory::Memcpy(Destination, &ConvertedValue, sizeof(ConvertedValue));
					break;
				}
			case EChannelValueType::Float32:
				{
					const float ConvertedValue = static_cast<float>(Value);
					FMemory::Memcpy(Destination, &ConvertedValue, sizeof(ConvertedValue));
					break;
				}
			default:
				break;
		}
	}

	static void SetRawUInt8(FChannelData& ChannelData, const int32 PixelIndex, const uint8 Value)
	{
		check(ChannelData.ValueType == EChannelValueType::UInt8);
		ChannelData.Values[PixelIndex] = Value;
	}

	static void SetRawUInt16(FChannelData& ChannelData, const int32 PixelIndex, const uint16 Value)
	{
		check(ChannelData.ValueType == EChannelValueType::UInt16);
		FMemory::Memcpy(ChannelData.Values.GetData() + PixelIndex * sizeof(uint16), &Value, sizeof(Value));
	}

	static void SetRawFloat16(FChannelData& ChannelData, const int32 PixelIndex, const FFloat16 Value)
	{
		check(ChannelData.ValueType == EChannelValueType::Float16);
		FMemory::Memcpy(ChannelData.Values.GetData() + PixelIndex * sizeof(FFloat16), &Value, sizeof(Value));
	}

	static void SetRawFloat32(FChannelData& ChannelData, const int32 PixelIndex, const float Value)
	{
		check(ChannelData.ValueType == EChannelValueType::Float32);
		FMemory::Memcpy(ChannelData.Values.GetData() + PixelIndex * sizeof(float), &Value, sizeof(Value));
	}

	static double GetValueAsUnitDouble(const FChannelData& ChannelData, const int32 PixelIndex)
	{
		const uint8* Source = ChannelData.Values.GetData() + PixelIndex * GetBytesPerSample(ChannelData.ValueType);
		switch (ChannelData.ValueType)
		{
			case EChannelValueType::UInt8:
				{
					return static_cast<double>(*Source) / 255.0;
				}
			case EChannelValueType::UInt16:
				{
					uint16 Value = 0;
					FMemory::Memcpy(&Value, Source, sizeof(Value));
					return static_cast<double>(Value) / 65535.0;
				}
			case EChannelValueType::Float16:
				{
					FFloat16 Value;
					FMemory::Memcpy(&Value, Source, sizeof(Value));
					return static_cast<float>(Value);
				}
			case EChannelValueType::Float32:
				{
					float Value = 0.0f;
					FMemory::Memcpy(&Value, Source, sizeof(Value));
					return Value;
				}
			default:
				return 0.0;
		}
	}

	static int32 GetSampleIndex(const FChannelData& ChannelData, const int32 TargetX, const int32 TargetY, const int32 TargetWidth, const int32 TargetHeight)
	{
		const int32 SourceX = FMath::Clamp(static_cast<int32>((static_cast<int64>(TargetX) * ChannelData.Width) / FMath::Max(TargetWidth, 1)), 0, ChannelData.Width - 1);
		const int32 SourceY = FMath::Clamp(static_cast<int32>((static_cast<int64>(TargetY) * ChannelData.Height) / FMath::Max(TargetHeight, 1)), 0, ChannelData.Height - 1);

		return SourceY * ChannelData.Width + SourceX;
	}

	static double GetOutputChannelValue(const FResolvedOutputChannel* Channel, const ETextureChannelPackerChannel OutputChannel, const int32 X, const int32 Y, const int32 Width, const int32 Height)
	{
		if (!Channel)
		{
			return OutputChannel == ETextureChannelPackerChannel::Alpha ? 1.0 : 0.0;
		}

		if (Channel->bUseConstant)
		{
			return static_cast<double>(Channel->ConstantValue) / 255.0;
		}

		if (!Channel->bHasData)
		{
			return OutputChannel == ETextureChannelPackerChannel::Alpha ? 1.0 : 0.0;
		}

		const int32 SourceIndex = GetSampleIndex(Channel->Data, X, Y, Width, Height);
		return GetValueAsUnitDouble(Channel->Data, SourceIndex);
	}

	static EChannelValueType GetMaxValueType(const TArray<FResolvedOutputChannel>& Channels)
	{
		EChannelValueType Result = EChannelValueType::UInt8;
		for (const FResolvedOutputChannel& Channel : Channels)
		{
			if (!Channel.bHasData)
			{
				continue;
			}

			if (Channel.Data.ValueType == EChannelValueType::Float32)
			{
				return EChannelValueType::Float32;
			}

			if (Channel.Data.ValueType == EChannelValueType::Float16)
			{
				Result = EChannelValueType::Float16;
			}
			else if (Channel.Data.ValueType == EChannelValueType::UInt16 && Result == EChannelValueType::UInt8)
			{
				Result = EChannelValueType::UInt16;
			}
		}

		return Result;
	}

	static ETextureSourceFormat ResolveAutoFormat(const TArray<FResolvedOutputChannel>& Channels, const bool bSingleChannel)
	{
		switch (GetMaxValueType(Channels))
		{
			case EChannelValueType::Float32:
				{
					return bSingleChannel ? TSF_R32F : TSF_RGBA32F;
				}
			case EChannelValueType::Float16:
				{
					return bSingleChannel ? TSF_R16F : TSF_RGBA16F;
				}
			case EChannelValueType::UInt16:
				{
					return bSingleChannel ? TSF_G16 : TSF_RGBA16;
				}
			case EChannelValueType::UInt8:
			default:
				{
					return bSingleChannel ? TSF_G8 : TSF_BGRA8;
				}
		}
	}

	static ETextureSourceFormat ResolveOutputFormat(const FTextureChannelPackerOutputSettings& Settings, const TArray<FResolvedOutputChannel>& Channels, const bool bSingleChannel)
	{
		switch (Settings.SourceFormat)
		{
			case ETextureChannelPackerOutputFormat::G8:
				{
					return TSF_G8;
				}
			case ETextureChannelPackerOutputFormat::G16:
				{
					return TSF_G16;
				}
			case ETextureChannelPackerOutputFormat::R16F:
				{
					return TSF_R16F;
				}
			case ETextureChannelPackerOutputFormat::R32F:
				{
					return TSF_R32F;
				}
			case ETextureChannelPackerOutputFormat::BGRA8:
				{
					return TSF_BGRA8;
				}
			case ETextureChannelPackerOutputFormat::RGBA16:
				{
					return TSF_RGBA16;
				}
			case ETextureChannelPackerOutputFormat::RGBA16F:
				{
					return TSF_RGBA16F;
				}
			case ETextureChannelPackerOutputFormat::RGBA32F:
				{
					return TSF_RGBA32F;
				}
			case ETextureChannelPackerOutputFormat::Auto:
			default:
				{
					return ResolveAutoFormat(Channels, bSingleChannel);
				}
		}
	}

	static bool ReadTextureChannel(UTexture* Texture, const ETextureChannelPackerChannel Channel, FChannelData& OutData, FText& OutError)
	{
		UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
		if (!Texture2D)
		{
			OutError = LOCTEXT("UnsupportedTextureClass", "Only Texture2D assets with editor source data can be processed.");
			return false;
		}

#if WITH_EDITORONLY_DATA
		const int32 Width = Texture2D->Source.GetSizeX();
		const int32 Height = Texture2D->Source.GetSizeY();
		const ETextureSourceFormat SourceFormat = Texture2D->Source.GetFormat();
		if (Width <= 0 || Height <= 0 || SourceFormat == TSF_Invalid)
		{
			OutError = FText::Format(LOCTEXT("MissingSourceData", "Texture '{0}' has no readable editor source data."), FText::FromString(Texture2D->GetName()));
			return false;
		}

		const int32 ChannelIndex = GetChannelIndex(Channel);
		uint8* SourceData = Texture2D->Source.LockMip(0);
		if (!SourceData)
		{
			OutError = FText::Format(LOCTEXT("LockMipFailed", "Could not lock source data for texture '{0}'."), FText::FromString(Texture2D->GetName()));
			return false;
		}

		const int32 PixelCount = Width * Height;
		switch (SourceFormat)
		{
			case TSF_BGRA8:
				{
					InitializeChannel(OutData, Width, Height, EChannelValueType::UInt8);
					for (int32 Index = 0; Index < PixelCount; ++Index)
					{
						const uint8* Pixel = SourceData + Index * 4;
						const uint8 Value = ChannelIndex == 0 ? Pixel[2] : ChannelIndex == 1 ? Pixel[1] : ChannelIndex == 2 ? Pixel[0] : Pixel[3];
						SetRawUInt8(OutData, Index, Value);
					}
					break;
				}
			case TSF_G8:
				{
					InitializeChannel(OutData, Width, Height, EChannelValueType::UInt8);
					for (int32 Index = 0; Index < PixelCount; ++Index)
					{
						SetRawUInt8(OutData, Index, Channel == ETextureChannelPackerChannel::Alpha ? 255 : SourceData[Index]);
					}
					break;
				}
			case TSF_G16:
				{
					InitializeChannel(OutData, Width, Height, EChannelValueType::UInt16);
					const uint16* SourcePixels = reinterpret_cast<const uint16*>(SourceData);
					for (int32 Index = 0; Index < PixelCount; ++Index)
					{
						SetRawUInt16(OutData, Index, Channel == ETextureChannelPackerChannel::Alpha ? 65535 : SourcePixels[Index]);
					}
					break;
				}
			case TSF_R16F:
				{
					InitializeChannel(OutData, Width, Height, EChannelValueType::Float16);
					const FFloat16* SourcePixels = reinterpret_cast<const FFloat16*>(SourceData);
					for (int32 Index = 0; Index < PixelCount; ++Index)
					{
						SetRawFloat16(OutData, Index, Channel == ETextureChannelPackerChannel::Alpha ? FFloat16(1.0f) : SourcePixels[Index]);
					}
					break;
				}
			case TSF_R32F:
				{
					InitializeChannel(OutData, Width, Height, EChannelValueType::Float32);
					const float* SourcePixels = reinterpret_cast<const float*>(SourceData);
					for (int32 Index = 0; Index < PixelCount; ++Index)
					{
						SetRawFloat32(OutData, Index, Channel == ETextureChannelPackerChannel::Alpha ? 1.0f : SourcePixels[Index]);
					}
					break;
				}
			case TSF_RGBA16:
				{
					InitializeChannel(OutData, Width, Height, EChannelValueType::UInt16);
					const uint16* SourcePixels = reinterpret_cast<const uint16*>(SourceData);
					for (int32 Index = 0; Index < PixelCount; ++Index)
					{
						SetRawUInt16(OutData, Index, SourcePixels[Index * 4 + ChannelIndex]);
					}
					break;
				}
			case TSF_RGBA16F:
				{
					InitializeChannel(OutData, Width, Height, EChannelValueType::Float16);
					const FFloat16* SourcePixels = reinterpret_cast<const FFloat16*>(SourceData);
					for (int32 Index = 0; Index < PixelCount; ++Index)
					{
						SetRawFloat16(OutData, Index, SourcePixels[Index * 4 + ChannelIndex]);
					}
					break;
				}
			case TSF_RGBA32F:
				{
					InitializeChannel(OutData, Width, Height, EChannelValueType::Float32);
					const float* SourcePixels = reinterpret_cast<const float*>(SourceData);
					for (int32 Index = 0; Index < PixelCount; ++Index)
					{
						SetRawFloat32(OutData, Index, SourcePixels[Index * 4 + ChannelIndex]);
					}
					break;
				}
			default:
				Texture2D->Source.UnlockMip(0);
				OutError = FText::Format(LOCTEXT("UnsupportedSourceFormat", "Texture '{0}' uses unsupported source format {1}."), FText::FromString(Texture2D->GetName()),
										 FText::AsNumber(static_cast<int32>(SourceFormat)));
				return false;
		}

		Texture2D->Source.UnlockMip(0);
		return true;
#else
		OutError = LOCTEXT("EditorOnlyDataRequired", "Texture channel packing requires editor-only texture source data.");
		return false;
#endif
	}

	static void ApplyTextureSettings(UTexture2D* Texture, const FTextureChannelPackerOutputSettings& Settings)
	{
		Texture->CompressionSettings = Settings.CompressionSettings;
		Texture->MipGenSettings = Settings.MipGenSettings;
		Texture->Filter = Settings.Filter;
		Texture->LODGroup = Settings.LODGroup;
		Texture->SRGB = Settings.bSRGB;
		Texture->VirtualTextureStreaming = Settings.bVirtualTextureStreaming;
	}

	static bool WriteTextureSource(UTexture2D* Texture, const int32 Width, const int32 Height, const ETextureSourceFormat OutputFormat, const TArray<FResolvedOutputChannel>& Channels, FText& OutError)
	{
#if WITH_EDITORONLY_DATA
		Texture->Source.Init(Width, Height, 1, 1, OutputFormat);
		uint8* DestinationData = Texture->Source.LockMip(0);
		if (!DestinationData)
		{
			OutError = LOCTEXT("DestinationLockFailed", "Could not lock output texture source data.");
			return false;
		}

		const FResolvedOutputChannel* ChannelByOutput[4] = {nullptr, nullptr, nullptr, nullptr};
		for (const FResolvedOutputChannel& Channel : Channels)
		{
			ChannelByOutput[GetChannelIndex(Channel.OutputChannel)] = &Channel;
		}

		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 PixelIndex = Y * Width + X;
				const double R = GetOutputChannelValue(ChannelByOutput[0], ETextureChannelPackerChannel::Red, X, Y, Width, Height);
				const double G = GetOutputChannelValue(ChannelByOutput[1], ETextureChannelPackerChannel::Green, X, Y, Width, Height);
				const double B = GetOutputChannelValue(ChannelByOutput[2], ETextureChannelPackerChannel::Blue, X, Y, Width, Height);
				const double A = GetOutputChannelValue(ChannelByOutput[3], ETextureChannelPackerChannel::Alpha, X, Y, Width, Height);

				switch (OutputFormat)
				{
					case TSF_G8:
						{
							DestinationData[PixelIndex] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(R * 255.0), 0, 255));
							break;
						}
					case TSF_G16:
						{
							uint16* DestinationPixels = reinterpret_cast<uint16*>(DestinationData);
							DestinationPixels[PixelIndex] = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(R * 65535.0), 0, 65535));
							break;
						}
					case TSF_R16F:
						{
							FFloat16* DestinationPixels = reinterpret_cast<FFloat16*>(DestinationData);
							DestinationPixels[PixelIndex] = FFloat16(static_cast<float>(R));
							break;
						}
					case TSF_R32F:
						{
							float* DestinationPixels = reinterpret_cast<float*>(DestinationData);
							DestinationPixels[PixelIndex] = static_cast<float>(R);
							break;
						}
					case TSF_BGRA8:
						{
							uint8* Pixel = DestinationData + PixelIndex * 4;
							Pixel[0] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(B * 255.0), 0, 255));
							Pixel[1] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(G * 255.0), 0, 255));
							Pixel[2] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(R * 255.0), 0, 255));
							Pixel[3] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(A * 255.0), 0, 255));
							break;
						}
					case TSF_RGBA16:
						{
							uint16* Pixel = reinterpret_cast<uint16*>(DestinationData) + PixelIndex * 4;
							Pixel[0] = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(R * 65535.0), 0, 65535));
							Pixel[1] = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(G * 65535.0), 0, 65535));
							Pixel[2] = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(B * 65535.0), 0, 65535));
							Pixel[3] = static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(A * 65535.0), 0, 65535));
							break;
						}
					case TSF_RGBA16F:
						{
							FFloat16* Pixel = reinterpret_cast<FFloat16*>(DestinationData) + PixelIndex * 4;
							Pixel[0] = FFloat16(static_cast<float>(R));
							Pixel[1] = FFloat16(static_cast<float>(G));
							Pixel[2] = FFloat16(static_cast<float>(B));
							Pixel[3] = FFloat16(static_cast<float>(A));
							break;
						}
					case TSF_RGBA32F:
						{
							float* Pixel = reinterpret_cast<float*>(DestinationData) + PixelIndex * 4;
							Pixel[0] = static_cast<float>(R);
							Pixel[1] = static_cast<float>(G);
							Pixel[2] = static_cast<float>(B);
							Pixel[3] = static_cast<float>(A);
							break;
						}
					default:
						Texture->Source.UnlockMip(0);
						OutError = LOCTEXT("UnsupportedOutputFormat", "The requested output source format is not supported.");
						return false;
				}
			}
		}

		Texture->Source.UnlockMip(0);
		return true;
#else
		OutError = LOCTEXT("EditorOnlyOutputRequired", "Texture channel packing requires editor-only texture source data.");
		return false;
#endif
	}

	static UTexture2D* CreateTextureAsset(const FString& OutputPackagePath, const FString& AssetName, const int32 Width, const int32 Height, const ETextureSourceFormat OutputFormat,
										  const TArray<FResolvedOutputChannel>& Channels, const FTextureChannelPackerOutputSettings& Settings, FText& OutError)
	{
		FString PackageName;
		FString UniqueAssetName;
		if (!FTextureChannelPackerPathService::MakeUniqueAssetPackageName(OutputPackagePath, AssetName, PackageName, UniqueAssetName))
		{
			OutError = FText::Format(LOCTEXT("InvalidPackageName", "Invalid output path or asset name: {0}"), FText::FromString(OutputPackagePath / AssetName));
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			OutError = FText::Format(LOCTEXT("CreatePackageFailed", "Could not create package: {0}"), FText::FromString(PackageName));
			return nullptr;
		}

		Package->FullyLoad();
		Package->Modify();

		UTexture2D* Texture = NewObject<UTexture2D>(Package, FName(*UniqueAssetName), RF_Public | RF_Standalone | RF_Transactional);
		if (!Texture)
		{
			OutError = FText::Format(LOCTEXT("CreateTextureFailed", "Could not create texture asset: {0}"), FText::FromString(UniqueAssetName));
			return nullptr;
		}

		Texture->Modify();
		if (!WriteTextureSource(Texture, Width, Height, OutputFormat, Channels, OutError))
		{
			return nullptr;
		}

		ApplyTextureSettings(Texture, Settings);
		Texture->UpdateResource();
		Texture->PostEditChange();

		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Texture);
		return Texture;
	}

	static bool ResolveChannels(const TArray<FTextureChannelPackerMapping>& Mappings, TArray<FResolvedOutputChannel>& OutChannels, FIntPoint& OutFirstSourceSize, FText& OutError)
	{
		OutChannels.Reset();
		OutFirstSourceSize = FIntPoint::ZeroValue;

		for (const FTextureChannelPackerMapping& Mapping : Mappings)
		{
			FResolvedOutputChannel ResolvedChannel;
			ResolvedChannel.OutputChannel = Mapping.OutputChannel;
			ResolvedChannel.bUseConstant = Mapping.Source.bUseConstant || !Mapping.Source.Texture.IsValid();
			ResolvedChannel.ConstantValue = Mapping.Source.ConstantValue;

			if (!ResolvedChannel.bUseConstant)
			{
				if (!ReadTextureChannel(Mapping.Source.Texture.Get(), Mapping.Source.Channel, ResolvedChannel.Data, OutError))
				{
					return false;
				}

				ResolvedChannel.bHasData = true;
				if (OutFirstSourceSize == FIntPoint::ZeroValue)
				{
					OutFirstSourceSize = FIntPoint(ResolvedChannel.Data.Width, ResolvedChannel.Data.Height);
				}
			}

			OutChannels.Add(MoveTemp(ResolvedChannel));
		}

		return true;
	}

	static FIntPoint ResolveTargetSize(const FTextureChannelPackerOutputSettings& Settings, const FIntPoint FirstSourceSize)
	{
		if (Settings.bUseFirstInputResolution && FirstSourceSize.X > 0 && FirstSourceSize.Y > 0)
		{
			return FirstSourceSize;
		}

		const int32 Resolution = FMath::Clamp(Settings.Resolution, 1, 16384);
		return FIntPoint(Resolution, Resolution);
	}

	static FIntPoint ResolvePreviewSize(const FIntPoint TargetSize)
	{
		constexpr int32 MaxPreviewDimension = 512;
		if (TargetSize.X <= 0 || TargetSize.Y <= 0)
		{
			return FIntPoint::ZeroValue;
		}

		if (TargetSize.X <= MaxPreviewDimension && TargetSize.Y <= MaxPreviewDimension)
		{
			return TargetSize;
		}

		const float Scale = static_cast<float>(MaxPreviewDimension) / static_cast<float>(FMath::Max(TargetSize.X, TargetSize.Y));
		return FIntPoint(FMath::Max(1, FMath::RoundToInt(static_cast<float>(TargetSize.X) * Scale)), FMath::Max(1, FMath::RoundToInt(static_cast<float>(TargetSize.Y) * Scale)));
	}

	static uint8 UnitDoubleToByte(const double Value)
	{
		return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Value * 255.0), 0, 255));
	}

	static double SelectPreviewChannelValue(const ETextureChannelPackerPreviewMode PreviewMode, const double R, const double G, const double B, const double A)
	{
		switch (PreviewMode)
		{
			case ETextureChannelPackerPreviewMode::Red:
				{
					return R;
				}
			case ETextureChannelPackerPreviewMode::Green:
				{
					return G;
				}
			case ETextureChannelPackerPreviewMode::Blue:
				{
					return B;
				}
			case ETextureChannelPackerPreviewMode::Alpha:
				{
					return A;
				}
			case ETextureChannelPackerPreviewMode::Composite:
			default:
				{
					return R;
				}
		}
	}

	static UTexture2D* CreatePreviewTexture(const TArray<FResolvedOutputChannel>& Channels, const FIntPoint TargetSize, const ETextureChannelPackerPreviewMode PreviewMode, const bool bSingleChannel,
											FText& OutError)
	{
		const FIntPoint PreviewSize = ResolvePreviewSize(TargetSize);
		if (PreviewSize == FIntPoint::ZeroValue)
		{
			OutError = LOCTEXT("InvalidPreviewSize", "Could not determine a valid preview size.");
			return nullptr;
		}

		const FResolvedOutputChannel* ChannelByOutput[4] = {nullptr, nullptr, nullptr, nullptr};
		for (const FResolvedOutputChannel& Channel : Channels)
		{
			ChannelByOutput[GetChannelIndex(Channel.OutputChannel)] = &Channel;
		}

		TArray<uint8> Pixels;
		Pixels.SetNumUninitialized(PreviewSize.X * PreviewSize.Y * 4);
		for (int32 Y = 0; Y < PreviewSize.Y; ++Y)
		{
			for (int32 X = 0; X < PreviewSize.X; ++X)
			{
				const int32 PixelIndex = Y * PreviewSize.X + X;
				const double R = GetOutputChannelValue(ChannelByOutput[0], ETextureChannelPackerChannel::Red, X, Y, PreviewSize.X, PreviewSize.Y);
				const double G = GetOutputChannelValue(ChannelByOutput[1], ETextureChannelPackerChannel::Green, X, Y, PreviewSize.X, PreviewSize.Y);
				const double B = GetOutputChannelValue(ChannelByOutput[2], ETextureChannelPackerChannel::Blue, X, Y, PreviewSize.X, PreviewSize.Y);
				const double A = GetOutputChannelValue(ChannelByOutput[3], ETextureChannelPackerChannel::Alpha, X, Y, PreviewSize.X, PreviewSize.Y);

				uint8 OutR = UnitDoubleToByte(R);
				uint8 OutG = UnitDoubleToByte(G);
				uint8 OutB = UnitDoubleToByte(B);
				if (bSingleChannel || PreviewMode != ETextureChannelPackerPreviewMode::Composite)
				{
					const uint8 Value = UnitDoubleToByte(bSingleChannel ? R : SelectPreviewChannelValue(PreviewMode, R, G, B, A));
					OutR = Value;
					OutG = Value;
					OutB = Value;
				}

				uint8* Pixel = Pixels.GetData() + PixelIndex * 4;
				Pixel[0] = OutB;
				Pixel[1] = OutG;
				Pixel[2] = OutR;
				Pixel[3] = 255;
			}
		}

		const FName PreviewTextureName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), TEXT("TextureChannelPackerPreview"));
		UTexture2D* PreviewTexture = UTexture2D::CreateTransient(PreviewSize.X, PreviewSize.Y, PF_B8G8R8A8, PreviewTextureName);
		if (!PreviewTexture || !PreviewTexture->GetPlatformData() || PreviewTexture->GetPlatformData()->Mips.Num() == 0)
		{
			OutError = LOCTEXT("PreviewTextureCreateFailed", "Could not create the preview texture.");
			return nullptr;
		}

		PreviewTexture->MipGenSettings = TMGS_NoMipmaps;
		PreviewTexture->Filter = TF_Bilinear;
		PreviewTexture->SRGB = false;
		PreviewTexture->NeverStream = true;

		FTexture2DMipMap& Mip = PreviewTexture->GetPlatformData()->Mips[0];
		void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
		if (!TextureData)
		{
			OutError = LOCTEXT("PreviewTextureLockFailed", "Could not lock the preview texture.");
			return nullptr;
		}

		FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num());
		Mip.BulkData.Unlock();
		PreviewTexture->UpdateResource();
		return PreviewTexture;
	}

	static FTextureChannelPackerResult ExecutePackLike(const FTextureChannelPackerRequest& Request)
	{
		FTextureChannelPackerResult Result;
		TArray<FResolvedOutputChannel> ResolvedChannels;
		FIntPoint FirstSourceSize;
		if (!ResolveChannels(Request.Mappings, ResolvedChannels, FirstSourceSize, Result.Message))
		{
			return Result;
		}

		if (ResolvedChannels.Num() == 0)
		{
			Result.Message = LOCTEXT("NoChannels", "No channel mappings were provided.");
			return Result;
		}

		const FIntPoint TargetSize = ResolveTargetSize(Request.OutputSettings, FirstSourceSize);
		const ETextureSourceFormat OutputFormat = ResolveOutputFormat(Request.OutputSettings, ResolvedChannels, false);

		FText CreateError;
		UTexture2D* Texture = CreateTextureAsset(Request.OutputPackagePath, Request.OutputBaseName, TargetSize.X, TargetSize.Y, OutputFormat, ResolvedChannels, Request.OutputSettings, CreateError);
		if (!Texture)
		{
			Result.Message = CreateError;
			return Result;
		}

		Result.bSuccess = true;
		Result.CreatedTextures.Add(Texture);
		Result.Message = FText::Format(LOCTEXT("CreatedTexture", "Created texture: {0}"), FText::FromString(Texture->GetPathName()));
		return Result;
	}

	static FTextureChannelPackerResult ExecuteUnpack(const FTextureChannelPackerRequest& Request)
	{
		FTextureChannelPackerResult Result;
		if (!Request.UnpackSourceTexture.IsValid())
		{
			Result.Message = LOCTEXT("MissingUnpackSource", "Select a packed source texture before unpacking.");
			return Result;
		}

		TArray<FTextureChannelPackerUnpackOutput> EnabledOutputs;
		for (const FTextureChannelPackerUnpackOutput& Output : Request.UnpackOutputs)
		{
			if (Output.bEnabled)
			{
				EnabledOutputs.Add(Output);
			}
		}

		if (EnabledOutputs.Num() == 0)
		{
			Result.Message = LOCTEXT("NoUnpackChannels", "Select at least one channel to unpack.");
			return Result;
		}

		for (const FTextureChannelPackerUnpackOutput& Output : EnabledOutputs)
		{
			FChannelData SourceChannel;
			if (!ReadTextureChannel(Request.UnpackSourceTexture.Get(), Output.SourceChannel, SourceChannel, Result.Message))
			{
				return Result;
			}

			FResolvedOutputChannel ResolvedChannel;
			ResolvedChannel.OutputChannel = ETextureChannelPackerChannel::Red;
			ResolvedChannel.bHasData = true;
			ResolvedChannel.Data = MoveTemp(SourceChannel);

			TArray<FResolvedOutputChannel> Channels;
			Channels.Add(MoveTemp(ResolvedChannel));

			FTextureChannelPackerOutputSettings OutputSettings = Request.OutputSettings;
			OutputSettings.CompressionSettings = TC_Grayscale;
			OutputSettings.bSRGB = false;

			const FIntPoint TargetSize = ResolveTargetSize(OutputSettings, FIntPoint(Channels[0].Data.Width, Channels[0].Data.Height));
			const ETextureSourceFormat OutputFormat = ResolveOutputFormat(OutputSettings, Channels, true);
			const FString AssetName = FTextureChannelPackerPathService::SanitizeAssetName(Request.OutputBaseName + Output.OutputSuffix);

			FText CreateError;
			UTexture2D* Texture = CreateTextureAsset(Request.OutputPackagePath, AssetName, TargetSize.X, TargetSize.Y, OutputFormat, Channels, OutputSettings, CreateError);
			if (!Texture)
			{
				Result.Message = CreateError;
				return Result;
			}

			Result.CreatedTextures.Add(Texture);
		}

		Result.bSuccess = Result.CreatedTextures.Num() > 0;
		Result.Message = FText::Format(LOCTEXT("CreatedTextures", "Created {0} texture asset(s)."), FText::AsNumber(Result.CreatedTextures.Num()));
		return Result;
	}

	static FTextureChannelPackerPreviewResult BuildPackLikePreview(const FTextureChannelPackerRequest& Request, const ETextureChannelPackerPreviewMode PreviewMode)
	{
		FTextureChannelPackerPreviewResult Result;
		TArray<FResolvedOutputChannel> ResolvedChannels;
		FIntPoint FirstSourceSize;
		if (!ResolveChannels(Request.Mappings, ResolvedChannels, FirstSourceSize, Result.Message))
		{
			return Result;
		}

		if (ResolvedChannels.Num() == 0)
		{
			Result.Message = LOCTEXT("PreviewNoChannels", "No channel mappings were provided.");
			return Result;
		}

		const FIntPoint TargetSize = ResolveTargetSize(Request.OutputSettings, FirstSourceSize);
		Result.PreviewTexture = CreatePreviewTexture(ResolvedChannels, TargetSize, PreviewMode, false, Result.Message);
		Result.bSuccess = Result.PreviewTexture != nullptr;
		Result.PreviewSize = Result.PreviewTexture ? FIntPoint(Result.PreviewTexture->GetSizeX(), Result.PreviewTexture->GetSizeY()) : FIntPoint::ZeroValue;
		Result.Message = Result.bSuccess ? LOCTEXT("PreviewBuilt", "Preview updated.") : Result.Message;
		return Result;
	}

	static FTextureChannelPackerPreviewResult BuildUnpackPreview(const FTextureChannelPackerRequest& Request, const ETextureChannelPackerPreviewMode PreviewMode)
	{
		FTextureChannelPackerPreviewResult Result;
		if (!Request.UnpackSourceTexture.IsValid())
		{
			Result.Message = LOCTEXT("PreviewMissingUnpackSource", "Select a packed source texture before previewing unpack.");
			return Result;
		}

		const FTextureChannelPackerUnpackOutput* FirstEnabledOutput = nullptr;
		for (const FTextureChannelPackerUnpackOutput& Output : Request.UnpackOutputs)
		{
			if (Output.bEnabled)
			{
				FirstEnabledOutput = &Output;
				break;
			}
		}

		if (!FirstEnabledOutput)
		{
			Result.Message = LOCTEXT("PreviewNoUnpackChannels", "Select at least one channel to preview.");
			return Result;
		}

		FChannelData SourceChannel;
		if (!ReadTextureChannel(Request.UnpackSourceTexture.Get(), FirstEnabledOutput->SourceChannel, SourceChannel, Result.Message))
		{
			return Result;
		}

		FResolvedOutputChannel ResolvedChannel;
		ResolvedChannel.OutputChannel = ETextureChannelPackerChannel::Red;
		ResolvedChannel.bHasData = true;
		ResolvedChannel.Data = MoveTemp(SourceChannel);

		TArray<FResolvedOutputChannel> Channels;
		Channels.Add(MoveTemp(ResolvedChannel));

		const FIntPoint TargetSize = ResolveTargetSize(Request.OutputSettings, FIntPoint(Channels[0].Data.Width, Channels[0].Data.Height));
		Result.PreviewTexture = CreatePreviewTexture(Channels, TargetSize, PreviewMode, true, Result.Message);
		Result.bSuccess = Result.PreviewTexture != nullptr;
		Result.PreviewSize = Result.PreviewTexture ? FIntPoint(Result.PreviewTexture->GetSizeX(), Result.PreviewTexture->GetSizeY()) : FIntPoint::ZeroValue;
		Result.Message = Result.bSuccess ? LOCTEXT("UnpackPreviewBuilt", "Preview updated.") : Result.Message;
		return Result;
	}
} // namespace TextureChannelPackerTexture

FTextureChannelPackerResult FTextureChannelPackerTextureService::Execute(const FTextureChannelPackerRequest& Request)
{
	check(IsInGameThread());

	const FScopedTransaction Transaction(LOCTEXT("TextureChannelPackerTransaction", "Texture Channel Packer Operation"));
	FScopedSlowTask SlowTask(1.0f, LOCTEXT("ProcessingTextures", "Processing texture channels..."));
	SlowTask.MakeDialog(true);

	if (SlowTask.ShouldCancel())
	{
		FTextureChannelPackerResult Result;
		Result.Message = LOCTEXT("OperationCancelled", "Texture channel operation was cancelled.");
		return Result;
	}

	switch (Request.Operation)
	{
		case ETextureChannelPackerOperation::Unpack:
			return TextureChannelPackerTexture::ExecuteUnpack(Request);
		case ETextureChannelPackerOperation::Pack:
		case ETextureChannelPackerOperation::Repack:
		case ETextureChannelPackerOperation::Copy:
		default:
			return TextureChannelPackerTexture::ExecutePackLike(Request);
	}
}

FTextureChannelPackerPreviewResult FTextureChannelPackerTextureService::BuildPreview(const FTextureChannelPackerRequest& Request, const ETextureChannelPackerPreviewMode PreviewMode)
{
	check(IsInGameThread());

	switch (Request.Operation)
	{
		case ETextureChannelPackerOperation::Unpack:
			return TextureChannelPackerTexture::BuildUnpackPreview(Request, PreviewMode);
		case ETextureChannelPackerOperation::Pack:
		case ETextureChannelPackerOperation::Repack:
		case ETextureChannelPackerOperation::Copy:
		default:
			return TextureChannelPackerTexture::BuildPackLikePreview(Request, PreviewMode);
	}
}

bool FTextureChannelPackerTextureService::CanReadTexture(const UTexture* Texture, FText& OutError)
{
	const UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
	if (!Texture2D)
	{
		OutError = LOCTEXT("CanReadUnsupportedClass", "Only Texture2D assets can be read by this tool.");
		return false;
	}

#if WITH_EDITORONLY_DATA
	if (Texture2D->Source.GetSizeX() <= 0 || Texture2D->Source.GetSizeY() <= 0 || Texture2D->Source.GetFormat() == TSF_Invalid)
	{
		OutError = FText::Format(LOCTEXT("CanReadMissingSource", "Texture '{0}' has no readable source data."), FText::FromString(Texture2D->GetName()));
		return false;
	}

	return true;
#else
	OutError = LOCTEXT("CanReadNeedsEditorData", "Editor-only texture source data is required.");
	return false;
#endif
}

FIntPoint FTextureChannelPackerTextureService::GetTextureSourceSize(const UTexture* Texture)
{
	const UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
	if (!Texture2D)
	{
		return FIntPoint::ZeroValue;
	}

#if WITH_EDITORONLY_DATA
	return FIntPoint(Texture2D->Source.GetSizeX(), Texture2D->Source.GetSizeY());
#else
	return FIntPoint::ZeroValue;
#endif
}

#undef LOCTEXT_NAMESPACE
