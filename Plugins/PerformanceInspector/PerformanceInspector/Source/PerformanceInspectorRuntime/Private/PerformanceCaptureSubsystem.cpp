#include "PerformanceCaptureSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PerformanceInspectorRuntime.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Rendering/DrawElements.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Settings/PerformanceInspectorAutomationSettings.h"
#include "Slate/WidgetRenderer.h"
#include "TextureResource.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
bool ShouldExportSummaryPng()
{
	const UPerformanceInspectorAutomationSettings* Settings = GetDefault<UPerformanceInspectorAutomationSettings>();
	return Settings && Settings->bEnableSummaryPngExport && FSlateApplication::IsInitialized();
}

FVector2f ToSlateVector2f(const FVector2D& Value)
{
	return FVector2f(static_cast<float>(Value.X), static_cast<float>(Value.Y));
}

FPaintGeometry MakePaintGeometry(const FGeometry& Geometry)
{
	return Geometry.ToPaintGeometry(FSlateLayoutTransform());
}

FPaintGeometry MakePaintGeometry(const FGeometry& Geometry, const FVector2D& LocalOffset, const FVector2D& LocalSize)
{
	return Geometry.ToPaintGeometry(ToSlateVector2f(LocalSize), FSlateLayoutTransform(ToSlateVector2f(LocalOffset)));
}

struct FMetricSummary
{
	float Min = 0.0f;
	float Average = 0.0f;
	float Max = 0.0f;
	float Last = 0.0f;
};

float FrameMsToFPS(float FrameMs);
FString GetDominantBottleneck(const TArray<FRuntimePerformanceCaptureSample>& Samples);

template <typename AccessorType>
FMetricSummary BuildMetricSummary(const TArray<FRuntimePerformanceCaptureSample>& Samples, AccessorType Accessor)
{
	FMetricSummary Summary;
	if (Samples.Num() == 0)
	{
		return Summary;
	}

	float Sum = 0.0f;
	Summary.Min = TNumericLimits<float>::Max();
	Summary.Max = TNumericLimits<float>::Lowest();

	for (const FRuntimePerformanceCaptureSample& Sample : Samples)
	{
		const float Value = Accessor(Sample);
		Sum += Value;
		Summary.Min = FMath::Min(Summary.Min, Value);
		Summary.Max = FMath::Max(Summary.Max, Value);
		Summary.Last = Value;
	}

	Summary.Average = Sum / Samples.Num();
	return Summary;
}

void PopulateReportMetrics(const TArray<FRuntimePerformanceCaptureSample>& Samples, FPerformanceCaptureReport& OutReport)
{
	if (Samples.Num() == 0)
	{
		return;
	}

	const FMetricSummary FrameSummary = BuildMetricSummary(Samples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(Samples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(Samples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary InputSummary = BuildMetricSummary(Samples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.InputLatencyTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(Samples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

	OutReport.SampleCount = Samples.Num();
	OutReport.DurationSeconds = static_cast<float>(Samples.Last().ElapsedSeconds);
	OutReport.AverageFPS = FrameMsToFPS(FrameSummary.Average);
	OutReport.MinFPS = FrameMsToFPS(FrameSummary.Max);
	OutReport.MaxFPS = FrameMsToFPS(FMath::Max(FrameSummary.Min, KINDA_SMALL_NUMBER));
	OutReport.DominantBottleneck = GetDominantBottleneck(Samples);
	OutReport.AverageFrameTimeMs = FrameSummary.Average;
	OutReport.MinFrameTimeMs = FrameSummary.Min;
	OutReport.MaxFrameTimeMs = FrameSummary.Max;
	OutReport.AverageGameThreadTimeMs = GameSummary.Average;
	OutReport.MaxGameThreadTimeMs = GameSummary.Max;
	OutReport.AverageRenderThreadTimeMs = RenderSummary.Average;
	OutReport.MaxRenderThreadTimeMs = RenderSummary.Max;
	OutReport.AverageInputLatencyTimeMs = InputSummary.Average;
	OutReport.MaxInputLatencyTimeMs = InputSummary.Max;
	OutReport.AverageRHITTimeMs = RHISummary.Average;
	OutReport.MaxRHITTimeMs = RHISummary.Max;
}

void AddThresholdFailure(FPerformanceCaptureReport& InOutReport, const FString& Message)
{
	InOutReport.ThresholdFailures.Add(Message);
	InOutReport.bPassedThresholds = false;
}

void WriteThresholdSummary(TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer, const FPerformanceCaptureReport& Report)
{
	Writer->WriteObjectStart(TEXT("thresholds"));
	Writer->WriteValue(TEXT("configured"), Report.bThresholdsConfigured);
	Writer->WriteValue(TEXT("passed"), Report.bPassedThresholds);
	Writer->WriteObjectStart(TEXT("applied"));
	Writer->WriteValue(TEXT("minAverageFps"), Report.AppliedThresholds.MinAverageFPS);
	Writer->WriteValue(TEXT("maxAverageFrameTimeMs"), Report.AppliedThresholds.MaxAverageFrameTimeMs);
	Writer->WriteValue(TEXT("maxAverageGameThreadTimeMs"), Report.AppliedThresholds.MaxAverageGameThreadTimeMs);
	Writer->WriteValue(TEXT("maxAverageRenderThreadTimeMs"), Report.AppliedThresholds.MaxAverageRenderThreadTimeMs);
	Writer->WriteValue(TEXT("maxAverageInputLatencyTimeMs"), Report.AppliedThresholds.MaxAverageInputLatencyTimeMs);
	Writer->WriteValue(TEXT("maxAverageRhiTimeMs"), Report.AppliedThresholds.MaxAverageRHITTimeMs);
	Writer->WriteValue(TEXT("maxPeakFrameTimeMs"), Report.AppliedThresholds.MaxPeakFrameTimeMs);
	Writer->WriteValue(TEXT("maxPeakGameThreadTimeMs"), Report.AppliedThresholds.MaxPeakGameThreadTimeMs);
	Writer->WriteValue(TEXT("maxPeakRenderThreadTimeMs"), Report.AppliedThresholds.MaxPeakRenderThreadTimeMs);
	Writer->WriteValue(TEXT("maxPeakInputLatencyTimeMs"), Report.AppliedThresholds.MaxPeakInputLatencyTimeMs);
	Writer->WriteValue(TEXT("maxPeakRhiTimeMs"), Report.AppliedThresholds.MaxPeakRHITTimeMs);
	Writer->WriteObjectEnd();
	Writer->WriteArrayStart(TEXT("failures"));
	for (const FString& Failure : Report.ThresholdFailures)
	{
		Writer->WriteValue(Failure);
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
}

FString LexToString(EPerformanceCaptureEventSeverity Severity)
{
	switch (Severity)
	{
	case EPerformanceCaptureEventSeverity::Info:
		return TEXT("Info");
	case EPerformanceCaptureEventSeverity::Warning:
		return TEXT("Warning");
	case EPerformanceCaptureEventSeverity::Critical:
		return TEXT("Critical");
	default:
		return TEXT("Unknown");
	}
}

FLinearColor GetSeverityFallbackColor(EPerformanceCaptureEventSeverity Severity)
{
	switch (Severity)
	{
	case EPerformanceCaptureEventSeverity::Info:
		return FLinearColor(0.95f, 0.75f, 0.18f, 1.0f);
	case EPerformanceCaptureEventSeverity::Warning:
		return FLinearColor(1.0f, 0.45f, 0.16f, 1.0f);
	case EPerformanceCaptureEventSeverity::Critical:
		return FLinearColor(1.0f, 0.22f, 0.22f, 1.0f);
	default:
		return FLinearColor::White;
	}
}

struct FEventLabelCluster
{
	double StartSeconds = 0.0;
	double EndSeconds = 0.0;
	FString Category;
	FLinearColor Color = FLinearColor::White;
	int32 EventCount = 0;
};

TArray<FEventLabelCluster> BuildEventLabelClusters(const TArray<FPerformanceCaptureEvent>& Events, double TotalDurationSeconds, float GraphWidth)
{
	TArray<FEventLabelCluster> Clusters;
	if (Events.Num() == 0 || TotalDurationSeconds <= 0.0 || GraphWidth <= 0.0f)
	{
		return Clusters;
	}

	const float MinClusterSpacingPx = 84.0f;
	FEventLabelCluster CurrentCluster;

	for (const FPerformanceCaptureEvent& Event : Events)
	{
		const float EventX = GraphWidth * FMath::Clamp(static_cast<float>(Event.ElapsedSeconds / TotalDurationSeconds), 0.0f, 1.0f);
		if (CurrentCluster.EventCount == 0)
		{
			CurrentCluster.StartSeconds = Event.ElapsedSeconds;
			CurrentCluster.EndSeconds = Event.ElapsedSeconds;
			CurrentCluster.Category = Event.Category.IsEmpty() ? TEXT("Event") : Event.Category;
			CurrentCluster.Color = Event.Color.A > 0.0f ? Event.Color : GetSeverityFallbackColor(Event.Severity);
			CurrentCluster.EventCount = 1;
			continue;
		}

		const float ClusterEndX = GraphWidth * FMath::Clamp(static_cast<float>(CurrentCluster.EndSeconds / TotalDurationSeconds), 0.0f, 1.0f);
		if ((EventX - ClusterEndX) <= MinClusterSpacingPx && Event.Category == CurrentCluster.Category)
		{
			CurrentCluster.EndSeconds = Event.ElapsedSeconds;
			CurrentCluster.EventCount++;
			continue;
		}

		Clusters.Add(CurrentCluster);
		CurrentCluster = FEventLabelCluster();
		CurrentCluster.StartSeconds = Event.ElapsedSeconds;
		CurrentCluster.EndSeconds = Event.ElapsedSeconds;
		CurrentCluster.Category = Event.Category.IsEmpty() ? TEXT("Event") : Event.Category;
		CurrentCluster.Color = Event.Color.A > 0.0f ? Event.Color : GetSeverityFallbackColor(Event.Severity);
		CurrentCluster.EventCount = 1;
	}

	if (CurrentCluster.EventCount > 0)
	{
		Clusters.Add(CurrentCluster);
	}

	return Clusters;
}

TSharedRef<SWidget> BuildSessionEventsListWidget(const TArray<FPerformanceCaptureEvent>& Events)
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	const int32 MaxRows = 10;
	const int32 RowsToShow = FMath::Min(MaxRows, Events.Num());

	for (int32 EventIndex = 0; EventIndex < RowsToShow; ++EventIndex)
	{
		const FPerformanceCaptureEvent& Event = Events[EventIndex];
		Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%6.2fs | %s"), Event.ElapsedSeconds, *Event.Name.ToString())))
			.ColorAndOpacity(Event.Color.A > 0.0f ? Event.Color : GetSeverityFallbackColor(Event.Severity))
			.AutoWrapText(true)
		];
	}

	if (Events.Num() > RowsToShow)
	{
		Rows->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("+ %d more events"), Events.Num() - RowsToShow)))
			.ColorAndOpacity(FLinearColor(0.76f, 0.80f, 0.84f, 0.92f))
		];
	}

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.12f, 0.13f, 0.16f, 1.0f))
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Session Events")))
				.Font(FCoreStyle::Get().GetFontStyle("HeadingSmall"))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				Rows
			]
		];
}

