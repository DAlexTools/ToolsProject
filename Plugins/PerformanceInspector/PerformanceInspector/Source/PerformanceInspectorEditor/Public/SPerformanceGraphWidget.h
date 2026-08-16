#pragma once

#include "CoreMinimal.h"
#include "PerformanceCaptureTypes.h"
#include "UnrealClient.h"
#include "Widgets/SCompoundWidget.h"

class SComboButton;
enum class ECheckBoxState : uint8;

struct FPerformanceSeriesStats
{
	bool bHasSamples = false;
	float CurrentMs = 0.0f;
	float AverageMs = 0.0f;
	float MaxMs = 0.0f;
};

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
static_assert(sizeof(FPerformanceSeriesStats) == 16, "Stats layout changed");
static_assert(alignof(FPerformanceSeriesStats) == 4, "Stats alignment changed");
#endif

struct FPerformanceGraphSample
{
	float TimeMs = 0.0f;

	explicit FPerformanceGraphSample(float InTimeMs)
		: TimeMs(InTimeMs)
	{
	}
};

struct FPerformanceGraphSeries
{
	FName Name;
	FLinearColor Color;
	TArray<FPerformanceGraphSample> Samples;

	FPerformanceGraphSeries(FName InName, const FLinearColor& InColor)
		: Name(InName)
		, Color(InColor)
	{
	}
};

struct FPerformanceCaptureSample 
{
	double ElapsedSeconds = 0.0;
	FDateTime TimestampUtc;
	TArray<float, TFixedAllocator<MAX_NUM_GPUS>> GPUFrameTimesMs;

	float FrameTimeMs = 0.0f;
	float GameThreadTimeMs = 0.0f;
	float RenderThreadTimeMs = 0.0f;
	float InputLatencyTimeMs = 0.0f;
	float RHITTimeMs = 0.0f;

	FPerformanceCaptureSample()
	{
		GPUFrameTimesMs.Init(0.0f, MAX_NUM_GPUS);
	}
};


class PERFORMANCEINSPECTOREDITOR_API SPerformanceGraphWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPerformanceGraphWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetPaused(bool bInPaused);
	void ClearSamples();
	void SetMaxSamples(int32 InMaxSamples);
	int32 GetMaxSamples() const;
	void SetAutoScaleEnabled(bool bEnabled);
	bool IsAutoScaleEnabled() const;
	FText GetDataSourceText() const;
	bool LoadCaptureSessionFromJson(const FString& FilePath);
	void ExitSavedSessionMode();
	bool IsSavedSessionModeActive() const;
	void ZoomSavedSession(float ZoomFactor);
	void PanSavedSession(int32 SampleDelta);
	void ResetSavedSessionView();
	FString GetSavedSessionSourcePath() const;
	FString GetCaptureDirectoryPath() const;
	FString GetLastCaptureJsonPath() const;
	void FocusAllMetrics();
	void FocusFrameMetric();
	void FocusGameThreadMetric();
	void FocusRenderThreadMetric();
	void FocusRHIMetric();
	void FocusGPUMetrics();
	void ClearSelectedRange();
	bool HasSelectedRange() const;
	bool GetSeriesStats(FName SeriesName, FPerformanceSeriesStats& OutStats) const;
	void StartCaptureSession();
	bool StopCaptureSession(FString& OutCsvPath, FString& OutJsonPath);
	bool IsCaptureSessionActive() const;
	int32 GetCaptureSampleCount() const;
	double GetCaptureSessionDurationSeconds() const;
	FText GetLastCaptureSummaryText() const;
	FText GetCaptureSessionSummaryText() const;
	bool CanExportCurrentSummaryPng() const;
	bool ExportCurrentSummaryPng(const FString& FilePath) const;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

