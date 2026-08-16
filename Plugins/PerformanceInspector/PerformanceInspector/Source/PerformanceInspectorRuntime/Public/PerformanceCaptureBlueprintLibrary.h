#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PerformanceCaptureTypes.h"
#include "PerformanceCaptureBlueprintLibrary.generated.h"

class UPerformanceCaptureSubsystem;

UCLASS()
class PERFORMANCEINSPECTORRUNTIME_API UPerformanceCaptureBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Performance Capture", meta = (WorldContext = "WorldContextObject"))
	static void StartPerformanceCapture(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Performance Capture", meta = (WorldContext = "WorldContextObject"))
	static void StartTimedPerformanceCapture(UObject* WorldContextObject, float DurationSeconds,
		const FPerformanceCaptureThresholdSettings& Thresholds, bool bExitProcessOnCompletion = false,
		bool bFailExitCodeOnThresholdFailure = false);

	UFUNCTION(BlueprintCallable, Category = "Performance Capture", meta = (WorldContext = "WorldContextObject"))
	static FPerformanceCaptureReport StopPerformanceCapture(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Performance Capture", meta = (WorldContext = "WorldContextObject"))
	static bool IsPerformanceCaptureActive(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Performance Capture", meta = (WorldContext = "WorldContextObject"))
	static int32 GetPerformanceCaptureSampleCount(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Performance Capture", meta = (WorldContext = "WorldContextObject"))
	static float GetPerformanceCaptureDurationSeconds(UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Performance Capture", meta = (WorldContext = "WorldContextObject"))
	static FPerformanceCaptureReport GetLastPerformanceCaptureReport(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Performance Capture", meta = (WorldContext = "WorldContextObject"))
	static void AddPerformanceMarker(UObject* WorldContextObject, FName Name, const FString& Category = TEXT("Gameplay"),
		const FString& Details = TEXT(""), EPerformanceCaptureEventSeverity Severity = EPerformanceCaptureEventSeverity::Info,
		FLinearColor Color = FLinearColor(0.95f, 0.75f, 0.18f, 1.0f));

private:
	static UPerformanceCaptureSubsystem* ResolveSubsystem(UObject* WorldContextObject);
};
