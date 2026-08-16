#pragma once

#include "CoreMinimal.h"
#include "PerformanceCaptureTypes.h"
#include "UnrealClient.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "PerformanceCaptureSubsystem.generated.h"

struct FRuntimePerformanceCaptureSample
{
	FDateTime TimestampUtc;
	double ElapsedSeconds = 0.0;
	float FrameTimeMs = 0.0f;
	float GameThreadTimeMs = 0.0f;
	float RenderThreadTimeMs = 0.0f;
	float InputLatencyTimeMs = 0.0f;
	float RHITTimeMs = 0.0f;
	TArray<float, TFixedAllocator<MAX_NUM_GPUS>> GPUFrameTimesMs;

	FRuntimePerformanceCaptureSample()
	{
		GPUFrameTimesMs.Init(0.0f, MAX_NUM_GPUS);
	}
};

UCLASS(BlueprintType)
class PERFORMANCEINSPECTORRUNTIME_API UPerformanceCaptureSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override;

	UFUNCTION(BlueprintCallable, Category = "Performance Capture")
	void StartCaptureSession();

	UFUNCTION(BlueprintCallable, Category = "Performance Capture")
	void StartTimedCaptureSession(float DurationSeconds, const FPerformanceCaptureThresholdSettings& Thresholds,
		bool bExitProcessOnCompletion = false, bool bFailExitCodeOnThresholdFailure = false);

	UFUNCTION(BlueprintCallable, Category = "Performance Capture")
	FPerformanceCaptureReport StopCaptureSession();

	UFUNCTION(BlueprintPure, Category = "Performance Capture")
	bool IsCaptureSessionActive() const;

	UFUNCTION(BlueprintPure, Category = "Performance Capture")
	int32 GetCaptureSampleCount() const;

	UFUNCTION(BlueprintPure, Category = "Performance Capture")
	float GetCaptureSessionDurationSeconds() const;

	UFUNCTION(BlueprintPure, Category = "Performance Capture")
	const FPerformanceCaptureReport& GetLastCaptureReport() const;

	UFUNCTION(BlueprintCallable, Category = "Performance Capture")
	void AddEventMarker(FName Name, const FString& Category = TEXT("Gameplay"), const FString& Details = TEXT(""),
		EPerformanceCaptureEventSeverity Severity = EPerformanceCaptureEventSeverity::Info,
		FLinearColor Color = FLinearColor(0.95f, 0.75f, 0.18f, 1.0f));

	static bool BuildReportFromSamples(const TArray<FRuntimePerformanceCaptureSample>& Samples, const TArray<FPerformanceCaptureEvent>& Events,
		const FDateTime& CaptureSessionStartUtc, const FPerformanceCaptureThresholdSettings& Thresholds, FPerformanceCaptureReport& OutReport);
	static void EvaluateReportAgainstThresholds(FPerformanceCaptureReport& InOutReport, const FPerformanceCaptureThresholdSettings& Thresholds);

	const TArray<FPerformanceCaptureEvent>& GetRecordedEvents() const;
	const TArray<FPerformanceCaptureEvent>& GetCaptureSessionEvents() const;

private:
	bool UpdatePerformanceData();
	void UpdateStatsFromData(const FStatUnitData& StatUnitData);
	void AppendCaptureSample();
	void BeginCaptureSession(const FPerformanceCaptureThresholdSettings& Thresholds, float DurationSeconds,
		bool bExitProcessOnCompletion, bool bFailExitCodeOnThresholdFailure);
	void HandleCommandLineAutoCapture();
	void HandleTimedCaptureStop();

	FString BuildCaptureDirectory() const;
	bool SaveCaptureSessionToCsv(const FString& FilePath) const;
	bool SaveCaptureSessionToJson(const FString& FilePath) const;
	bool SaveCaptureSummaryToCsv(const FString& FilePath, FPerformanceCaptureReport& OutReport) const;
	bool SaveCaptureSummaryToJson(const FString& FilePath, const FPerformanceCaptureReport& Report) const;
	bool SaveCaptureSummaryToPng(const FString& FilePath, const FPerformanceCaptureReport& Report) const;
	bool SaveCaptureEventsToCsv(const FString& FilePath) const;
	bool SaveCaptureEventsToJson(const FString& FilePath) const;

	FStatUnitData CurrentStats;
	TArray<FRuntimePerformanceCaptureSample> CaptureSessionSamples;
	TArray<FPerformanceCaptureEvent> RecordedEvents;
	TArray<FPerformanceCaptureEvent> CaptureSessionEvents;
	FDateTime CaptureSessionStartUtc;
	FPerformanceCaptureReport LastCaptureReport;
	FPerformanceCaptureThresholdSettings ActiveThresholds;
	float ActiveCaptureDurationSeconds = 0.0f;
	bool bStopAfterDuration = false;
	bool bSkipSummaryPngExport = false;
	bool bRequestExitOnCaptureComplete = false;
	bool bUseFailExitCodeOnThresholdFailure = false;
	bool bCommandLineAutoCaptureHandled = false;
	bool bCaptureSessionActive = false;
	bool bStatUnitEnabled = false;
};