float FrameMsToFPS(float FrameMs)
{
	return FrameMs > KINDA_SMALL_NUMBER ? 1000.0f / FrameMs : 0.0f;
}

FString GetSampleBottleneck(const FRuntimePerformanceCaptureSample& Sample)
{
	float MaxGPUTime = 0.0f;
	for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
	{
		MaxGPUTime = FMath::Max(MaxGPUTime, Sample.GPUFrameTimesMs[GPUIndex]);
	}

	float WinningValue = Sample.GameThreadTimeMs;
	FString WinningLabel = TEXT("CPU Game");

	if (Sample.RenderThreadTimeMs > WinningValue)
	{
		WinningValue = Sample.RenderThreadTimeMs;
		WinningLabel = TEXT("CPU Render");
	}

	if (Sample.RHITTimeMs > WinningValue)
	{
		WinningValue = Sample.RHITTimeMs;
		WinningLabel = TEXT("CPU RHI");
	}

	if (MaxGPUTime > WinningValue)
	{
		WinningValue = MaxGPUTime;
		WinningLabel = TEXT("GPU");
	}

	return WinningValue <= 0.0f ? TEXT("Unknown") : WinningLabel;
}

FString GetDominantBottleneck(const TArray<FRuntimePerformanceCaptureSample>& Samples)
{
	if (Samples.Num() == 0)
	{
		return TEXT("Unknown");
	}

	TMap<FString, int32> BottleneckCounts;
	for (const FRuntimePerformanceCaptureSample& Sample : Samples)
	{
		BottleneckCounts.FindOrAdd(GetSampleBottleneck(Sample))++;
	}

	FString DominantLabel = TEXT("Unknown");
	int32 DominantCount = 0;
	for (const TPair<FString, int32>& Pair : BottleneckCounts)
	{
		if (Pair.Value > DominantCount)
		{
			DominantLabel = Pair.Key;
			DominantCount = Pair.Value;
		}
	}

	return DominantLabel;
}

TSharedRef<SWidget> BuildLegendRow(const FLinearColor& Color, const FString& Text)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Color)
			.Padding(FMargin(7.0f))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ColorAndOpacity(FLinearColor::White)
		];
}

class SSessionReportGraphWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSessionReportGraphWidget) {}
		SLATE_ARGUMENT(const TArray<FRuntimePerformanceCaptureSample>*, Samples)
		SLATE_ARGUMENT(const TArray<FPerformanceCaptureEvent>*, Events)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Samples = InArgs._Samples;
		Events = InArgs._Events;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(1120.0f, 360.0f);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D Size = AllottedGeometry.GetLocalSize();
		const float LeftPad = 58.0f;
		const float RightPad = 20.0f;
		const float TopPad = 18.0f;
		const float BottomPad = 28.0f;
		const float GraphWidth = FMath::Max(1.0f, Size.X - LeftPad - RightPad);
		const float GraphHeight = FMath::Max(1.0f, Size.Y - TopPad - BottomPad);
		const float GraphBottom = TopPad + GraphHeight;
		const double TotalDurationSeconds = Samples && Samples->Num() > 0 ? Samples->Last().ElapsedSeconds : 0.0;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			MakePaintGeometry(AllottedGeometry),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.035f, 0.04f, 0.05f, 1.0f));

		const int32 HorizontalGridLines = 5;
		for (int32 GridIndex = 0; GridIndex <= HorizontalGridLines; ++GridIndex)
		{
			const float Y = TopPad + GraphHeight * GridIndex / HorizontalGridLines;
			const TArray<FVector2D> GridPoints = { FVector2D(LeftPad, Y), FVector2D(LeftPad + GraphWidth, Y) };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				MakePaintGeometry(AllottedGeometry),
				GridPoints,
				ESlateDrawEffect::None,
				FLinearColor(0.45f, 0.50f, 0.56f, 0.16f),
				true,
				1.0f);
		}

		const int32 VerticalGridLines = 8;
		for (int32 GridIndex = 0; GridIndex <= VerticalGridLines; ++GridIndex)
		{
			const float X = LeftPad + GraphWidth * GridIndex / VerticalGridLines;
			const TArray<FVector2D> GridPoints = { FVector2D(X, TopPad), FVector2D(X, GraphBottom) };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				MakePaintGeometry(AllottedGeometry),
				GridPoints,
				ESlateDrawEffect::None,
				FLinearColor(0.45f, 0.50f, 0.56f, 0.08f),
				true,
				1.0f);

			const double LabelSeconds = TotalDurationSeconds * static_cast<double>(GridIndex) / static_cast<double>(VerticalGridLines);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 2,
				MakePaintGeometry(AllottedGeometry, FVector2D(X - 20.0f, GraphBottom + 6.0f), FVector2D(48.0f, 16.0f)),
				FString::Printf(TEXT("%.1fs"), LabelSeconds),
				FCoreStyle::Get().GetFontStyle("SmallFont"),
				ESlateDrawEffect::None,
				FLinearColor(0.78f, 0.82f, 0.86f, 0.95f));
		}

		if (!Samples || Samples->Num() == 0)
		{
			return LayerId + 2;
		}

		float DisplayMax = 16.67f;
		for (const FRuntimePerformanceCaptureSample& Sample : *Samples)
		{
			DisplayMax = FMath::Max(DisplayMax, Sample.FrameTimeMs);
			DisplayMax = FMath::Max(DisplayMax, Sample.GameThreadTimeMs);
			DisplayMax = FMath::Max(DisplayMax, Sample.RenderThreadTimeMs);
			DisplayMax = FMath::Max(DisplayMax, Sample.RHITTimeMs);
		}
		DisplayMax *= 1.15f;

		for (int32 GridIndex = 0; GridIndex <= HorizontalGridLines; ++GridIndex)
		{
			const float Alpha = 1.0f - static_cast<float>(GridIndex) / HorizontalGridLines;
			const float ValueMs = DisplayMax * Alpha;
			const float Y = TopPad + GraphHeight * GridIndex / HorizontalGridLines - 8.0f;
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 2,
				MakePaintGeometry(AllottedGeometry, FVector2D(8.0f, Y), FVector2D(44.0f, 16.0f)),
				FString::Printf(TEXT("%.0f"), ValueMs),
				FCoreStyle::Get().GetFontStyle("SmallFont"),
				ESlateDrawEffect::None,
				FLinearColor(0.78f, 0.82f, 0.86f, 0.95f));
		}

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 2,
			MakePaintGeometry(AllottedGeometry, FVector2D(LeftPad + GraphWidth * 0.5f - 40.0f, GraphBottom + 22.0f), FVector2D(80.0f, 16.0f)),
			TEXT("Session Time"),
			FCoreStyle::Get().GetFontStyle("SmallFont"),
			ESlateDrawEffect::None,
			FLinearColor(0.90f, 0.92f, 0.95f, 1.0f));

		auto DrawSeries = [&](auto Accessor, const FLinearColor& Color)
		{
			if (Samples->Num() < 2)
			{
				return;
			}

			TArray<FVector2D> Points;
			Points.Reserve(Samples->Num());
			const float XStep = GraphWidth / FMath::Max(Samples->Num() - 1, 1);

			for (int32 SampleIndex = 0; SampleIndex < Samples->Num(); ++SampleIndex)
			{
				const float ValueMs = Accessor((*Samples)[SampleIndex]);
				const float X = LeftPad + SampleIndex * XStep;
				const float Y = GraphBottom - (FMath::Clamp(ValueMs, 0.0f, DisplayMax) / DisplayMax) * GraphHeight;
				Points.Add(FVector2D(X, Y));
			}

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 3,
				MakePaintGeometry(AllottedGeometry),
				Points,
				ESlateDrawEffect::PreMultipliedAlpha,
				Color,
				true,
				2.0f);
		};

		DrawSeries([](const FRuntimePerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; }, FLinearColor(0.10f, 1.00f, 0.10f));
		DrawSeries([](const FRuntimePerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; }, FLinearColor(1.00f, 0.20f, 0.20f));
		DrawSeries([](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; }, FLinearColor(0.25f, 0.55f, 1.00f));
		DrawSeries([](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; }, FLinearColor(1.00f, 0.45f, 1.00f));

		if (Events && Events->Num() > 0 && TotalDurationSeconds > 0.0)
		{
			for (const FPerformanceCaptureEvent& Event : *Events)
			{
				const float NormalizedTime = FMath::Clamp(static_cast<float>(Event.ElapsedSeconds / TotalDurationSeconds), 0.0f, 1.0f);
				const float X = LeftPad + GraphWidth * NormalizedTime;
				const TArray<FVector2D> MarkerPoints = { FVector2D(X, TopPad), FVector2D(X, GraphBottom) };
				const FLinearColor MarkerColor = Event.Color.A > 0.0f ? Event.Color : GetSeverityFallbackColor(Event.Severity);

				FSlateDrawElement::MakeLines(
					OutDrawElements,
					LayerId + 4,
					MakePaintGeometry(AllottedGeometry),
					MarkerPoints,
					ESlateDrawEffect::None,
					MarkerColor.CopyWithNewOpacity(0.85f),
					true,
					1.2f);
			}

			const TArray<FEventLabelCluster> Clusters = BuildEventLabelClusters(*Events, TotalDurationSeconds, GraphWidth);
			const int32 MaxClusterLabels = FMath::Min(6, Clusters.Num());
			for (int32 ClusterIndex = 0; ClusterIndex < MaxClusterLabels; ++ClusterIndex)
			{
				const FEventLabelCluster& Cluster = Clusters[ClusterIndex];
				const double ClusterMidpointSeconds = (Cluster.StartSeconds + Cluster.EndSeconds) * 0.5;
				const float ClusterX = LeftPad + GraphWidth * FMath::Clamp(static_cast<float>(ClusterMidpointSeconds / TotalDurationSeconds), 0.0f, 1.0f);
				const FString Label = Cluster.EventCount > 1
					? FString::Printf(TEXT("%s +%d"), *Cluster.Category, Cluster.EventCount - 1)
					: Cluster.Category;

				FSlateDrawElement::MakeText(
					OutDrawElements,
					LayerId + 5,
					MakePaintGeometry(AllottedGeometry, FVector2D(FMath::Min(ClusterX + 4.0f, LeftPad + GraphWidth - 118.0f), TopPad + 4.0f), FVector2D(116.0f, 16.0f)),
					Label,
					FCoreStyle::Get().GetFontStyle("SmallFont"),
					ESlateDrawEffect::None,
					Cluster.Color);
			}
		}

		return LayerId + 5;
	}

private:
	const TArray<FRuntimePerformanceCaptureSample>* Samples = nullptr;
	const TArray<FPerformanceCaptureEvent>* Events = nullptr;
};
}

void UPerformanceCaptureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	HandleCommandLineAutoCapture();
}

void UPerformanceCaptureSubsystem::Deinitialize()
{
	bCaptureSessionActive = false;
	CaptureSessionSamples.Reset();
	CaptureSessionEvents.Reset();
	RecordedEvents.Reset();
	LastCaptureReport = FPerformanceCaptureReport();
	Super::Deinitialize();
}

void UPerformanceCaptureSubsystem::Tick(float DeltaTime)
{
	if (!bCommandLineAutoCaptureHandled)
	{
		HandleCommandLineAutoCapture();
	}

	if (!bCaptureSessionActive)
	{
		return;
	}

	if (UpdatePerformanceData())
	{
		AppendCaptureSample();
	}

	if (bStopAfterDuration && ActiveCaptureDurationSeconds > 0.0f && GetCaptureSessionDurationSeconds() >= ActiveCaptureDurationSeconds)
	{
		HandleTimedCaptureStop();
	}
}

