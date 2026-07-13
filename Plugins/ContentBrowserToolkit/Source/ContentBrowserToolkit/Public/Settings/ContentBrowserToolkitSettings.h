#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "ContentBrowserToolkitSettings.generated.h"

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Content Browser Toolkit"))
class CONTENTBROWSERTOOLKIT_API UContentBrowserToolkitSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UContentBrowserToolkitSettings* Get();
	static UContentBrowserToolkitSettings* GetMutable();

	UPROPERTY(EditAnywhere, config, Category = "Thresholds", meta = (ClampMin = "1", UIMin = "1", DisplayName = "Large Asset Threshold MB"))
	int32 LargeAssetThresholdMB = 50;

	UPROPERTY(EditAnywhere, config, Category = "Thresholds", meta = (ClampMin = "1", UIMin = "1", DisplayName = "Tiny Asset Threshold KB"))
	int32 TinyAssetThresholdKB = 4;

	UPROPERTY(EditAnywhere, config, Category = "Thresholds", meta = (ClampMin = "256", UIMin = "256", DisplayName = "Large Texture Max Size"))
	int32 LargeTextureMaxSize = 4096;

	UPROPERTY(EditAnywhere, config, Category = "Thresholds", meta = (ClampMin = "1", UIMin = "1", DisplayName = "High Triangle Threshold"))
	int32 HighTriangleThreshold = 100000;

	UPROPERTY(EditAnywhere, config, Category = "Thresholds", meta = (ClampMin = "1", UIMin = "1", DisplayName = "Too Many Material Slots Threshold"))
	int32 TooManyMaterialSlotsThreshold = 8;

	UPROPERTY(EditAnywhere, config, Category = "Thresholds", meta = (ClampMin = "1", UIMin = "1", DisplayName = "Too Many UV Channels Threshold"))
	int32 TooManyUVChannelsThreshold = 4;

	UPROPERTY(EditAnywhere, config, Category = "Thresholds", meta = (ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "100", DisplayName = "Low Audio Quality Threshold"))
	int32 LowAudioQualityThreshold = 35;

	UPROPERTY(EditAnywhere, config, Category = "Audit", meta = (DisplayName = "Enable Cached Audit Filters"))
	bool bEnableCachedAuditFilters = true;
};