protected:
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	void AddSeries(FName SeriesName, const FLinearColor& Color);
	void AddSample(FName SeriesName, float TimeMs);
	void AppendCaptureSample();
	bool UpdatePerformanceData();
	void UpdateStatsFromData(const FStatUnitData& StatUnitData);
	void TrimSeriesToMaxSamples();
	void SyncRuntimeEvents();
	void AddEventToCapture(const FPerformanceCaptureEvent& Event);
	void ClampSavedSessionView();
	int32 GetSavedVisibleSampleCount() const;
	int32 GetSavedVisibleStartIndex() const;
	int32 GetSavedVisibleEndIndex() const;
	bool GetActiveSavedSampleRange(int32& OutStartIndex, int32& OutEndIndex) const;
	float GetSavedSeriesValue(const FPerformanceCaptureSample& Sample, FName SeriesName) const;
	bool IsPointInSavedOverview(const FVector2D& LocalPosition, const FVector2D& WidgetSize) const;
	bool IsPointInMainGraph(const FVector2D& LocalPosition, const FVector2D& WidgetSize) const;
	int32 GetSavedSampleIndexFromGraphPosition(const FVector2D& LocalPosition, const FVector2D& WidgetSize) const;
	void UpdateSelectedRangeFromGraphPositions(const FVector2D& StartLocalPosition, const FVector2D& EndLocalPosition, const FVector2D& WidgetSize);
	void CenterSavedSessionOnNormalizedPosition(float NormalizedPosition);
	FString BuildCaptureDirectory() const;
	bool BuildActiveSummaryExportData(TArray<FPerformanceCaptureSample>& OutSamples, TArray<FPerformanceCaptureEvent>& OutEvents, FDateTime& OutStartUtc, bool& bOutIsSelectedRange) const;
	bool SaveCaptureSessionToCsv(const FString& FilePath) const;
	bool SaveCaptureSessionToJson(const FString& FilePath) const;
	bool SaveCaptureSummaryToCsv(const FString& FilePath) const;
	bool SaveCaptureSummaryToJson(const FString& FilePath) const;
	bool SaveCaptureSummaryToPng(const FString& FilePath) const;
	bool SaveCaptureSummaryToPng(const FString& FilePath, const TArray<FPerformanceCaptureSample>& Samples, const TArray<FPerformanceCaptureEvent>& Events, const FDateTime& SessionStartUtc, bool bIsSelectedRange) const;
	bool SaveCaptureEventsToCsv(const FString& FilePath) const;
	bool SaveCaptureEventsToJson(const FString& FilePath) const;

	TSharedRef<SWidget> BuildSeriesVisibilityMenu();
	TSharedRef<SWidget> BuildThresholdMenu();
	FText GetVisibleSeriesSummary() const;
	FText GetThresholdSummary() const;
	void SetFocusedSeries(const TArray<FName>& VisibleSeriesNames);

	ECheckBoxState IsSeriesVisible(FName SeriesName) const;
	void OnToggleSeries(ECheckBoxState NewState, FName SeriesName);
	bool IsSeriesVisibleBool(FName SeriesName) const;

	void InitializeDefaultBudgets();
	void ApplyBudgetPreset(float InBudgetMs);
	void SetSeriesBudget(FName SeriesName, float InThresholdMs);
	float GetSeriesBudget(FName SeriesName) const;
	bool HasSeriesBudget(FName SeriesName) const;
	bool IsSeriesThresholdExceeded(FName SeriesName, float ValueMs) const;
	float GetHighestActiveBudget() const;
	float GetLowestActiveBudget() const;
	float GetDisplayMaxTime() const;

	void DrawBackground(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size) const;
	void DrawGrid(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float DisplayMaxTime) const;
	void DrawYAxisLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float DisplayMaxTime) const;
	void DrawLegend(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
	void DrawAlertZone(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float DisplayMaxTime) const;
	void DrawAlertLine(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float DisplayMaxTime) const;
	void DrawEventMarkers(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size) const;
	void DrawGraphLines(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float XStep, float DisplayMaxTime) const;
	void DrawSelectedRangeOverlay(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float XStep) const;
	void DrawHoverLine(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size) const;
	void DrawHoverTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float XStep, float DisplayMaxTime) const;
	void DrawTable(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, const FVector2D& Size, int32 LayerId) const;

	TMap<FName, FPerformanceGraphSeries> GraphSeries;
	TArray<FName> SeriesDrawOrder;
	TMap<FName, bool> SeriesVisibility;
	TMap<FName, float> SeriesBudgetsMs;

	int32 MaxSamples = 100;
	float SmoothedMaxObservedTime = 1.0f;
	FStatUnitData CurrentStats;
	FText DataSourceText;

	bool bStatUnitEnabled = false;
	bool bShowUnitMaxTimes = false;
	bool bShowFrameRateTime = true;
	bool bShowGameThreadTime = true;
	bool bShowRenderThreadTime = true;
	bool bShowRHITTime = true;

	float MaxFrameTime = 0.0f;
	float MaxGameThreadTime = 0.0f;
	float MaxRenderThreadTime = 0.0f;
	float MaxRHITTime = 0.0f;

	FVector2D HoverPosition = FVector2D::ZeroVector;
	bool bIsHovering = false;
	bool bIsPaused = false;
	bool bAutoScaleEnabled = true;
	bool bIsCaptureSessionActive = false;

	TArray<FPerformanceCaptureSample> CaptureSessionSamples;
	TArray<FPerformanceCaptureEvent> CaptureSessionEvents;
	FDateTime CaptureSessionStartUtc;
	FDateTime LastCaptureSavedUtc;
	FString LastCaptureCsvPath;
	FString LastCaptureJsonPath;
	int32 SyncedRuntimeEventCount = 0;
	FVector2D LastKnownGraphSize = FVector2D::ZeroVector;
	bool bIsSavedSessionMode = false;
	bool bIsPanningSavedSession = false;
	bool bIsDraggingSavedOverview = false;
	bool bIsSelectingSavedRange = false;
	bool bHasSelectedRange = false;
	FVector2D SavedSessionDragAnchor = FVector2D::ZeroVector;
	FVector2D SavedRangeSelectionAnchor = FVector2D::ZeroVector;
	FString SavedSessionSourcePath;
	TArray<FPerformanceCaptureSample> SavedSessionSamples;
	TArray<FPerformanceCaptureEvent> SavedSessionEvents;
	int32 SavedSessionViewStartIndex = 0;
	int32 SavedSessionViewSampleCount = 0;
	int32 SelectedRangeStartIndex = INDEX_NONE;
	int32 SelectedRangeEndIndex = INDEX_NONE;

	TSharedPtr<SComboButton> SeriesVisibilityComboButton;
	TSharedPtr<SComboButton> ThresholdComboButton;
};