TStatId UPerformanceCaptureSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPerformanceCaptureSubsystem, STATGROUP_Tickables);
}

bool UPerformanceCaptureSubsystem::IsTickable() const
{
	return !IsTemplate();
}

bool UPerformanceCaptureSubsystem::IsTickableInEditor() const
{
	return false;
}

void UPerformanceCaptureSubsystem::StartCaptureSession()
{
	BeginCaptureSession(FPerformanceCaptureThresholdSettings(), 0.0f, false, false);
}

void UPerformanceCaptureSubsystem::StartTimedCaptureSession(float DurationSeconds, const FPerformanceCaptureThresholdSettings& Thresholds,
	bool bExitProcessOnCompletion, bool bFailExitCodeOnThresholdFailure)
{
	BeginCaptureSession(Thresholds, DurationSeconds, bExitProcessOnCompletion, bFailExitCodeOnThresholdFailure);
}

FPerformanceCaptureReport UPerformanceCaptureSubsystem::StopCaptureSession()
{
	auto ResetCaptureCompletionState = [this]()
	{
		ActiveThresholds = FPerformanceCaptureThresholdSettings();
		ActiveCaptureDurationSeconds = 0.0f;
		bSkipSummaryPngExport = false;
		bRequestExitOnCaptureComplete = false;
		bUseFailExitCodeOnThresholdFailure = false;
	};

	FPerformanceCaptureReport Report;
	Report.bWasRecording = bCaptureSessionActive;

	if (!bCaptureSessionActive)
	{
		return Report;
	}

	bCaptureSessionActive = false;
	bStopAfterDuration = false;

	if (CaptureSessionSamples.Num() == 0)
	{
		UE_LOG(LogPerformanceInspectorRuntime, Warning,
			TEXT("Performance capture stopped without samples. Duration=%.2fs, SkipSummaryPng=%s"),
			GetCaptureSessionDurationSeconds(),
			bSkipSummaryPngExport ? TEXT("true") : TEXT("false"));
		LastCaptureReport = Report;
		ResetCaptureCompletionState();
		return Report;
	}

	BuildReportFromSamples(CaptureSessionSamples, CaptureSessionEvents, CaptureSessionStartUtc, ActiveThresholds, Report);

	const FString CaptureDirectory = BuildCaptureDirectory();
	IFileManager::Get().MakeDirectory(*CaptureDirectory, true);

	const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString BasePath = FPaths::Combine(CaptureDirectory, FString::Printf(TEXT("runtime_performance_capture_%s"), *Timestamp));
	Report.CsvPath = BasePath + TEXT(".csv");
	Report.JsonPath = BasePath + TEXT(".json");
	Report.SummaryCsvPath = BasePath + TEXT("_summary.csv");
	Report.SummaryJsonPath = BasePath + TEXT("_summary.json");
	Report.SummaryPngPath = BasePath + TEXT("_summary.png");
	Report.EventsCsvPath = BasePath + TEXT("_events.csv");
	Report.EventsJsonPath = BasePath + TEXT("_events.json");

	const bool bCsvSaved = SaveCaptureSessionToCsv(Report.CsvPath);
	const bool bJsonSaved = SaveCaptureSessionToJson(Report.JsonPath);
	const bool bSummaryCsvSaved = SaveCaptureSummaryToCsv(Report.SummaryCsvPath, Report);
	const bool bSummaryJsonSaved = SaveCaptureSummaryToJson(Report.SummaryJsonPath, Report);
	bool bSummaryPngSaved = false;
	if (bSkipSummaryPngExport || !ShouldExportSummaryPng())
	{
		UE_LOG(LogPerformanceInspectorRuntime, Log,
			TEXT("Skipping summary PNG export. BasePath=%s, TimedStop=%s, PngExportEnabled=%s, SlateReady=%s"),
			*BasePath,
			bSkipSummaryPngExport ? TEXT("true") : TEXT("false"),
			GetDefault<UPerformanceInspectorAutomationSettings>()->bEnableSummaryPngExport ? TEXT("true") : TEXT("false"),
			FSlateApplication::IsInitialized() ? TEXT("true") : TEXT("false"));
		Report.SummaryPngPath.Reset();
	}
	else
	{
		bSummaryPngSaved = SaveCaptureSummaryToPng(Report.SummaryPngPath, Report);
	}
	const bool bEventsCsvSaved = SaveCaptureEventsToCsv(Report.EventsCsvPath);
	const bool bEventsJsonSaved = SaveCaptureEventsToJson(Report.EventsJsonPath);

	Report.bSuccess = bCsvSaved || bJsonSaved || bSummaryCsvSaved || bSummaryJsonSaved || bSummaryPngSaved || bEventsCsvSaved || bEventsJsonSaved;
	LastCaptureReport = Report;
	UE_LOG(LogPerformanceInspectorRuntime, Log,
		TEXT("Performance capture stopped. Samples=%d, Duration=%.2fs, Success=%s, ThresholdsConfigured=%s, PassedThresholds=%s, SummaryPngSaved=%s"),
		CaptureSessionSamples.Num(),
		GetCaptureSessionDurationSeconds(),
		Report.bSuccess ? TEXT("true") : TEXT("false"),
		Report.bThresholdsConfigured ? TEXT("true") : TEXT("false"),
		Report.bPassedThresholds ? TEXT("true") : TEXT("false"),
		bSummaryPngSaved ? TEXT("true") : TEXT("false"));
	ResetCaptureCompletionState();
	return Report;
}

bool UPerformanceCaptureSubsystem::IsCaptureSessionActive() const
{
	return bCaptureSessionActive;
}

int32 UPerformanceCaptureSubsystem::GetCaptureSampleCount() const
{
	return CaptureSessionSamples.Num();
}

float UPerformanceCaptureSubsystem::GetCaptureSessionDurationSeconds() const
{
	if (CaptureSessionSamples.Num() > 0)
	{
		return static_cast<float>(CaptureSessionSamples.Last().ElapsedSeconds);
	}

	if (bCaptureSessionActive && CaptureSessionStartUtc != FDateTime())
	{
		return static_cast<float>((FDateTime::UtcNow() - CaptureSessionStartUtc).GetTotalSeconds());
	}

	return 0.0f;
}

const FPerformanceCaptureReport& UPerformanceCaptureSubsystem::GetLastCaptureReport() const
{
	return LastCaptureReport;
}

void UPerformanceCaptureSubsystem::AddEventMarker(FName Name, const FString& Category, const FString& Details,
	EPerformanceCaptureEventSeverity Severity, FLinearColor Color)
{
	if (Name.IsNone())
	{
		return;
	}

	FPerformanceCaptureEvent Event;
	Event.TimestampUtc = FDateTime::UtcNow();
	Event.ElapsedSeconds = CaptureSessionStartUtc == FDateTime() ? 0.0 : (Event.TimestampUtc - CaptureSessionStartUtc).GetTotalSeconds();
	Event.Name = Name;
	Event.Category = Category;
	Event.Details = Details;
	Event.Severity = Severity;
	Event.Color = Color.A > 0.0f ? Color : GetSeverityFallbackColor(Severity);

	RecordedEvents.Add(Event);

	if (bCaptureSessionActive)
	{
		CaptureSessionEvents.Add(Event);
	}
}

