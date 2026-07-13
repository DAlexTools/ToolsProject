// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DataAssetManagerSettings.generated.h"

/**
 * @brief Configurable editor settings for the Data Asset Manager plugin.
 */
UCLASS(Config = Engine, defaultconfig)
class DATAASSETMANAGER_API UDataAssetManagerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * @brief Initializes default plugin settings.
	 */
	UDataAssetManagerSettings();

#if WITH_EDITOR
	/**
	 * @brief Returns the display text for the settings section in the editor.
	 * @return Localized section text.
	 */
	virtual FText GetSectionText() const override;
#endif

	/** @brief Long package paths scanned for Data Asset instances. */
	UPROPERTY(Config, EditAnywhere, Category = "Settings", meta = (RelativePath, LongPackageName))
	TArray<FDirectoryPath> ScannedAssetDirectories = { { TEXT("/Game") } };

	/** @brief Data Asset classes excluded from scan results. */
	UPROPERTY(Config, EditAnywhere, Category = "Settings", meta = (AllowedClasses = "/Script/Engine.DataAsset"))
	TArray<TSubclassOf<UDataAsset>> ExcludedScanAssetTypes;

	/** @brief URL opened by the Documentation menu entry. */
	UPROPERTY(Config, EditAnywhere, Category = "Settings")
	FString DocumentationURL;

	/** @brief Fallback minimum value used by numeric randomization for integer properties without ClampMin metadata. */
	UPROPERTY(Config, EditAnywhere, Category = "Settings|Randomization")
	int32 RandomIntegerClampMin = 0;

	/** @brief Fallback maximum value used by numeric randomization for integer properties without ClampMax metadata. */
	UPROPERTY(Config, EditAnywhere, Category = "Settings|Randomization")
	int32 RandomIntegerClampMax = 100;

	/** @brief Fallback minimum value used by numeric randomization for floating point properties without ClampMin metadata. */
	UPROPERTY(Config, EditAnywhere, Category = "Settings|Randomization")
	float RandomFloatClampMin = 0.0f;

	/** @brief Fallback maximum value used by numeric randomization for floating point properties without ClampMax metadata. */
	UPROPERTY(Config, EditAnywhere, Category = "Settings|Randomization")
	float RandomFloatClampMax = 1.0f;

	/** @brief Root Object Customization title color */
	UPROPERTY(Config, EditAnywhere, Category = "Settings|Customization")
	FLinearColor RootCustomColor = FLinearColor(0.2f, 0.4f, 0.8f, 1.f);

};
