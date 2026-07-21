// Copyright 2025 DimAlek. All Rights Reserved.

#include "Settings/TextureChannelPackerSettings.h"

#define LOCTEXT_NAMESPACE "TextureChannelPackerSettings"

namespace TextureChannelPackerSettings
{
	static FTextureChannelPackerPresetChannel MakeChannelPreset(
		const ETextureChannelPackerChannel SourceChannel,
		const ETextureChannelPackerChannel OutputChannel,
		const FString& SourcePattern,
		const FString& OutputSuffix,
		const bool bUseConstant = false,
		const uint8 ConstantValue = 0)
	{
		FTextureChannelPackerPresetChannel Channel;
		Channel.SourceChannel = SourceChannel;
		Channel.OutputChannel = OutputChannel;
		Channel.SourcePattern = SourcePattern;
		Channel.OutputSuffix = OutputSuffix;
		Channel.bUseConstant = bUseConstant;
		Channel.ConstantValue = ConstantValue;
		return Channel;
	}
}

UTextureChannelPackerSettings::UTextureChannelPackerSettings()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("TextureChannelPacker");

	if (Presets.Num() == 0)
	{
		FTextureChannelPackerPreset OrmPack;
		OrmPack.Name = TEXT("Pack ORM");
		OrmPack.Operation = ETextureChannelPackerOperation::Pack;
		OrmPack.OutputSuffix = TEXT("_ORM");
		OrmPack.OutputSettings.CompressionSettings = TC_Masks;
		OrmPack.OutputSettings.bSRGB = false;
		OrmPack.Channels = 
		{
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Red, ETextureChannelPackerChannel::Red, TEXT("_AO;_Occlusion;AmbientOcclusion"), TEXT("_AO")),
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Red, ETextureChannelPackerChannel::Green, TEXT("_R;_Roughness;Roughness"), TEXT("_R")),
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Red, ETextureChannelPackerChannel::Blue, TEXT("_M;_Metallic;Metallic"), TEXT("_M")),
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Alpha, ETextureChannelPackerChannel::Alpha, TEXT(""), TEXT("_A"), true, 255)
		};
		Presets.Add(OrmPack);

		FTextureChannelPackerPreset OrmToRmo;
		OrmToRmo.Name = TEXT("Repack ORM to RMO");
		OrmToRmo.Operation = ETextureChannelPackerOperation::Repack;
		OrmToRmo.OutputSuffix = TEXT("_RMO");
		OrmToRmo.OutputSettings.CompressionSettings = TC_Masks;
		OrmToRmo.OutputSettings.bSRGB = false;
		OrmToRmo.Channels = 
		{
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Green, ETextureChannelPackerChannel::Red, TEXT("_ORM"), TEXT("_R")),
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Blue, ETextureChannelPackerChannel::Green, TEXT("_ORM"), TEXT("_M")),
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Red, ETextureChannelPackerChannel::Blue, TEXT("_ORM"), TEXT("_AO")),
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Alpha, ETextureChannelPackerChannel::Alpha, TEXT(""), TEXT("_A"), true, 255)
		};
		Presets.Add(OrmToRmo);

		FTextureChannelPackerPreset UnpackOrm;
		UnpackOrm.Name = TEXT("Unpack ORM");
		UnpackOrm.Operation = ETextureChannelPackerOperation::Unpack;
		UnpackOrm.OutputSuffix = TEXT("");
		UnpackOrm.OutputSettings.SourceFormat = ETextureChannelPackerOutputFormat::Auto;
		UnpackOrm.OutputSettings.CompressionSettings = TC_Grayscale;
		UnpackOrm.OutputSettings.bSRGB = false;
		UnpackOrm.Channels = 
		{
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Red, ETextureChannelPackerChannel::Red, TEXT("_ORM"), TEXT("_AO")),
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Green, ETextureChannelPackerChannel::Red, TEXT("_ORM"), TEXT("_R")),
			TextureChannelPackerSettings::MakeChannelPreset(ETextureChannelPackerChannel::Blue, ETextureChannelPackerChannel::Red, TEXT("_ORM"), TEXT("_M"))
		};
		Presets.Add(UnpackOrm);
	}
}

const UTextureChannelPackerSettings* UTextureChannelPackerSettings::Get()
{
	return GetDefault<UTextureChannelPackerSettings>();
}

UTextureChannelPackerSettings* UTextureChannelPackerSettings::GetMutable()
{
	return GetMutableDefault<UTextureChannelPackerSettings>();
}

#if WITH_EDITOR
FText UTextureChannelPackerSettings::GetSectionText() const
{
	return LOCTEXT("SettingsDisplayName", "Texture Channel Packer");
}
#endif

const FTextureChannelPackerPreset* UTextureChannelPackerSettings::FindPreset(const FString& PresetName) const
{
	return Presets.FindByPredicate([&PresetName](const FTextureChannelPackerPreset& Preset)
	{
		return Preset.Name.Equals(PresetName, ESearchCase::IgnoreCase);
	});
}

void UTextureChannelPackerSettings::SavePreset(const FTextureChannelPackerPreset& Preset)
{
	if (Preset.Name.IsEmpty())
	{
		return;
	}

	if (FTextureChannelPackerPreset* ExistingPreset = Presets.FindByPredicate([&Preset](const FTextureChannelPackerPreset& Existing)
		{
			return Existing.Name.Equals(Preset.Name, ESearchCase::IgnoreCase);
		}))
	{
		*ExistingPreset = Preset;
	}
	else
	{
		Presets.Add(Preset);
	}

	SaveConfig();
}

#undef LOCTEXT_NAMESPACE