void UPerformanceCaptureSubsystem::BeginCaptureSession(const FPerformanceCaptureThresholdSettings& Thresholds, float DurationSeconds,
	bool bExitProcessOnCompletion, bool bFailExitCodeOnThresholdFailure)
{
	CaptureSessionSamples.Reset();
	CaptureSessionEvents.Reset();
	CaptureSessionStartUtc = FDateTime::UtcNow();
	ActiveThresholds = Thresholds;
	ActiveCaptureDurationSeconds = FMath::Max(0.0f, DurationSeconds);
	bStopAfterDuration = ActiveCaptureDurationSeconds > 0.0f;
	bSkipSummaryPngExport = bStopAfterDuration;
	bRequestExitOnCaptureComplete = bExitProcessOnCompletion;
	bUseFailExitCodeOnThresholdFailure = bFailExitCodeOnThresholdFailure;
	bCaptureSessionActive = true;
	LastCaptureReport = FPerformanceCaptureReport();
	UE_LOG(LogPerformanceInspectorRuntime, Log,
		TEXT("Performance capture started. Timed=%s, Duration=%.2fs, ExitOnCompletion=%s, FailExitCodeOnThresholdFailure=%s, SkipSummaryPng=%s"),
		bStopAfterDuration ? TEXT("true") : TEXT("false"),
		ActiveCaptureDurationSeconds,
		bRequestExitOnCaptureComplete ? TEXT("true") : TEXT("false"),
		bUseFailExitCodeOnThresholdFailure ? TEXT("true") : TEXT("false"),
		bSkipSummaryPngExport ? TEXT("true") : TEXT("false"));
}

void UPerformanceCaptureSubsystem::HandleCommandLineAutoCapture()
{
	const UPerformanceInspectorAutomationSettings* Settings = GetDefault<UPerformanceInspectorAutomationSettings>();
	if (!Settings || !Settings->bEnableCommandLineAutoCapture || bCaptureSessionActive)
	{
		bCommandLineAutoCaptureHandled = true;
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		bCommandLineAutoCaptureHandled = true;
		return;
	}

	const bool bAutoCaptureRequested = FParse::Param(FCommandLine::Get(), TEXT("PerformanceInspectorAutoCapture"));
	if (!bAutoCaptureRequested)
	{
		bCommandLineAutoCaptureHandled = true;
		return;
	}

	float DurationSeconds = Settings->DefaultCommandLineCaptureDurationSeconds;
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorCaptureDuration="), DurationSeconds);

	FPerformanceCaptureThresholdSettings Thresholds = Settings->DefaultCommandLineThresholds;
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMinAverageFPS="), Thresholds.MinAverageFPS);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxAverageFrameTimeMs="), Thresholds.MaxAverageFrameTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxAverageGameThreadTimeMs="), Thresholds.MaxAverageGameThreadTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxAverageRenderThreadTimeMs="), Thresholds.MaxAverageRenderThreadTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxAverageInputLatencyTimeMs="), Thresholds.MaxAverageInputLatencyTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxAverageRHITTimeMs="), Thresholds.MaxAverageRHITTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxPeakFrameTimeMs="), Thresholds.MaxPeakFrameTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxPeakGameThreadTimeMs="), Thresholds.MaxPeakGameThreadTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxPeakRenderThreadTimeMs="), Thresholds.MaxPeakRenderThreadTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxPeakInputLatencyTimeMs="), Thresholds.MaxPeakInputLatencyTimeMs);
	FParse::Value(FCommandLine::Get(), TEXT("PerformanceInspectorMaxPeakRHITTimeMs="), Thresholds.MaxPeakRHITTimeMs);

	const bool bExitOnCompletion = FParse::Param(FCommandLine::Get(), TEXT("PerformanceInspectorExitOnComplete"))
		|| Settings->bExitProcessOnCommandLineCaptureComplete;
	const bool bFailExitCode = FParse::Param(FCommandLine::Get(), TEXT("PerformanceInspectorFailOnThresholdFailure"))
		|| Settings->bUseFailExitCodeOnThresholdFailure;

	UE_LOG(LogPerformanceInspectorRuntime, Log,
		TEXT("Starting command line auto capture. Duration=%.2fs, ExitOnCompletion=%s, FailExitCodeOnThresholdFailure=%s"),
		DurationSeconds,
		bExitOnCompletion ? TEXT("true") : TEXT("false"),
		bFailExitCode ? TEXT("true") : TEXT("false"));
	BeginCaptureSession(Thresholds, DurationSeconds, bExitOnCompletion, bFailExitCode);
	AddEventMarker(TEXT("AutoCaptureStart"), TEXT("Automation"), TEXT("Command line auto capture started"));
	bCommandLineAutoCaptureHandled = true;
}

void UPerformanceCaptureSubsystem::HandleTimedCaptureStop()
{
	const bool bShouldRequestExitOnCompletion = bRequestExitOnCaptureComplete;
	const bool bShouldUseFailExitCodeOnThresholdFailure = bUseFailExitCodeOnThresholdFailure;

	UE_LOG(LogPerformanceInspectorRuntime, Log,
		TEXT("Timed performance capture reached duration threshold at %.2fs. RequestExitOnComplete=%s"),
		GetCaptureSessionDurationSeconds(),
		bRequestExitOnCaptureComplete ? TEXT("true") : TEXT("false"));
	AddEventMarker(TEXT("AutoCaptureStop"), TEXT("Automation"), TEXT("Capture stopped after reaching configured duration"));

	const FPerformanceCaptureReport Report = StopCaptureSession();
	if (bShouldRequestExitOnCompletion)
	{
		const bool bUseFailureCode = bShouldUseFailExitCodeOnThresholdFailure && Report.bThresholdsConfigured && !Report.bPassedThresholds;
		FPlatformMisc::RequestExitWithStatus(false, bUseFailureCode ? 1 : 0, TEXT("PerformanceInspector auto capture completed"));
	}
}

bool UPerformanceCaptureSubsystem::BuildReportFromSamples(const TArray<FRuntimePerformanceCaptureSample>& Samples,
	const TArray<FPerformanceCaptureEvent>& Events, const FDateTime& InCaptureSessionStartUtc, const FPerformanceCaptureThresholdSettings& Thresholds,
	FPerformanceCaptureReport& OutReport)
{
	(void)Events;
	(void)InCaptureSessionStartUtc;
	OutReport = FPerformanceCaptureReport();
	if (Samples.Num() == 0)
	{
		return false;
	}

	PopulateReportMetrics(Samples, OutReport);
	OutReport.bThresholdsConfigured = Thresholds.HasAnyThresholds();
	OutReport.bPassedThresholds = true;
	OutReport.AppliedThresholds = Thresholds;
	EvaluateReportAgainstThresholds(OutReport, Thresholds);
	return true;
}

