// Copyright 2025 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Models/TextureChannelPackerTypes.h"
#include "TextureChannelPackerSettings.generated.h"

/**
 * @class UTextureChannelPackerSettings
 * @brief Configuration settings for the Texture Channel Packer system.
 *
 * This class manages user-configurable presets and settings for packing multiple
 * texture channels into a single texture. It inherits from UDeveloperSettings to
 * provide editor-integrated configuration that can be saved per-project.
 *
 * The Texture Channel Packer allows combining grayscale or single-channel textures
 * (e.g., R, G, B, A masks) into a single RGBA texture to optimize memory usage
 * and shader complexity.
 *
 * @note This class is marked as config = Editor and defaultconfig, meaning
 *       settings are stored in the project's DefaultEditor.ini file.
 *
 * @see FTextureChannelPackerPreset
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Texture Channel Packer"))
class TEXTURECHANNELPACKER_API UTextureChannelPackerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * @brief Default constructor.
	 *
	 * Initializes the settings object with default values.
	 * Called when the settings are first loaded or created.
	 */
	UTextureChannelPackerSettings();

	/** 
	 * @brief Gets the singleton instance of the settings (const version).
	 * 
	 * @return Pointer to the singleton settings instance, or nullptr if not available.
	 * 
	 * @note This is the preferred way to access settings for read-only operations.
	 * @see GetMutable() for read-write access.
	 */
	static const UTextureChannelPackerSettings* Get();

	/**
	 * @brief Gets the singleton instance of the settings (mutable version).
	 * 
	 * @return Pointer to the singleton settings instance, or nullptr if not available.
	 * 
	 * @warning Use the method only when you need to modify settings.
	 *				For read-only access, prefer Get().
	 * @see Get() for const access.
	 */
	static UTextureChannelPackerSettings* GetMutable();

#if WITH_EDITOR
	/**
	 * @brief Gets the display name for this settings section in the Editor.
	 * 
	 * @return Localized text to display as the section header in the Editor's
	 *		Project Settings dialog.
	 * 
	 * @note Thes method is only available in Editor builds.
	 * @override UDeveloperSettings::GetSectionText()
	 */
	virtual FText GetSectionText() const override;
#endif
	/**
	 * @brief Collection of user-defined packing presets.
	 * 
	 * Each preset defines a mapping of source textures to target RGBA channels.
	 * These presets can be selected, saved, and loaded via the Editor UI or
	 * programmatically.
	 * 
	 * @note The array is configurable in the Editor and saved to project settings.
	 * @see FTextureChannelPackerPresset for preset structure definition.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Presets")
	TArray<FTextureChannelPackerPreset> Presets;

	/**
	 * @brief Find as a preset by its now.
	 * 
	 * @param PresetName The name of the preset to search for.
	 * @return Pointer to the found preset, or nullptr if no preset with the 
	 *					specified name exists.
	 * 
	 * @see SavePreset() for adding or updating presets.
	 */
	const FTextureChannelPackerPreset* FindPreset(const FString& PresetName) const;

	/**
	 * @brief Saves or updates a preset ain the settings.
	 * 
	 * If a preset with the same name already exists, it will be replaced.
	 * Otherwise, the new preset will be added to the Presets array.
	 * 
	 * @param Preset The preset to save. Must have a valid name.
	 * 
	 * @note The preset's name (FTextureChannelPackerPreset::Name) is used as the unique identifier.
	 * @note After saving, the settings should be saved to disk using 
	 *		SaveConfig() or the Editor's save mechanism.
	 * 
	 * @warning Invalid or emptry preset names may couse undefined behavior. 
	 * 
	 * @see FindPreset() for retrieving saved presets.
	 */
	void SavePreset(const FTextureChannelPackerPreset& Preset);
};
