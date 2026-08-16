#pragma once

#include "CoreMinimal.h"
#include "PerformanceCaptureTypes.generated.h"

USTRUCT(BlueprintType)
struct PERFORMANCEINSPECTORRUNTIME_API FPerformanceCaptureThresholdSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MinAverageFPS = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxAverageFrameTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxAverageGameThreadTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxAverageRenderThreadTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxAverageInputLatencyTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxAverageRHITTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxPeakFrameTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxPeakGameThreadTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxPeakRenderThreadTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxPeakInputLatencyTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance Capture")
	float MaxPeakRHITTimeMs = 0.0f;

	bool HasAnyThresholds() const
	{
		return MinAverageFPS > 0.0f
			|| MaxAverageFrameTimeMs > 0.0f
			|| MaxAverageGameThreadTimeMs > 0.0f
			|| MaxAverageRenderThreadTimeMs > 0.0f
			|| MaxAverageInputLatencyTimeMs > 0.0f
			|| MaxAverageRHITTimeMs > 0.0f
			|| MaxPeakFrameTimeMs > 0.0f
			|| MaxPeakGameThreadTimeMs > 0.0f
			|| MaxPeakRenderThreadTimeMs > 0.0f
			|| MaxPeakInputLatencyTimeMs > 0.0f
			|| MaxPeakRHITTimeMs > 0.0f;
	}
};

USTRUCT(BlueprintType)
struct PERFORMANCEINSPECTORRUNTIME_API FPerformanceCaptureReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	bool bWasRecording = false;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	int32 SampleCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float AverageFPS = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float MinFPS = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float MaxFPS = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString DominantBottleneck;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float AverageFrameTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float MinFrameTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float MaxFrameTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float AverageGameThreadTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float MaxGameThreadTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float AverageRenderThreadTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float MaxRenderThreadTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float AverageInputLatencyTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float MaxInputLatencyTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float AverageRHITTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	float MaxRHITTimeMs = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	bool bThresholdsConfigured = false;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	bool bPassedThresholds = true;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	TArray<FString> ThresholdFailures;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FPerformanceCaptureThresholdSettings AppliedThresholds;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString CsvPath;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString JsonPath;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString SummaryCsvPath;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString SummaryJsonPath;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString SummaryPngPath;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString EventsCsvPath;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString EventsJsonPath;
};

UENUM(BlueprintType)
enum class EPerformanceCaptureEventSeverity : uint8
{
	Info,
	Warning,
	Critical
};

USTRUCT(BlueprintType)
struct PERFORMANCEINSPECTORRUNTIME_API FPerformanceCaptureEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FDateTime TimestampUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	double ElapsedSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FName Name;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString Category;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FString Details;

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	FLinearColor Color = FLinearColor(0.95f, 0.75f, 0.18f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Performance Capture")
	EPerformanceCaptureEventSeverity Severity = EPerformanceCaptureEventSeverity::Info;
};