void UPerformanceCaptureSubsystem::EvaluateReportAgainstThresholds(FPerformanceCaptureReport& InOutReport,
	const FPerformanceCaptureThresholdSettings& Thresholds)
{
	InOutReport.bThresholdsConfigured = Thresholds.HasAnyThresholds();
	InOutReport.bPassedThresholds = true;
	InOutReport.AppliedThresholds = Thresholds;
	InOutReport.ThresholdFailures.Reset();

	if (!InOutReport.bThresholdsConfigured)
	{
		return;
	}

	if (Thresholds.MinAverageFPS > 0.0f && InOutReport.AverageFPS < Thresholds.MinAverageFPS)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Average FPS %.2f is below threshold %.2f"), InOutReport.AverageFPS, Thresholds.MinAverageFPS));
	}

	if (Thresholds.MaxAverageFrameTimeMs > 0.0f && InOutReport.AverageFrameTimeMs > Thresholds.MaxAverageFrameTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Average frame time %.2f ms exceeds threshold %.2f ms"), InOutReport.AverageFrameTimeMs, Thresholds.MaxAverageFrameTimeMs));
	}

	if (Thresholds.MaxAverageGameThreadTimeMs > 0.0f && InOutReport.AverageGameThreadTimeMs > Thresholds.MaxAverageGameThreadTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Average game thread time %.2f ms exceeds threshold %.2f ms"), InOutReport.AverageGameThreadTimeMs, Thresholds.MaxAverageGameThreadTimeMs));
	}

	if (Thresholds.MaxAverageRenderThreadTimeMs > 0.0f && InOutReport.AverageRenderThreadTimeMs > Thresholds.MaxAverageRenderThreadTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Average render thread time %.2f ms exceeds threshold %.2f ms"), InOutReport.AverageRenderThreadTimeMs, Thresholds.MaxAverageRenderThreadTimeMs));
	}

	if (Thresholds.MaxAverageInputLatencyTimeMs > 0.0f && InOutReport.AverageInputLatencyTimeMs > Thresholds.MaxAverageInputLatencyTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Average input latency %.2f ms exceeds threshold %.2f ms"), InOutReport.AverageInputLatencyTimeMs, Thresholds.MaxAverageInputLatencyTimeMs));
	}

	if (Thresholds.MaxAverageRHITTimeMs > 0.0f && InOutReport.AverageRHITTimeMs > Thresholds.MaxAverageRHITTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Average RHI time %.2f ms exceeds threshold %.2f ms"), InOutReport.AverageRHITTimeMs, Thresholds.MaxAverageRHITTimeMs));
	}

	if (Thresholds.MaxPeakFrameTimeMs > 0.0f && InOutReport.MaxFrameTimeMs > Thresholds.MaxPeakFrameTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Peak frame time %.2f ms exceeds threshold %.2f ms"), InOutReport.MaxFrameTimeMs, Thresholds.MaxPeakFrameTimeMs));
	}

	if (Thresholds.MaxPeakGameThreadTimeMs > 0.0f && InOutReport.MaxGameThreadTimeMs > Thresholds.MaxPeakGameThreadTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Peak game thread time %.2f ms exceeds threshold %.2f ms"), InOutReport.MaxGameThreadTimeMs, Thresholds.MaxPeakGameThreadTimeMs));
	}

	if (Thresholds.MaxPeakRenderThreadTimeMs > 0.0f && InOutReport.MaxRenderThreadTimeMs > Thresholds.MaxPeakRenderThreadTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Peak render thread time %.2f ms exceeds threshold %.2f ms"), InOutReport.MaxRenderThreadTimeMs, Thresholds.MaxPeakRenderThreadTimeMs));
	}

	if (Thresholds.MaxPeakInputLatencyTimeMs > 0.0f && InOutReport.MaxInputLatencyTimeMs > Thresholds.MaxPeakInputLatencyTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Peak input latency %.2f ms exceeds threshold %.2f ms"), InOutReport.MaxInputLatencyTimeMs, Thresholds.MaxPeakInputLatencyTimeMs));
	}

	if (Thresholds.MaxPeakRHITTimeMs > 0.0f && InOutReport.MaxRHITTimeMs > Thresholds.MaxPeakRHITTimeMs)
	{
		AddThresholdFailure(InOutReport, FString::Printf(TEXT("Peak RHI time %.2f ms exceeds threshold %.2f ms"), InOutReport.MaxRHITTimeMs, Thresholds.MaxPeakRHITTimeMs));
	}
}

const TArray<FPerformanceCaptureEvent>& UPerformanceCaptureSubsystem::GetRecordedEvents() const
{
	return RecordedEvents;
}

const TArray<FPerformanceCaptureEvent>& UPerformanceCaptureSubsystem::GetCaptureSessionEvents() const
{
	return CaptureSessionEvents;
}

bool UPerformanceCaptureSubsystem::UpdatePerformanceData()
{
	if (!GEngine)
	{
		return false;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWorld* World = GameInstance->GetWorld())
		{
			if (!bStatUnitEnabled && World->GetGameViewport())
			{
				GEngine->SetEngineStat(World, World->GetGameViewport(), TEXT("Unit"), true);
				bStatUnitEnabled = true;
			}

			if (UGameViewportClient* ViewportClient = World->GetGameViewport())
			{
				if (ViewportClient->Viewport)
				{
					if (const FStatUnitData* StatUnitData = ViewportClient->GetStatUnitData())
					{
						UpdateStatsFromData(*StatUnitData);
						return true;
					}
				}
			}
		}
	}

	return false;
}

void UPerformanceCaptureSubsystem::UpdateStatsFromData(const FStatUnitData& StatUnitData)
{
	CurrentStats.FrameTime = StatUnitData.FrameTime;
	CurrentStats.GameThreadTime = StatUnitData.GameThreadTime;
	CurrentStats.RenderThreadTime = StatUnitData.RenderThreadTime;
	CurrentStats.InputLatencyTime = StatUnitData.InputLatencyTime;
	CurrentStats.RHITTime = StatUnitData.RHITTime;

	for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
	{
		CurrentStats.GPUFrameTime[GPUIndex] = StatUnitData.GPUFrameTime[GPUIndex];
	}
}

void UPerformanceCaptureSubsystem::AppendCaptureSample()
{
	FRuntimePerformanceCaptureSample Sample;
	Sample.TimestampUtc = FDateTime::UtcNow();
	Sample.ElapsedSeconds = CaptureSessionStartUtc == FDateTime()
		? 0.0
		: (Sample.TimestampUtc - CaptureSessionStartUtc).GetTotalSeconds();
	Sample.FrameTimeMs = CurrentStats.FrameTime;
	Sample.GameThreadTimeMs = CurrentStats.GameThreadTime;
	Sample.RenderThreadTimeMs = CurrentStats.RenderThreadTime;
	Sample.InputLatencyTimeMs = CurrentStats.InputLatencyTime;
	Sample.RHITTimeMs = CurrentStats.RHITTime;

	for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
	{
		Sample.GPUFrameTimesMs[GPUIndex] = CurrentStats.GPUFrameTime[GPUIndex];
	}

	CaptureSessionSamples.Add(MoveTemp(Sample));
}

FString UPerformanceCaptureSubsystem::BuildCaptureDirectory() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PerformanceCaptures"));
}

bool UPerformanceCaptureSubsystem::SaveCaptureSessionToCsv(const FString& FilePath) const
{
	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	FString CsvContent = TEXT("timestamp_utc,elapsed_seconds,frame_ms,estimated_fps,game_thread_ms,render_thread_ms,input_latency_ms,rhi_thread_ms,bottleneck");
	for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
	{
		CsvContent += FString::Printf(TEXT(",gpu_%d_frame_ms"), GPUIndex);
	}
	CsvContent += LINE_TERMINATOR;

	for (const FRuntimePerformanceCaptureSample& Sample : CaptureSessionSamples)
	{
		CsvContent += FString::Printf(
			TEXT("%s,%.6f,%.4f,%.2f,%.4f,%.4f,%.4f,%.4f,%s"),
			*Sample.TimestampUtc.ToIso8601(),
			Sample.ElapsedSeconds,
			Sample.FrameTimeMs,
			FrameMsToFPS(Sample.FrameTimeMs),
			Sample.GameThreadTimeMs,
			Sample.RenderThreadTimeMs,
			Sample.InputLatencyTimeMs,
			Sample.RHITTimeMs,
			*GetSampleBottleneck(Sample));

		for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
		{
			CsvContent += FString::Printf(TEXT(",%.4f"), Sample.GPUFrameTimesMs[GPUIndex]);
		}

		CsvContent += LINE_TERMINATOR;
	}

	return FFileHelper::SaveStringToFile(CsvContent, *FilePath);
}

