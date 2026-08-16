#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PerformanceCaptureTypes.h"
#include "PerformanceInspectorAutomationSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Performance Inspector"))
class PERFORMANCEINSPECTORRUNTIME_API UPerformanceInspectorAutomationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Config, Category = "PIE Automation")
	bool bAutoStartCaptureOnPIE = false;

	UPROPERTY(EditAnywhere, Config, Category = "PIE Automation", meta = (ClampMin = "0.0"))
	float PIECaptureDurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, Config, Category = "PIE Automation")
	bool bStopCaptureOnEndPIE = true;

	UPROPERTY(EditAnywhere, Config, Category = "PIE Automation")
	FPerformanceCaptureThresholdSettings PIEThresholds;

	UPROPERTY(EditAnywhere, Config, Category = "Command Line / CI")
	bool bEnableCommandLineAutoCapture = true;

	UPROPERTY(EditAnywhere, Config, Category = "Command Line / CI", meta = (ClampMin = "0.0"))
	float DefaultCommandLineCaptureDurationSeconds = 30.0f;

	UPROPERTY(EditAnywhere, Config, Category = "Command Line / CI")
	bool bExitProcessOnCommandLineCaptureComplete = true;

	UPROPERTY(EditAnywhere, Config, Category = "Command Line / CI")
	bool bUseFailExitCodeOnThresholdFailure = true;

	UPROPERTY(EditAnywhere, Config, Category = "Command Line / CI")
	FPerformanceCaptureThresholdSettings DefaultCommandLineThresholds;

	UPROPERTY(EditAnywhere, Config, Category = "Export")
	bool bEnableSummaryPngExport = false;

	virtual FName GetCategoryName() const override;
};