bool UPerformanceCaptureSubsystem::SaveCaptureSessionToJson(const FString& FilePath) const
{
	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	FPerformanceCaptureReport Report;
	BuildReportFromSamples(CaptureSessionSamples, CaptureSessionEvents, CaptureSessionStartUtc, ActiveThresholds, Report);
	const FMetricSummary FrameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary InputSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.InputLatencyTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

	FString JsonContent;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonContent);

	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("captureStartedUtc"), CaptureSessionStartUtc.ToIso8601());
	Writer->WriteValue(TEXT("captureSavedUtc"), FDateTime::UtcNow().ToIso8601());
	Writer->WriteValue(TEXT("sampleCount"), CaptureSessionSamples.Num());
	Writer->WriteValue(TEXT("durationSeconds"), CaptureSessionSamples.Last().ElapsedSeconds);
	Writer->WriteObjectStart(TEXT("summary"));
	Writer->WriteValue(TEXT("dominantBottleneck"), GetDominantBottleneck(CaptureSessionSamples));
	Writer->WriteValue(TEXT("averageFps"), FrameMsToFPS(FrameSummary.Average));
	Writer->WriteValue(TEXT("minFps"), FrameMsToFPS(FrameSummary.Max));
	Writer->WriteValue(TEXT("maxFps"), FrameMsToFPS(FMath::Max(FrameSummary.Min, KINDA_SMALL_NUMBER)));

	auto WriteMetricSummary = [&Writer](const TCHAR* MetricName, const FMetricSummary& Summary)
	{
		Writer->WriteObjectStart(MetricName);
		Writer->WriteValue(TEXT("min"), Summary.Min);
		Writer->WriteValue(TEXT("average"), Summary.Average);
		Writer->WriteValue(TEXT("max"), Summary.Max);
		Writer->WriteValue(TEXT("last"), Summary.Last);
		Writer->WriteObjectEnd();
	};

	WriteMetricSummary(TEXT("frameMs"), FrameSummary);
	WriteMetricSummary(TEXT("gameThreadMs"), GameSummary);
	WriteMetricSummary(TEXT("renderThreadMs"), RenderSummary);
	WriteMetricSummary(TEXT("inputLatencyMs"), InputSummary);
	WriteMetricSummary(TEXT("rhiThreadMs"), RHISummary);
	WriteThresholdSummary(Writer, Report);
	Writer->WriteObjectEnd();

	Writer->WriteArrayStart(TEXT("samples"));
	for (const FRuntimePerformanceCaptureSample& Sample : CaptureSessionSamples)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("timestampUtc"), Sample.TimestampUtc.ToIso8601());
		Writer->WriteValue(TEXT("elapsedSeconds"), Sample.ElapsedSeconds);
		Writer->WriteValue(TEXT("frameTimeMs"), Sample.FrameTimeMs);
		Writer->WriteValue(TEXT("estimatedFps"), FrameMsToFPS(Sample.FrameTimeMs));
		Writer->WriteValue(TEXT("gameThreadTimeMs"), Sample.GameThreadTimeMs);
		Writer->WriteValue(TEXT("renderThreadTimeMs"), Sample.RenderThreadTimeMs);
		Writer->WriteValue(TEXT("inputLatencyTimeMs"), Sample.InputLatencyTimeMs);
		Writer->WriteValue(TEXT("rhiThreadTimeMs"), Sample.RHITTimeMs);
		Writer->WriteValue(TEXT("bottleneck"), GetSampleBottleneck(Sample));
		Writer->WriteArrayStart(TEXT("gpuFrameTimesMs"));
		for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
		{
			Writer->WriteValue(Sample.GPUFrameTimesMs[GPUIndex]);
		}
		Writer->WriteArrayEnd();
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();
	Writer->WriteArrayStart(TEXT("events"));
	for (const FPerformanceCaptureEvent& Event : CaptureSessionEvents)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("timestampUtc"), Event.TimestampUtc.ToIso8601());
		Writer->WriteValue(TEXT("elapsedSeconds"), Event.ElapsedSeconds);
		Writer->WriteValue(TEXT("name"), Event.Name.ToString());
		Writer->WriteValue(TEXT("category"), Event.Category);
		Writer->WriteValue(TEXT("details"), Event.Details);
		Writer->WriteValue(TEXT("severity"), LexToString(Event.Severity));
		Writer->WriteValue(TEXT("color"), Event.Color.ToString());
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();

	return FFileHelper::SaveStringToFile(JsonContent, *FilePath);
}

bool UPerformanceCaptureSubsystem::SaveCaptureSummaryToCsv(const FString& FilePath, FPerformanceCaptureReport& OutReport) const
{
	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	BuildReportFromSamples(CaptureSessionSamples, CaptureSessionEvents, CaptureSessionStartUtc, ActiveThresholds, OutReport);

	FString FailureList = FString::Join(OutReport.ThresholdFailures, TEXT(" | "));
	FailureList.ReplaceInline(TEXT("\""), TEXT("\"\""));

	FString CsvContent = TEXT("sample_count,event_count,duration_seconds,avg_fps,min_fps,max_fps,dominant_bottleneck,frame_avg_ms,frame_min_ms,frame_max_ms,game_avg_ms,game_max_ms,render_avg_ms,render_max_ms,input_avg_ms,input_max_ms,rhi_avg_ms,rhi_max_ms,thresholds_configured,passed_thresholds,threshold_failure_count,threshold_failures");
	CsvContent += LINE_TERMINATOR;
	CsvContent += FString::Printf(
		TEXT("%d,%d,%.6f,%.2f,%.2f,%.2f,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%s,%s,%d,\"%s\""),
		OutReport.SampleCount,
		CaptureSessionEvents.Num(),
		OutReport.DurationSeconds,
		OutReport.AverageFPS,
		OutReport.MinFPS,
		OutReport.MaxFPS,
		*OutReport.DominantBottleneck,
		OutReport.AverageFrameTimeMs,
		OutReport.MinFrameTimeMs,
		OutReport.MaxFrameTimeMs,
		OutReport.AverageGameThreadTimeMs,
		OutReport.MaxGameThreadTimeMs,
		OutReport.AverageRenderThreadTimeMs,
		OutReport.MaxRenderThreadTimeMs,
		OutReport.AverageInputLatencyTimeMs,
		OutReport.MaxInputLatencyTimeMs,
		OutReport.AverageRHITTimeMs,
		OutReport.MaxRHITTimeMs,
		OutReport.bThresholdsConfigured ? TEXT("true") : TEXT("false"),
		OutReport.bPassedThresholds ? TEXT("true") : TEXT("false"),
		OutReport.ThresholdFailures.Num(),
		*FailureList);

	return FFileHelper::SaveStringToFile(CsvContent, *FilePath);
}

bool UPerformanceCaptureSubsystem::SaveCaptureSummaryToJson(const FString& FilePath, const FPerformanceCaptureReport& Report) const
{
	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	const FMetricSummary FrameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary InputSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.InputLatencyTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

	FString JsonContent;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonContent);

	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("captureStartedUtc"), CaptureSessionStartUtc.ToIso8601());
	Writer->WriteValue(TEXT("captureSavedUtc"), FDateTime::UtcNow().ToIso8601());
	Writer->WriteValue(TEXT("sampleCount"), Report.SampleCount);
	Writer->WriteValue(TEXT("durationSeconds"), Report.DurationSeconds);
	Writer->WriteValue(TEXT("eventCount"), CaptureSessionEvents.Num());
	Writer->WriteValue(TEXT("dominantBottleneck"), Report.DominantBottleneck);
	Writer->WriteValue(TEXT("averageFps"), Report.AverageFPS);
	Writer->WriteValue(TEXT("minFps"), Report.MinFPS);
	Writer->WriteValue(TEXT("maxFps"), Report.MaxFPS);

	auto WriteMetricSummary = [&Writer](const TCHAR* MetricName, const FMetricSummary& Summary)
	{
		Writer->WriteObjectStart(MetricName);
		Writer->WriteValue(TEXT("min"), Summary.Min);
		Writer->WriteValue(TEXT("average"), Summary.Average);
		Writer->WriteValue(TEXT("max"), Summary.Max);
		Writer->WriteValue(TEXT("last"), Summary.Last);
		Writer->WriteObjectEnd();
	};

	WriteMetricSummary(TEXT("frameMs"), FrameSummary);
	WriteMetricSummary(TEXT("gameThreadMs"), GameSummary);
	WriteMetricSummary(TEXT("renderThreadMs"), RenderSummary);
	WriteMetricSummary(TEXT("inputLatencyMs"), InputSummary);
	WriteMetricSummary(TEXT("rhiThreadMs"), RHISummary);
	WriteThresholdSummary(Writer, Report);
	Writer->WriteArrayStart(TEXT("events"));
	for (const FPerformanceCaptureEvent& Event : CaptureSessionEvents)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("elapsedSeconds"), Event.ElapsedSeconds);
		Writer->WriteValue(TEXT("name"), Event.Name.ToString());
		Writer->WriteValue(TEXT("category"), Event.Category);
		Writer->WriteValue(TEXT("details"), Event.Details);
		Writer->WriteValue(TEXT("severity"), LexToString(Event.Severity));
		Writer->WriteValue(TEXT("color"), Event.Color.ToString());
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();

	return FFileHelper::SaveStringToFile(JsonContent, *FilePath);
}

bool UPerformanceCaptureSubsystem::SaveCaptureSummaryToPng(const FString& FilePath, const FPerformanceCaptureReport& Report) const
{
	if (CaptureSessionSamples.Num() == 0 || !ShouldExportSummaryPng())
	{
		return false;
	}

	const FMetricSummary FrameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary InputSummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.InputLatencyTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(CaptureSessionSamples, [](const FRuntimePerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

	const TSharedRef<SWidget> ReportWidget =
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.08f, 0.09f, 0.11f, 1.0f))
		.Padding(18.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Runtime Performance Session Report")))
				.Font(FCoreStyle::Get().GetFontStyle("HeadingMedium"))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(
					TEXT("Start %s | Duration %.2f s | Samples %d | Events %d | Dominant bottleneck %s"),
					*CaptureSessionStartUtc.ToString(TEXT("%Y-%m-%d %H:%M:%S UTC")),
					Report.DurationSeconds,
					Report.SampleCount,
					CaptureSessionEvents.Num(),
					*Report.DominantBottleneck)))
				.AutoWrapText(true)
				.ColorAndOpacity(FLinearColor(0.82f, 0.86f, 0.90f, 0.95f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.12f, 0.13f, 0.16f, 1.0f))
				.Padding(12.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(
						TEXT("FPS avg %.1f | min %.1f | max %.1f\nFrame avg %.2f ms | min %.2f | max %.2f\nGame avg %.2f ms | Render avg %.2f ms | Input avg %.2f ms | RHI avg %.2f ms\nThresholds %s"),
						Report.AverageFPS,
						Report.MinFPS,
						Report.MaxFPS,
						FrameSummary.Average,
						FrameSummary.Min,
						FrameSummary.Max,
						GameSummary.Average,
						RenderSummary.Average,
						InputSummary.Average,
						RHISummary.Average,
						Report.bThresholdsConfigured ? (Report.bPassedThresholds ? TEXT("PASS") : TEXT("FAIL")) : TEXT("OFF"))))
					.AutoWrapText(true)
					.ColorAndOpacity(FLinearColor::White)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.12f, 0.13f, 0.16f, 1.0f))
				.Padding(12.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Graph Legend")))
						.Font(FCoreStyle::Get().GetFontStyle("HeadingSmall"))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						BuildLegendRow(FLinearColor(0.10f, 1.00f, 0.10f, 1.0f), TEXT("Frame  | Total frame time. Main indicator of overall FPS."))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						BuildLegendRow(FLinearColor(1.00f, 0.20f, 0.20f, 1.0f), TEXT("Game   | Game thread CPU time. High values mean gameplay/script logic is expensive."))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						BuildLegendRow(FLinearColor(0.25f, 0.55f, 1.00f, 1.0f), TEXT("Render | Render thread CPU time. High values mean rendering submission is expensive."))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						BuildLegendRow(FLinearColor(1.00f, 0.45f, 1.00f, 1.0f), TEXT("RHI    | RHI thread CPU time. High values mean low-level rendering/RHI work is expensive."))
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSessionReportGraphWidget)
				.Samples(&CaptureSessionSamples)
				.Events(&CaptureSessionEvents)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				BuildSessionEventsListWidget(CaptureSessionEvents)
			]
		];

	const FVector2D DrawSize(1200.0f, CaptureSessionEvents.Num() > 0 ? 940.0f : 820.0f);
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	if (!RenderTarget)
	{
		return false;
	}

	RenderTarget->ClearColor = FLinearColor(0.08f, 0.09f, 0.11f, 1.0f);
	RenderTarget->InitCustomFormat(static_cast<uint32>(DrawSize.X), static_cast<uint32>(DrawSize.Y), PF_B8G8R8A8, false);
	RenderTarget->UpdateResourceImmediate(true);

	FWidgetRenderer WidgetRenderer(true, false);
	WidgetRenderer.DrawWidget(RenderTarget, ReportWidget, DrawSize, 0.0f);

	TArray<FColor> Pixels;
	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource || !RenderTargetResource->ReadPixels(Pixels) || Pixels.Num() == 0)
	{
		return false;
	}

	TArray64<uint8> CompressedPng;
	FImageUtils::PNGCompressImageArray(static_cast<int32>(DrawSize.X), static_cast<int32>(DrawSize.Y), TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), CompressedPng);
	return FFileHelper::SaveArrayToFile(CompressedPng, *FilePath);
}

bool UPerformanceCaptureSubsystem::SaveCaptureEventsToCsv(const FString& FilePath) const
{
	if (CaptureSessionEvents.Num() == 0)
	{
		return false;
	}

	FString CsvContent = TEXT("timestamp_utc,elapsed_seconds,name,category,severity,details,color");
	CsvContent += LINE_TERMINATOR;

	for (const FPerformanceCaptureEvent& Event : CaptureSessionEvents)
	{
		FString EscapedDetails = Event.Details;
		EscapedDetails.ReplaceInline(TEXT("\""), TEXT("\"\""));

		CsvContent += FString::Printf(TEXT("%s,%.6f,%s,%s,%s,\"%s\",%s"),
			*Event.TimestampUtc.ToIso8601(),
			Event.ElapsedSeconds,
			*Event.Name.ToString(),
			*Event.Category,
			*LexToString(Event.Severity),
			*EscapedDetails,
			*Event.Color.ToString());
		CsvContent += LINE_TERMINATOR;
	}

	return FFileHelper::SaveStringToFile(CsvContent, *FilePath);
}

bool UPerformanceCaptureSubsystem::SaveCaptureEventsToJson(const FString& FilePath) const
{
	if (CaptureSessionEvents.Num() == 0)
	{
		return false;
	}

	FString JsonContent;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonContent);

	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("captureStartedUtc"), CaptureSessionStartUtc.ToIso8601());
	Writer->WriteArrayStart(TEXT("events"));
	for (const FPerformanceCaptureEvent& Event : CaptureSessionEvents)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("timestampUtc"), Event.TimestampUtc.ToIso8601());
		Writer->WriteValue(TEXT("elapsedSeconds"), Event.ElapsedSeconds);
		Writer->WriteValue(TEXT("name"), Event.Name.ToString());
		Writer->WriteValue(TEXT("category"), Event.Category);
		Writer->WriteValue(TEXT("details"), Event.Details);
		Writer->WriteValue(TEXT("severity"), LexToString(Event.Severity));
		Writer->WriteValue(TEXT("color"), Event.Color.ToString());
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();

	return FFileHelper::SaveStringToFile(JsonContent, *FilePath);
}
