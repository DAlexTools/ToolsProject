#include "SPerformanceGraphWidget.h"

#include "Editor.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/Platform.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PerformanceCaptureSubsystem.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Rendering/DrawElements.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Slate/WidgetRenderer.h"
#include "TextureResource.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
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

	constexpr float MinGraphScaleMs = 16.67f;
	constexpr float BaseGraphScaleMs = 60.0f;
	constexpr float GraphTopPaddingPx = 20.0f;
	constexpr float GraphBottomPaddingPx = 16.0f;
	constexpr float GraphLeftPaddingPx = 46.0f;
	constexpr float GraphRightPaddingPx = 12.0f;
	constexpr float SavedOverviewHeightPx = 44.0f;
	constexpr float SavedOverviewGapPx = 12.0f;
	constexpr float DefaultFrameBudgetMs = 33.3f;
	constexpr float DefaultThreadBudgetMs = 16.67f;
	constexpr float DefaultInputBudgetMs = 50.0f;
	constexpr float DefaultDisabledBudgetMs = 0.0f;

	struct FMetricSummary
	{
		float Min = 0.0f;
		float Average = 0.0f;
		float Max = 0.0f;
		float Last = 0.0f;
	};
	
	struct FEventLabelCluster
	{
		int32 EventCount = 0;
		double StartSeconds = 0.0;
		double EndSeconds = 0.0;
		FString Category;
		FLinearColor Color = FLinearColor::White;
	};
	
	FMetricSummary BuildMetricSummary(const TArray<FPerformanceCaptureSample>& Samples, TFunctionRef<float(const FPerformanceCaptureSample&)> Accessor)
	{
		FMetricSummary Summary;
		if (Samples.Num() == 0)
		{
			return Summary;
		}
	
		float Sum = 0.0f;
		Summary.Min = TNumericLimits<float>::Max();
		Summary.Max = TNumericLimits<float>::Lowest();
	
		for (const FPerformanceCaptureSample& Sample : Samples)
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

	EPerformanceCaptureEventSeverity ParseSeverity(const FString& Value)
	{
		if (Value.Equals(TEXT("Critical"), ESearchCase::IgnoreCase))
		{
			return EPerformanceCaptureEventSeverity::Critical;
		}
	
		if (Value.Equals(TEXT("Warning"), ESearchCase::IgnoreCase))
		{
			return EPerformanceCaptureEventSeverity::Warning;
		}
	
		return EPerformanceCaptureEventSeverity::Info;
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
	
	bool IsFrameSeries(FName SeriesName)
	{
		return SeriesName == TEXT("FrameTime");
	}
	
	bool IsInputSeries(FName SeriesName)
	{
		return SeriesName == TEXT("InputTime");
	}
	
	bool IsGpuSeries(FName SeriesName)
	{
		return SeriesName.ToString().StartsWith(TEXT("GPU "));
	}
	
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
	
	FString GetSampleBottleneck(const FPerformanceCaptureSample& Sample)
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
	
	FString GetDominantBottleneck(const TArray<FPerformanceCaptureSample>& Samples)
	{
		if (Samples.Num() == 0)
		{
			return TEXT("Unknown");
		}
	
		TMap<FString, int32> BottleneckCounts;
		for (const FPerformanceCaptureSample& Sample : Samples)
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
	
	UPerformanceCaptureSubsystem* GetPerformanceCaptureSubsystem()
	{
		if (!GEngine)
		{
			return nullptr;
		}
	
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType != EWorldType::PIE && WorldContext.WorldType != EWorldType::Game)
			{
				continue;
			}
	
			if (UWorld* World = WorldContext.World())
			{
				if (UGameInstance* GameInstance = World->GetGameInstance())
				{
					return GameInstance->GetSubsystem<UPerformanceCaptureSubsystem>();
				}
			}
		}
	
		return nullptr;
	}

	// todo leaf widget #2 
	class SSessionReportGraphWidget : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SSessionReportGraphWidget) {}
			SLATE_ARGUMENT(const TArray<FPerformanceCaptureSample>*, Samples)
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
			for (const FPerformanceCaptureSample& Sample : *Samples)
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
	
			auto DrawSeries = [&](TFunctionRef<float(const FPerformanceCaptureSample&)> Accessor, const FLinearColor& Color)
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
	
			DrawSeries([](const FPerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; }, FLinearColor(0.10f, 1.00f, 0.10f));
			DrawSeries([](const FPerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; }, FLinearColor(1.00f, 0.20f, 0.20f));
			DrawSeries([](const FPerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; }, FLinearColor(0.25f, 0.55f, 1.00f));
			DrawSeries([](const FPerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; }, FLinearColor(1.00f, 0.45f, 1.00f));
	
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
		const TArray<FPerformanceCaptureSample>* Samples = nullptr;
		const TArray<FPerformanceCaptureEvent>* Events = nullptr;
	};
}

void SPerformanceGraphWidget::Construct(const FArguments& InArgs)
{
	DataSourceText = FText::FromString(TEXT("Waiting for data"));

	AddSeries(TEXT("FrameTime"), FLinearColor(0.10f, 1.00f, 0.10f));
	AddSeries(TEXT("GameThread"), FLinearColor(1.00f, 0.20f, 0.20f));
	AddSeries(TEXT("RenderThread"), FLinearColor(0.25f, 0.55f, 1.00f));
	AddSeries(TEXT("RHITTime"), FLinearColor(1.00f, 0.45f, 1.00f));
	AddSeries(TEXT("InputTime"), FLinearColor(1.00f, 0.65f, 0.10f));

	for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
	{
		AddSeries(FName(*FString::Printf(TEXT("GPU %d FrameTime"), GPUIndex)), FLinearColor(1.00f, 0.90f, 0.15f));
	}

	InitializeDefaultBudgets();

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBox)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(30.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(24.0f, 0.0f, 8.0f, 0.0f)
			[
				SAssignNew(SeriesVisibilityComboButton, SComboButton)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &SPerformanceGraphWidget::GetVisibleSeriesSummary)
					.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
				]
				.MenuContent()
				[
					BuildSeriesVisibilityMenu()
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(ThresholdComboButton, SComboButton)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &SPerformanceGraphWidget::GetThresholdSummary)
					.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
				]
				.MenuContent()
				[
					BuildThresholdMenu()
				]
			]
		]
	];
}

void SPerformanceGraphWidget::SetPaused(bool bInPaused)
{
	bIsPaused = bInPaused;
}

void SPerformanceGraphWidget::ClearSamples()
{
	ExitSavedSessionMode();

	for (TPair<FName, FPerformanceGraphSeries>& SeriesPair : GraphSeries)
	{
		SeriesPair.Value.Samples.Reset();
	}

	CurrentStats = FStatUnitData();
	SmoothedMaxObservedTime = 1.0f;
	MaxFrameTime = 0.0f;
	MaxGameThreadTime = 0.0f;
	MaxRenderThreadTime = 0.0f;
	MaxRHITTime = 0.0f;
	DataSourceText = FText::FromString(TEXT("Waiting for data"));
	CaptureSessionSamples.Reset();
	CaptureSessionEvents.Reset();
	bIsCaptureSessionActive = false;
	CaptureSessionStartUtc = FDateTime();
	SyncedRuntimeEventCount = 0;
}

void SPerformanceGraphWidget::SetMaxSamples(int32 InMaxSamples)
{
	MaxSamples = FMath::Max(2, InMaxSamples);
	TrimSeriesToMaxSamples();
}

int32 SPerformanceGraphWidget::GetMaxSamples() const
{
	return MaxSamples;
}

void SPerformanceGraphWidget::SetAutoScaleEnabled(bool bEnabled)
{
	bAutoScaleEnabled = bEnabled;
}

bool SPerformanceGraphWidget::IsAutoScaleEnabled() const
{
	return bAutoScaleEnabled;
}

FText SPerformanceGraphWidget::GetDataSourceText() const
{
	return DataSourceText;
}

bool SPerformanceGraphWidget::LoadCaptureSessionFromJson(const FString& FilePath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
	{
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}

	TArray<FPerformanceCaptureSample> LoadedSamples;
	const TArray<TSharedPtr<FJsonValue>>* SampleArray = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("samples"), SampleArray) || !SampleArray)
	{
		return false;
	}

	LoadedSamples.Reserve(SampleArray->Num());
	for (const TSharedPtr<FJsonValue>& SampleValue : *SampleArray)
	{
		const TSharedPtr<FJsonObject> SampleObject = SampleValue.IsValid() ? SampleValue->AsObject() : nullptr;
		if (!SampleObject.IsValid())
		{
			continue;
		}

		FPerformanceCaptureSample Sample;
		const FString TimestampString = SampleObject->GetStringField(TEXT("timestampUtc"));
		FDateTime::ParseIso8601(*TimestampString, Sample.TimestampUtc);
		Sample.ElapsedSeconds = SampleObject->GetNumberField(TEXT("elapsedSeconds"));
		Sample.FrameTimeMs = SampleObject->GetNumberField(TEXT("frameTimeMs"));
		Sample.GameThreadTimeMs = SampleObject->GetNumberField(TEXT("gameThreadTimeMs"));
		Sample.RenderThreadTimeMs = SampleObject->GetNumberField(TEXT("renderThreadTimeMs"));
		Sample.InputLatencyTimeMs = SampleObject->GetNumberField(TEXT("inputLatencyTimeMs"));
		Sample.RHITTimeMs = SampleObject->GetNumberField(TEXT("rhiThreadTimeMs"));

		const TArray<TSharedPtr<FJsonValue>>* GPUArray = nullptr;
		if (SampleObject->TryGetArrayField(TEXT("gpuFrameTimesMs"), GPUArray) && GPUArray)
		{
			for (int32 GPUIndex = 0; GPUIndex < FMath::Min(GPUArray->Num(), MAX_NUM_GPUS); ++GPUIndex)
			{
				Sample.GPUFrameTimesMs[GPUIndex] = static_cast<float>((*GPUArray)[GPUIndex]->AsNumber());
			}
		}

		LoadedSamples.Add(MoveTemp(Sample));
	}

	if (LoadedSamples.Num() == 0)
	{
		return false;
	}

	TArray<FPerformanceCaptureEvent> LoadedEvents;
	const TArray<TSharedPtr<FJsonValue>>* EventArray = nullptr;
	if (RootObject->TryGetArrayField(TEXT("events"), EventArray) && EventArray)
	{
		LoadedEvents.Reserve(EventArray->Num());
		for (const TSharedPtr<FJsonValue>& EventValue : *EventArray)
		{
			const TSharedPtr<FJsonObject> EventObject = EventValue.IsValid() ? EventValue->AsObject() : nullptr;
			if (!EventObject.IsValid())
			{
				continue;
			}

			FPerformanceCaptureEvent Event;
			const FString TimestampString = EventObject->GetStringField(TEXT("timestampUtc"));
			FDateTime::ParseIso8601(*TimestampString, Event.TimestampUtc);
			Event.ElapsedSeconds = EventObject->GetNumberField(TEXT("elapsedSeconds"));
			Event.Name = FName(*EventObject->GetStringField(TEXT("name")));
			Event.Category = EventObject->GetStringField(TEXT("category"));
			Event.Details = EventObject->GetStringField(TEXT("details"));
			Event.Severity = ParseSeverity(EventObject->GetStringField(TEXT("severity")));
			Event.Color = FLinearColor::Transparent;
			Event.Color.InitFromString(EventObject->GetStringField(TEXT("color")));
			LoadedEvents.Add(MoveTemp(Event));
		}
	}

	const FString CaptureStartedUtcString = RootObject->GetStringField(TEXT("captureStartedUtc"));
	FDateTime LoadedCaptureStartUtc;
	FDateTime::ParseIso8601(*CaptureStartedUtcString, LoadedCaptureStartUtc);

	SavedSessionSamples = MoveTemp(LoadedSamples);
	SavedSessionEvents = MoveTemp(LoadedEvents);
	SavedSessionSourcePath = FilePath;
	CaptureSessionStartUtc = LoadedCaptureStartUtc != FDateTime() ? LoadedCaptureStartUtc : SavedSessionSamples[0].TimestampUtc;
	bIsSavedSessionMode = true;
	bIsPanningSavedSession = false;
	ClearSelectedRange();
	ResetSavedSessionView();
	DataSourceText = FText::FromString(FString::Printf(TEXT("Source: Saved Session (%s)"), *FPaths::GetCleanFilename(FilePath)));
	return true;
}

void SPerformanceGraphWidget::ExitSavedSessionMode()
{
	bIsSavedSessionMode = false;
	bIsPanningSavedSession = false;
	bIsDraggingSavedOverview = false;
	bIsSelectingSavedRange = false;
	SavedSessionDragAnchor = FVector2D::ZeroVector;
	SavedSessionSamples.Reset();
	SavedSessionEvents.Reset();
	SavedSessionSourcePath.Reset();
	SavedSessionViewStartIndex = 0;
	SavedSessionViewSampleCount = 0;
	ClearSelectedRange();
}

bool SPerformanceGraphWidget::IsSavedSessionModeActive() const
{
	return bIsSavedSessionMode;
}

void SPerformanceGraphWidget::ZoomSavedSession(float ZoomFactor)
{
	if (!bIsSavedSessionMode || SavedSessionSamples.Num() < 2)
	{
		return;
	}

	const int32 CurrentCount = GetSavedVisibleSampleCount();
	const int32 NewCount = FMath::Clamp(FMath::RoundToInt(CurrentCount * ZoomFactor), 10, SavedSessionSamples.Num());
	if (NewCount == CurrentCount)
	{
		return;
	}

	const int32 CurrentStart = GetSavedVisibleStartIndex();
	const int32 CurrentCenter = CurrentStart + CurrentCount / 2;
	SavedSessionViewSampleCount = NewCount;
	SavedSessionViewStartIndex = CurrentCenter - NewCount / 2;
	ClampSavedSessionView();
	ClearSelectedRange();
}

void SPerformanceGraphWidget::PanSavedSession(int32 SampleDelta)
{
	if (!bIsSavedSessionMode || SavedSessionSamples.Num() == 0)
	{
		return;
	}

	SavedSessionViewStartIndex += SampleDelta;
	ClampSavedSessionView();
	ClearSelectedRange();
}

void SPerformanceGraphWidget::ResetSavedSessionView()
{
	if (!bIsSavedSessionMode || SavedSessionSamples.Num() == 0)
	{
		return;
	}

	SavedSessionViewStartIndex = 0;
	SavedSessionViewSampleCount = SavedSessionSamples.Num();
	ClampSavedSessionView();
	ClearSelectedRange();
}

FString SPerformanceGraphWidget::GetSavedSessionSourcePath() const
{
	return SavedSessionSourcePath;
}

FString SPerformanceGraphWidget::GetCaptureDirectoryPath() const
{
	return BuildCaptureDirectory();
}

FString SPerformanceGraphWidget::GetLastCaptureJsonPath() const
{
	return LastCaptureJsonPath;
}

void SPerformanceGraphWidget::FocusAllMetrics()
{
	SetFocusedSeries(SeriesDrawOrder);
}

void SPerformanceGraphWidget::FocusFrameMetric()
{
	TArray<FName> VisibleSeriesNames;
	VisibleSeriesNames.Add(TEXT("FrameTime"));
	SetFocusedSeries(VisibleSeriesNames);
}

void SPerformanceGraphWidget::FocusGameThreadMetric()
{
	TArray<FName> VisibleSeriesNames;
	VisibleSeriesNames.Add(TEXT("GameThread"));
	SetFocusedSeries(VisibleSeriesNames);
}

void SPerformanceGraphWidget::FocusRenderThreadMetric()
{
	TArray<FName> VisibleSeriesNames;
	VisibleSeriesNames.Add(TEXT("RenderThread"));
	SetFocusedSeries(VisibleSeriesNames);
}

void SPerformanceGraphWidget::FocusRHIMetric()
{
	TArray<FName> VisibleSeriesNames;
	VisibleSeriesNames.Add(TEXT("RHITTime"));
	SetFocusedSeries(VisibleSeriesNames);
}

void SPerformanceGraphWidget::FocusGPUMetrics()
{
	TArray<FName> VisibleSeriesNames;
	for (const FName& SeriesName : SeriesDrawOrder)
	{
		if (IsGpuSeries(SeriesName))
		{
			VisibleSeriesNames.Add(SeriesName);
		}
	}

	SetFocusedSeries(VisibleSeriesNames);
}

void SPerformanceGraphWidget::ClearSelectedRange() // refactoring in function library
{
	bIsSelectingSavedRange = false;
	bHasSelectedRange = false;
	SavedRangeSelectionAnchor = FVector2D::ZeroVector;
	SelectedRangeStartIndex = INDEX_NONE;
	SelectedRangeEndIndex = INDEX_NONE;
}

bool SPerformanceGraphWidget::HasSelectedRange() const
{
	return bHasSelectedRange && SelectedRangeStartIndex != INDEX_NONE && SelectedRangeEndIndex != INDEX_NONE;
}

bool SPerformanceGraphWidget::GetSeriesStats(FName SeriesName, FPerformanceSeriesStats& OutStats) const
{
	if (bIsSavedSessionMode)
	{
		int32 StartIndex = INDEX_NONE;
		int32 EndIndex = INDEX_NONE;
		if (!GetActiveSavedSampleRange(StartIndex, EndIndex))
		{
			OutStats = FPerformanceSeriesStats();
			return false;
		}

		float Sum = 0.0f;
		float Peak = 0.0f;
		int32 SampleCount = 0;
		for (int32 SampleIndex = StartIndex; SampleIndex <= EndIndex; ++SampleIndex)
		{
			const float Value = GetSavedSeriesValue(SavedSessionSamples[SampleIndex], SeriesName);
			Sum += Value;
			Peak = FMath::Max(Peak, Value);
			OutStats.CurrentMs = Value;
			++SampleCount;
		}

		OutStats.AverageMs = SampleCount > 0 ? Sum / SampleCount : 0.0f;
		OutStats.MaxMs = Peak;
		OutStats.bHasSamples = SampleCount > 0;
		return OutStats.bHasSamples;
	}

	const FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName);
	if (!Series || Series->Samples.Num() == 0)
	{
		OutStats = FPerformanceSeriesStats();
		return false;
	}

	float Sum = 0.0f;
	float Peak = 0.0f;
	for (const FPerformanceGraphSample& Sample : Series->Samples)
	{
		Sum += Sample.TimeMs;
		Peak = FMath::Max(Peak, Sample.TimeMs);
	}

	OutStats.CurrentMs = Series->Samples.Last().TimeMs;
	OutStats.AverageMs = Sum / Series->Samples.Num();
	OutStats.MaxMs = Peak;
	OutStats.bHasSamples = true;
	return true;
}

bool SPerformanceGraphWidget::GetActiveSavedSampleRange(int32& OutStartIndex, int32& OutEndIndex) const
{
	if (SavedSessionSamples.Num() == 0)
	{
		OutStartIndex = INDEX_NONE;
		OutEndIndex = INDEX_NONE;
		return false;
	}

	if (HasSelectedRange())
	{
		OutStartIndex = FMath::Clamp(SelectedRangeStartIndex, 0, SavedSessionSamples.Num() - 1);
		OutEndIndex = FMath::Clamp(SelectedRangeEndIndex, 0, SavedSessionSamples.Num() - 1);
		if (OutStartIndex > OutEndIndex)
		{
			Swap(OutStartIndex, OutEndIndex);
		}

		return true;
	}

	OutStartIndex = GetSavedVisibleStartIndex();
	OutEndIndex = GetSavedVisibleEndIndex();
	return OutStartIndex != INDEX_NONE && OutEndIndex != INDEX_NONE && OutStartIndex <= OutEndIndex;
}

void SPerformanceGraphWidget::StartCaptureSession()
{
	if (bIsSavedSessionMode)
	{
		ExitSavedSessionMode();
	}

	CaptureSessionSamples.Reset();
	CaptureSessionEvents.Reset();
	CaptureSessionStartUtc = FDateTime::UtcNow();
	LastCaptureCsvPath.Reset();
	LastCaptureJsonPath.Reset();
	bIsCaptureSessionActive = true;
	SyncedRuntimeEventCount = 0;
	SyncRuntimeEvents();
}

bool SPerformanceGraphWidget::StopCaptureSession(FString& OutCsvPath, FString& OutJsonPath)
{
	OutCsvPath.Reset();
	OutJsonPath.Reset();

	if (!bIsCaptureSessionActive)
	{
		return false;
	}

	SyncRuntimeEvents();
	bIsCaptureSessionActive = false;

	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	const FString CaptureDirectory = BuildCaptureDirectory();
	IFileManager::Get().MakeDirectory(*CaptureDirectory, true);

	const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString BasePath = FPaths::Combine(CaptureDirectory, FString::Printf(TEXT("performance_capture_%s"), *Timestamp));
	const FString CsvPath = BasePath + TEXT(".csv");
	const FString JsonPath = BasePath + TEXT(".json");
	const FString SummaryCsvPath = BasePath + TEXT("_summary.csv");
	const FString SummaryJsonPath = BasePath + TEXT("_summary.json");
	const FString EventsCsvPath = BasePath + TEXT("_events.csv");
	const FString EventsJsonPath = BasePath + TEXT("_events.json");

	const bool bCsvSaved = SaveCaptureSessionToCsv(CsvPath);
	const bool bJsonSaved = SaveCaptureSessionToJson(JsonPath);
	const bool bSummaryCsvSaved = SaveCaptureSummaryToCsv(SummaryCsvPath);
	const bool bSummaryJsonSaved = SaveCaptureSummaryToJson(SummaryJsonPath);
	const bool bEventsCsvSaved = SaveCaptureEventsToCsv(EventsCsvPath);
	const bool bEventsJsonSaved = SaveCaptureEventsToJson(EventsJsonPath);

	if (bCsvSaved)
	{
		LastCaptureCsvPath = CsvPath;
		OutCsvPath = CsvPath;
	}

	if (bJsonSaved)
	{
		LastCaptureJsonPath = JsonPath;
		OutJsonPath = JsonPath;
	}

	if (bCsvSaved || bJsonSaved)
	{
		LastCaptureSavedUtc = FDateTime::UtcNow();
	}

	return bCsvSaved || bJsonSaved || bSummaryCsvSaved || bSummaryJsonSaved || bEventsCsvSaved || bEventsJsonSaved;
}

bool SPerformanceGraphWidget::IsCaptureSessionActive() const
{
	return bIsCaptureSessionActive;
}

int32 SPerformanceGraphWidget::GetCaptureSampleCount() const
{
	if (bIsSavedSessionMode)
	{
		return SavedSessionSamples.Num();
	}

	return CaptureSessionSamples.Num();
}

double SPerformanceGraphWidget::GetCaptureSessionDurationSeconds() const
{
	if (bIsSavedSessionMode && SavedSessionSamples.Num() > 0)
	{
		return SavedSessionSamples.Last().ElapsedSeconds;
	}

	if (CaptureSessionSamples.Num() > 0)
	{
		return CaptureSessionSamples.Last().ElapsedSeconds;
	}

	if (bIsCaptureSessionActive && CaptureSessionStartUtc != FDateTime())
	{
		return (FDateTime::UtcNow() - CaptureSessionStartUtc).GetTotalSeconds();
	}

	return 0.0;
}

FText SPerformanceGraphWidget::GetLastCaptureSummaryText() const
{
	if (bIsSavedSessionMode)
	{
		if (HasSelectedRange())
		{
			return FText::FromString(FString::Printf(TEXT("Selected range (%d samples)"), SelectedRangeEndIndex - SelectedRangeStartIndex + 1));
		}

		return FText::FromString(FString::Printf(TEXT("Viewing saved session (%d samples)"), SavedSessionSamples.Num()));
	}

	if (bIsCaptureSessionActive)
	{
		return FText::FromString(FString::Printf(TEXT("Capture REC (%d samples)"), CaptureSessionSamples.Num()));
	}

	if (CaptureSessionSamples.Num() == 0)
	{
		return FText::FromString(TEXT("Capture idle"));
	}

	if (!LastCaptureCsvPath.IsEmpty() || !LastCaptureJsonPath.IsEmpty())
	{
		return FText::FromString(FString::Printf(TEXT("Saved %d samples"), CaptureSessionSamples.Num()));
	}

	return FText::FromString(FString::Printf(TEXT("Capture stopped (%d samples)"), CaptureSessionSamples.Num()));
}

FText SPerformanceGraphWidget::GetCaptureSessionSummaryText() const
{
	const TArray<FPerformanceCaptureSample>& Samples = bIsSavedSessionMode ? SavedSessionSamples : CaptureSessionSamples;
	const TArray<FPerformanceCaptureEvent>& Events = bIsSavedSessionMode ? SavedSessionEvents : CaptureSessionEvents;
	if (Samples.Num() == 0)
	{
		return FText::FromString(TEXT("Session Summary: no recording yet"));
	}

	TArray<FPerformanceCaptureSample> SummarySamples;
	const TArray<FPerformanceCaptureSample>* SummarySamplesPtr = &Samples;
	if (bIsSavedSessionMode)
	{
		int32 StartIndex = INDEX_NONE;
		int32 EndIndex = INDEX_NONE;
		if (!GetActiveSavedSampleRange(StartIndex, EndIndex))
		{
			return FText::FromString(TEXT("Session Summary: no recording yet"));
		}

		SummarySamples.Reserve(EndIndex - StartIndex + 1);
		for (int32 SampleIndex = StartIndex; SampleIndex <= EndIndex; ++SampleIndex)
		{
			SummarySamples.Add(Samples[SampleIndex]);
		}

		SummarySamplesPtr = &SummarySamples;
	}

	const TArray<FPerformanceCaptureSample>& ActiveSamples = *SummarySamplesPtr;
	const FMetricSummary FrameSummary = BuildMetricSummary(ActiveSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(ActiveSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(ActiveSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(ActiveSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

	int32 ActiveEventCount = Events.Num();
	if (bIsSavedSessionMode && ActiveSamples.Num() > 0)
	{
		ActiveEventCount = 0;
		const double StartSeconds = ActiveSamples[0].ElapsedSeconds;
		const double EndSeconds = ActiveSamples.Last().ElapsedSeconds;
		for (const FPerformanceCaptureEvent& Event : Events)
		{
			if (Event.ElapsedSeconds >= StartSeconds && Event.ElapsedSeconds <= EndSeconds)
			{
				++ActiveEventCount;
			}
		}
	}

	const FString SummaryString = FString::Printf(
		TEXT("Session Summary: %s%s\nDuration: %.2f s | Samples: %d | Events: %d\nFPS avg %.1f | min %.1f | max %.1f\nFrame avg %.2f ms | min %.2f | max %.2f\nGame avg %.2f ms | Render avg %.2f ms | RHI avg %.2f ms"),
		bIsSavedSessionMode && HasSelectedRange() ? TEXT("Selected Range | ") : TEXT(""),
		*GetDominantBottleneck(ActiveSamples),
		ActiveSamples.Last().ElapsedSeconds - ActiveSamples[0].ElapsedSeconds,
		ActiveSamples.Num(),
		ActiveEventCount,
		FrameMsToFPS(FrameSummary.Average),
		FrameMsToFPS(FrameSummary.Max),
		FrameMsToFPS(FMath::Max(FrameSummary.Min, KINDA_SMALL_NUMBER)),
		FrameSummary.Average,
		FrameSummary.Min,
		FrameSummary.Max,
		GameSummary.Average,
		RenderSummary.Average,
		RHISummary.Average);

	return FText::FromString(SummaryString);
}

bool SPerformanceGraphWidget::CanExportCurrentSummaryPng() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	TArray<FPerformanceCaptureSample> Samples;
	TArray<FPerformanceCaptureEvent> Events;
	FDateTime SessionStartUtc;
	bool bIsSelectedRange = false;
	return BuildActiveSummaryExportData(Samples, Events, SessionStartUtc, bIsSelectedRange);
}

bool SPerformanceGraphWidget::ExportCurrentSummaryPng(const FString& FilePath) const
{
	TArray<FPerformanceCaptureSample> Samples;
	TArray<FPerformanceCaptureEvent> Events;
	FDateTime SessionStartUtc;
	bool bIsSelectedRange = false;
	if (!BuildActiveSummaryExportData(Samples, Events, SessionStartUtc, bIsSelectedRange))
	{
		return false;
	}

	return SaveCaptureSummaryToPng(FilePath, Samples, Events, SessionStartUtc, bIsSelectedRange);
}

void SPerformanceGraphWidget::AddSeries(FName SeriesName, const FLinearColor& Color)
{
	if (!GraphSeries.Contains(SeriesName))
	{
		GraphSeries.Add(SeriesName, FPerformanceGraphSeries(SeriesName, Color));
		SeriesDrawOrder.Add(SeriesName);
		SeriesVisibility.Add(SeriesName, true);
	}
}

void SPerformanceGraphWidget::AddSample(FName SeriesName, float TimeMs)
{
	if (FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName))
	{
		Series->Samples.Add(FPerformanceGraphSample(TimeMs));
		if (Series->Samples.Num() > MaxSamples)
		{
			Series->Samples.RemoveAt(0);
		}
	}
}

void SPerformanceGraphWidget::AppendCaptureSample()
{
	if (!bIsCaptureSessionActive)
	{
		return;
	}

	FPerformanceCaptureSample Sample;
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

void SPerformanceGraphWidget::SyncRuntimeEvents()
{
	UPerformanceCaptureSubsystem* Subsystem = GetPerformanceCaptureSubsystem();
	if (!Subsystem)
	{
		SyncedRuntimeEventCount = 0;
		return;
	}

	const TArray<FPerformanceCaptureEvent>& RecordedEvents = Subsystem->GetRecordedEvents();
	const int32 StartIndex = FMath::Clamp(SyncedRuntimeEventCount, 0, RecordedEvents.Num());

	for (int32 EventIndex = StartIndex; EventIndex < RecordedEvents.Num(); ++EventIndex)
	{
		AddEventToCapture(RecordedEvents[EventIndex]);
	}

	SyncedRuntimeEventCount = RecordedEvents.Num();
}

void SPerformanceGraphWidget::AddEventToCapture(const FPerformanceCaptureEvent& Event)
{
	if (!bIsCaptureSessionActive || CaptureSessionStartUtc == FDateTime() || Event.TimestampUtc < CaptureSessionStartUtc)
	{
		return;
	}

	FPerformanceCaptureEvent CaptureEvent = Event;
	CaptureEvent.ElapsedSeconds = (CaptureEvent.TimestampUtc - CaptureSessionStartUtc).GetTotalSeconds();
	CaptureEvent.Color = CaptureEvent.Color.A > 0.0f ? CaptureEvent.Color : GetSeverityFallbackColor(CaptureEvent.Severity);
	CaptureSessionEvents.Add(MoveTemp(CaptureEvent));
}

void SPerformanceGraphWidget::UpdateStatsFromData(const FStatUnitData& StatUnitData)
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

ECheckBoxState SPerformanceGraphWidget::IsSeriesVisible(FName SeriesName) const
{
	return IsSeriesVisibleBool(SeriesName) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SPerformanceGraphWidget::OnToggleSeries(ECheckBoxState NewState, FName SeriesName) // in function library
{
	SeriesVisibility.FindOrAdd(SeriesName) = (NewState == ECheckBoxState::Checked);
}

bool SPerformanceGraphWidget::IsSeriesVisibleBool(FName SeriesName) const // in funciton library
{
	if (const bool* bVisible = SeriesVisibility.Find(SeriesName))
	{
		return *bVisible;
	}

	return false;
}

TSharedRef<SWidget> SPerformanceGraphWidget::BuildSeriesVisibilityMenu()
{
	TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

	MenuContent->AddSlot()
	.AutoHeight()
	.Padding(2.0f, 0.0f, 2.0f, 6.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Focus Presets")))
		.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
	];

	TSharedRef<SHorizontalBox> FocusButtonsRowOne = SNew(SHorizontalBox);
	FocusButtonsRowOne->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("All")))
		.OnClicked_Lambda([this]() { FocusAllMetrics(); return FReply::Handled(); })
	];
	FocusButtonsRowOne->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("Frame")))
		.OnClicked_Lambda([this]() { FocusFrameMetric(); return FReply::Handled(); })
	];
	FocusButtonsRowOne->AddSlot().AutoWidth()
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("Game")))
		.OnClicked_Lambda([this]() { FocusGameThreadMetric(); return FReply::Handled(); })
	];
	MenuContent->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)[FocusButtonsRowOne];

	TSharedRef<SHorizontalBox> FocusButtonsRowTwo = SNew(SHorizontalBox);
	FocusButtonsRowTwo->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("Render")))
		.OnClicked_Lambda([this]() { FocusRenderThreadMetric(); return FReply::Handled(); })
	];
	FocusButtonsRowTwo->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("RHI")))
		.OnClicked_Lambda([this]() { FocusRHIMetric(); return FReply::Handled(); })
	];
	FocusButtonsRowTwo->AddSlot().AutoWidth()
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("GPU")))
		.OnClicked_Lambda([this]() { FocusGPUMetrics(); return FReply::Handled(); })
	];
	MenuContent->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[FocusButtonsRowTwo];

	MenuContent->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 6.0f)
	[
		SNew(SSeparator)
	];

	for (const FName& SeriesName : SeriesDrawOrder)
	{
		const FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName);
		if (!Series)
		{
			continue;
		}

		MenuContent->AddSlot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Series->Color)
				.Padding(FMargin(6.0f))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SPerformanceGraphWidget::IsSeriesVisible, SeriesName)
				.OnCheckStateChanged(this, &SPerformanceGraphWidget::OnToggleSeries, SeriesName)
				[
					SNew(STextBlock)
					.Text(FText::FromName(SeriesName))
					.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
				]
			]
		];
	}

	TSharedRef<SWidget> MenuBorder = SNew(SBorder)
		.Padding(8.0f)
		[
			SNew(SBox)
				.MinDesiredWidth(220.0f)
				.MaxDesiredHeight(260.0f)
				[
					SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							MenuContent
						]
				]
		];

	return MenuBorder;
}

TSharedRef<SWidget> SPerformanceGraphWidget::BuildThresholdMenu()
{
	TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

	MenuContent->AddSlot()
	.AutoHeight()
	.Padding(2.0f, 2.0f, 2.0f, 6.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Presets")))
		.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
	];

	auto AddPresetButton = [this, &MenuContent](const TCHAR* Label, float BudgetMs)
	{
		MenuContent->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SButton)
			.Text(FText::FromString(Label))
			.OnClicked_Lambda([this, BudgetMs]()
			{
				ApplyBudgetPreset(BudgetMs);
				return FReply::Handled();
			})
		];
	};

	AddPresetButton(TEXT("60 FPS Budget"), 16.67f);
	AddPresetButton(TEXT("30 FPS Budget"), 33.3f);
	AddPresetButton(TEXT("Unlocked"), DefaultDisabledBudgetMs);

	MenuContent->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 4.0f, 0.0f, 6.0f)
	[
		SNew(SSeparator)
	];

	MenuContent->AddSlot()
	.AutoHeight()
	.Padding(2.0f, 0.0f, 2.0f, 6.0f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Per-Series Budgets (ms)")))
		.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
	];

	for (const FName& SeriesName : SeriesDrawOrder)
	{
		const FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName);
		if (!Series)
		{
			continue;
		}

		MenuContent->AddSlot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Series->Color)
				.Padding(FMargin(6.0f))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromName(SeriesName))
				.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(72.0f)
				[
					SNew(SEditableTextBox)
					.Text_Lambda([this, SeriesName]()
					{
						const float BudgetMs = GetSeriesBudget(SeriesName);
						return BudgetMs > 0.0f
							? FText::FromString(FString::Printf(TEXT("%.2f"), BudgetMs))
							: FText::FromString(TEXT("off"));
					})
					.SelectAllTextWhenFocused(true)
					.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
					.OnTextCommitted_Lambda([this, SeriesName](const FText& NewText, ETextCommit::Type)
					{
						const FString TrimmedText = NewText.ToString().TrimStartAndEnd();
						if (TrimmedText.IsEmpty() || TrimmedText.Equals(TEXT("off"), ESearchCase::IgnoreCase))
						{
							SetSeriesBudget(SeriesName, DefaultDisabledBudgetMs);
							return;
						}

						float ParsedValue = 0.0f;
						if (LexTryParseString(ParsedValue, *TrimmedText))
						{
							SetSeriesBudget(SeriesName, ParsedValue);
						}
					})
				]
			]
		];
	}

	TSharedRef<SWidget> MenuBorder = SNew(SBorder)
		.Padding(8.0f)
		[
			SNew(SBox)
				.MinDesiredWidth(280.0f)
				.MaxDesiredHeight(420.0f)
				[
					SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							MenuContent
						]
				]
		];

	return MenuBorder;
}

FText SPerformanceGraphWidget::GetVisibleSeriesSummary() const // in function library
{
	int32 VisibleCount = 0;

	for (const FName& SeriesName : SeriesDrawOrder)
	{
		VisibleCount += IsSeriesVisibleBool(SeriesName) ? 1 : 0;
	}

	return FText::FromString(FString::Printf(TEXT("Series (%d/%d)"), VisibleCount, SeriesDrawOrder.Num()));
}

FText SPerformanceGraphWidget::GetThresholdSummary() const
{
	int32 ActiveBudgetCount = 0;
	for (const FName& SeriesName : SeriesDrawOrder)
	{
		ActiveBudgetCount += HasSeriesBudget(SeriesName) ? 1 : 0;
	}

	return FText::FromString(FString::Printf(TEXT("Budgets (%d)"), ActiveBudgetCount));
}

void SPerformanceGraphWidget::SetFocusedSeries(const TArray<FName>& VisibleSeriesNames) 
{// refactyoring in function library add maps in function parameter
	TSet<FName> VisibleSet;
	for (const FName& VisibleSeriesName : VisibleSeriesNames)
	{
		VisibleSet.Add(VisibleSeriesName);
	}

	for (const FName& SeriesName : SeriesDrawOrder)
	{
		SeriesVisibility.FindOrAdd(SeriesName) = VisibleSet.Contains(SeriesName);
	}
}

void SPerformanceGraphWidget::InitializeDefaultBudgets()
{
	for (const FName& SeriesName : SeriesDrawOrder)
	{
		if (IsFrameSeries(SeriesName))
		{
			SeriesBudgetsMs.Add(SeriesName, DefaultFrameBudgetMs);
		}
		else if (IsInputSeries(SeriesName))
		{
			SeriesBudgetsMs.Add(SeriesName, DefaultInputBudgetMs);
		}
		else
		{
			SeriesBudgetsMs.Add(SeriesName, DefaultThreadBudgetMs);
		}
	}
}

void SPerformanceGraphWidget::ApplyBudgetPreset(float InBudgetMs) 
{// refactoring in function library maps function parameter
	for (const FName& SeriesName : SeriesDrawOrder)
	{
		SetSeriesBudget(SeriesName, InBudgetMs);
	}
}

void SPerformanceGraphWidget::SetSeriesBudget(FName SeriesName, float InThresholdMs) 
{// refactoring in function library add maps Series in function parameter
	SeriesBudgetsMs.FindOrAdd(SeriesName) = FMath::Max(0.0f, InThresholdMs);
}

float SPerformanceGraphWidget::GetSeriesBudget(FName SeriesName) const 
{// refactoring in function library
	if (const float* BudgetMs = SeriesBudgetsMs.Find(SeriesName))
	{
		return *BudgetMs;
	}

	return DefaultDisabledBudgetMs;
}

bool SPerformanceGraphWidget::HasSeriesBudget(FName SeriesName) const
{// in function library or service 
	return GetSeriesBudget(SeriesName) > 0.0f;
}

bool SPerformanceGraphWidget::IsSeriesThresholdExceeded(FName SeriesName, float ValueMs) const
{
	const float BudgetMs = GetSeriesBudget(SeriesName);
	return BudgetMs > 0.0f && ValueMs > BudgetMs;
}

float SPerformanceGraphWidget::GetHighestActiveBudget() const
{
	float HighestBudgetMs = 0.0f;
	for (const TPair<FName, float>& BudgetPair : SeriesBudgetsMs)
	{
		if (BudgetPair.Value > 0.0f)
		{
			HighestBudgetMs = FMath::Max(HighestBudgetMs, BudgetPair.Value);
		}
	}

	return HighestBudgetMs;
}

float SPerformanceGraphWidget::GetLowestActiveBudget() const
{
	float LowestBudgetMs = TNumericLimits<float>::Max();
	bool bFoundBudget = false;
	for (const TPair<FName, float>& BudgetPair : SeriesBudgetsMs)
	{
		if (BudgetPair.Value > 0.0f)
		{
			LowestBudgetMs = FMath::Min(LowestBudgetMs, BudgetPair.Value);
			bFoundBudget = true;
		}
	}

	return bFoundBudget ? LowestBudgetMs : 0.0f;
}

float SPerformanceGraphWidget::GetDisplayMaxTime() const
{
	const float HighestBudgetMs = GetHighestActiveBudget();
	const float ThresholdDrivenMax = HighestBudgetMs > 0.0f
		? FMath::Max(BaseGraphScaleMs, HighestBudgetMs * 1.6f)
		: BaseGraphScaleMs;
	if (!bAutoScaleEnabled)
	{
		return FMath::Max(MinGraphScaleMs, ThresholdDrivenMax);
	}

	if (bIsSavedSessionMode)
	{
		float SavedObservedMax = 1.0f;
		const int32 StartIndex = GetSavedVisibleStartIndex();
		const int32 EndIndex = GetSavedVisibleEndIndex();
		for (int32 SampleIndex = StartIndex; SampleIndex <= EndIndex; ++SampleIndex)
		{
			const FPerformanceCaptureSample& Sample = SavedSessionSamples[SampleIndex];
			SavedObservedMax = FMath::Max(SavedObservedMax, Sample.FrameTimeMs);
			SavedObservedMax = FMath::Max(SavedObservedMax, Sample.GameThreadTimeMs);
			SavedObservedMax = FMath::Max(SavedObservedMax, Sample.RenderThreadTimeMs);
			SavedObservedMax = FMath::Max(SavedObservedMax, Sample.RHITTimeMs);
			for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
			{
				SavedObservedMax = FMath::Max(SavedObservedMax, Sample.GPUFrameTimesMs[GPUIndex]);
			}
		}

		return FMath::Max(MinGraphScaleMs, FMath::Max(ThresholdDrivenMax, SavedObservedMax * 1.15f));
	}

	const float ObservedDrivenMax = FMath::Max(ThresholdDrivenMax, SmoothedMaxObservedTime * 1.15f);
	return FMath::Max(MinGraphScaleMs, ObservedDrivenMax);
}

void SPerformanceGraphWidget::ClampSavedSessionView()
{
	if (!bIsSavedSessionMode || SavedSessionSamples.Num() == 0)
	{
		SavedSessionViewStartIndex = 0;
		SavedSessionViewSampleCount = 0;
		return;
	}

	SavedSessionViewSampleCount = FMath::Clamp(SavedSessionViewSampleCount, 2, SavedSessionSamples.Num());
	SavedSessionViewStartIndex = FMath::Clamp(SavedSessionViewStartIndex, 0, SavedSessionSamples.Num() - SavedSessionViewSampleCount);
}

int32 SPerformanceGraphWidget::GetSavedVisibleSampleCount() const
{
	return bIsSavedSessionMode ? FMath::Clamp(SavedSessionViewSampleCount, 2, SavedSessionSamples.Num()) : 0;
}

int32 SPerformanceGraphWidget::GetSavedVisibleStartIndex() const
{
	return bIsSavedSessionMode ? FMath::Clamp(SavedSessionViewStartIndex, 0, FMath::Max(0, SavedSessionSamples.Num() - GetSavedVisibleSampleCount())) : 0;
}

int32 SPerformanceGraphWidget::GetSavedVisibleEndIndex() const
{
	if (!bIsSavedSessionMode || SavedSessionSamples.Num() == 0)
	{
		return INDEX_NONE;
	}

	return GetSavedVisibleStartIndex() + GetSavedVisibleSampleCount() - 1;
}

float SPerformanceGraphWidget::GetSavedSeriesValue(const FPerformanceCaptureSample& Sample, FName SeriesName) const
{
	if (SeriesName == TEXT("FrameTime"))
	{
		return Sample.FrameTimeMs;
	}
	if (SeriesName == TEXT("GameThread"))
	{
		return Sample.GameThreadTimeMs;
	}
	if (SeriesName == TEXT("RenderThread"))
	{
		return Sample.RenderThreadTimeMs;
	}
	if (SeriesName == TEXT("InputTime"))
	{
		return Sample.InputLatencyTimeMs;
	}
	if (SeriesName == TEXT("RHITTime"))
	{
		return Sample.RHITTimeMs;
	}

	const FString SeriesString = SeriesName.ToString();
	int32 GPUIndex = INDEX_NONE;
	if (SeriesString.StartsWith(TEXT("GPU ")) && SeriesString.Len() > 4)
	{
		const FString IndexString = SeriesString.Mid(4).LeftChop(10).TrimStartAndEnd();
		if (LexTryParseString(GPUIndex, *IndexString) && Sample.GPUFrameTimesMs.IsValidIndex(GPUIndex))
		{
			return Sample.GPUFrameTimesMs[GPUIndex];
		}
	}

	return 0.0f;
}

bool SPerformanceGraphWidget::IsPointInSavedOverview(const FVector2D& LocalPosition, const FVector2D& WidgetSize) const
{
	if (!bIsSavedSessionMode)
	{
		return false;
	}

	const float OverviewTop = WidgetSize.Y - GraphBottomPaddingPx - SavedOverviewHeightPx;
	return LocalPosition.X >= GraphLeftPaddingPx
		&& LocalPosition.X <= WidgetSize.X - GraphRightPaddingPx
		&& LocalPosition.Y >= OverviewTop
		&& LocalPosition.Y <= OverviewTop + SavedOverviewHeightPx;
}

bool SPerformanceGraphWidget::IsPointInMainGraph(const FVector2D& LocalPosition, const FVector2D& WidgetSize) const
{
	const float GraphHeight = WidgetSize.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);
	const float GraphBottom = GraphTopPaddingPx + GraphHeight;
	return LocalPosition.X >= GraphLeftPaddingPx
		&& LocalPosition.X <= WidgetSize.X - GraphRightPaddingPx
		&& LocalPosition.Y >= GraphTopPaddingPx
		&& LocalPosition.Y <= GraphBottom;
}

int32 SPerformanceGraphWidget::GetSavedSampleIndexFromGraphPosition(const FVector2D& LocalPosition, const FVector2D& WidgetSize) const
{
	if (SavedSessionSamples.Num() == 0)
	{
		return INDEX_NONE;
	}

	const float GraphWidth = FMath::Max(1.0f, WidgetSize.X - GraphLeftPaddingPx - GraphRightPaddingPx);
	const int32 VisibleSampleCount = FMath::Max(GetSavedVisibleSampleCount(), 1);
	const float XStep = GraphWidth / FMath::Max(VisibleSampleCount - 1, 1);
	const int32 RelativeIndex = FMath::Clamp(FMath::RoundToInt((LocalPosition.X - GraphLeftPaddingPx) / XStep), 0, VisibleSampleCount - 1);
	return FMath::Clamp(GetSavedVisibleStartIndex() + RelativeIndex, 0, SavedSessionSamples.Num() - 1);
}

void SPerformanceGraphWidget::UpdateSelectedRangeFromGraphPositions(const FVector2D& StartLocalPosition, const FVector2D& EndLocalPosition, const FVector2D& WidgetSize)
{
	if (!bIsSavedSessionMode || SavedSessionSamples.Num() == 0)
	{
		ClearSelectedRange();
		return;
	}

	const int32 StartIndex = GetSavedSampleIndexFromGraphPosition(StartLocalPosition, WidgetSize);
	const int32 EndIndex = GetSavedSampleIndexFromGraphPosition(EndLocalPosition, WidgetSize);
	if (StartIndex == INDEX_NONE || EndIndex == INDEX_NONE)
	{
		ClearSelectedRange();
		return;
	}

	SelectedRangeStartIndex = FMath::Min(StartIndex, EndIndex);
	SelectedRangeEndIndex = FMath::Max(StartIndex, EndIndex);
	bHasSelectedRange = true;
}

void SPerformanceGraphWidget::CenterSavedSessionOnNormalizedPosition(float NormalizedPosition)
{
	if (!bIsSavedSessionMode || SavedSessionSamples.Num() == 0)
	{
		return;
	}

	const int32 VisibleCount = GetSavedVisibleSampleCount();
	const int32 CenterIndex = FMath::Clamp(FMath::RoundToInt(NormalizedPosition * FMath::Max(SavedSessionSamples.Num() - 1, 0)), 0, SavedSessionSamples.Num() - 1);
	SavedSessionViewStartIndex = CenterIndex - VisibleCount / 2;
	ClampSavedSessionView();
}

void SPerformanceGraphWidget::TrimSeriesToMaxSamples()
{
	for (TPair<FName, FPerformanceGraphSeries>& SeriesPair : GraphSeries)
	{
		FPerformanceGraphSeries& Series = SeriesPair.Value;
		if (Series.Samples.Num() > MaxSamples)
		{
			const int32 NumToRemove = Series.Samples.Num() - MaxSamples;
			Series.Samples.RemoveAt(0, NumToRemove);
		}
	}
}

FString SPerformanceGraphWidget::BuildCaptureDirectory() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PerformanceCaptures"));
}

bool SPerformanceGraphWidget::BuildActiveSummaryExportData(TArray<FPerformanceCaptureSample>& OutSamples, TArray<FPerformanceCaptureEvent>& OutEvents, FDateTime& OutStartUtc, bool& bOutIsSelectedRange) const
{
	OutSamples.Reset();
	OutEvents.Reset();
	OutStartUtc = FDateTime();
	bOutIsSelectedRange = false;

	if (bIsSavedSessionMode)
	{
		if (SavedSessionSamples.Num() == 0)
		{
			return false;
		}

		int32 StartIndex = 0;
		int32 EndIndex = SavedSessionSamples.Num() - 1;
		if (HasSelectedRange())
		{
			if (!GetActiveSavedSampleRange(StartIndex, EndIndex))
			{
				return false;
			}

			bOutIsSelectedRange = true;
		}

		OutSamples.Reserve(EndIndex - StartIndex + 1);
		for (int32 SampleIndex = StartIndex; SampleIndex <= EndIndex; ++SampleIndex)
		{
			OutSamples.Add(SavedSessionSamples[SampleIndex]);
		}

		const double StartSeconds = OutSamples[0].ElapsedSeconds;
		const double EndSeconds = OutSamples.Last().ElapsedSeconds;
		for (const FPerformanceCaptureEvent& Event : SavedSessionEvents)
		{
			if (Event.ElapsedSeconds >= StartSeconds && Event.ElapsedSeconds <= EndSeconds)
			{
				OutEvents.Add(Event);
			}
		}

		OutStartUtc = CaptureSessionStartUtc != FDateTime() ? CaptureSessionStartUtc : OutSamples[0].TimestampUtc;
		return true;
	}

	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	OutSamples = CaptureSessionSamples;
	OutEvents = CaptureSessionEvents;
	OutStartUtc = CaptureSessionStartUtc != FDateTime() ? CaptureSessionStartUtc : CaptureSessionSamples[0].TimestampUtc;
	return true;
}

bool SPerformanceGraphWidget::SaveCaptureSessionToCsv(const FString& FilePath) const
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

	for (const FPerformanceCaptureSample& Sample : CaptureSessionSamples)
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

bool SPerformanceGraphWidget::SaveCaptureSessionToJson(const FString& FilePath) const
{
	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	FString JsonContent;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonContent);
	const FMetricSummary FrameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary InputSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.InputLatencyTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

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
	Writer->WriteArrayStart(TEXT("gpuFrameMs"));
	for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
	{
		const FMetricSummary GPUSummary = BuildMetricSummary(CaptureSessionSamples, [GPUIndex](const FPerformanceCaptureSample& Sample)
		{
			return Sample.GPUFrameTimesMs[GPUIndex];
		});

		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("gpuIndex"), GPUIndex);
		Writer->WriteValue(TEXT("min"), GPUSummary.Min);
		Writer->WriteValue(TEXT("average"), GPUSummary.Average);
		Writer->WriteValue(TEXT("max"), GPUSummary.Max);
		Writer->WriteValue(TEXT("last"), GPUSummary.Last);
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->WriteArrayStart(TEXT("samples"));

	for (const FPerformanceCaptureSample& Sample : CaptureSessionSamples)
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

bool SPerformanceGraphWidget::SaveCaptureSummaryToCsv(const FString& FilePath) const
{
	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	const FMetricSummary FrameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary InputSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.InputLatencyTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

	FString CsvContent = TEXT("sample_count,event_count,duration_seconds,avg_fps,min_fps,max_fps,dominant_bottleneck,frame_avg_ms,frame_min_ms,frame_max_ms,game_avg_ms,render_avg_ms,input_avg_ms,rhi_avg_ms");
	CsvContent += LINE_TERMINATOR;
	CsvContent += FString::Printf(
		TEXT("%d,%d,%.6f,%.2f,%.2f,%.2f,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f"),
		CaptureSessionSamples.Num(),
		CaptureSessionEvents.Num(),
		CaptureSessionSamples.Last().ElapsedSeconds,
		FrameMsToFPS(FrameSummary.Average),
		FrameMsToFPS(FrameSummary.Max),
		FrameMsToFPS(FMath::Max(FrameSummary.Min, KINDA_SMALL_NUMBER)),
		*GetDominantBottleneck(CaptureSessionSamples),
		FrameSummary.Average,
		FrameSummary.Min,
		FrameSummary.Max,
		GameSummary.Average,
		RenderSummary.Average,
		InputSummary.Average,
		RHISummary.Average);

	return FFileHelper::SaveStringToFile(CsvContent, *FilePath);
}

bool SPerformanceGraphWidget::SaveCaptureSummaryToJson(const FString& FilePath) const
{
	if (CaptureSessionSamples.Num() == 0)
	{
		return false;
	}

	const FMetricSummary FrameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary InputSummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.InputLatencyTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(CaptureSessionSamples, [](const FPerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

	FString JsonContent;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonContent);

	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("captureStartedUtc"), CaptureSessionStartUtc.ToIso8601());
	Writer->WriteValue(TEXT("captureSavedUtc"), FDateTime::UtcNow().ToIso8601());
	Writer->WriteValue(TEXT("sampleCount"), CaptureSessionSamples.Num());
	Writer->WriteValue(TEXT("durationSeconds"), CaptureSessionSamples.Last().ElapsedSeconds);
	Writer->WriteValue(TEXT("eventCount"), CaptureSessionEvents.Num());
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

bool SPerformanceGraphWidget::SaveCaptureSummaryToPng(const FString& FilePath) const
{
	TArray<FPerformanceCaptureSample> Samples;
	TArray<FPerformanceCaptureEvent> Events;
	FDateTime SessionStartUtc;
	bool bIsSelectedRange = false;
	if (!BuildActiveSummaryExportData(Samples, Events, SessionStartUtc, bIsSelectedRange))
	{
		return false;
	}

	return SaveCaptureSummaryToPng(FilePath, Samples, Events, SessionStartUtc, bIsSelectedRange);
}

bool SPerformanceGraphWidget::SaveCaptureSummaryToPng(const FString& FilePath, const TArray<FPerformanceCaptureSample>& Samples, const TArray<FPerformanceCaptureEvent>& Events, const FDateTime& SessionStartUtc, bool bIsSelectedRange) const
{
	if (Samples.Num() == 0 || !FSlateApplication::IsInitialized())
	{
		return false;
	}

	const FMetricSummary FrameSummary = BuildMetricSummary(Samples, [](const FPerformanceCaptureSample& Sample) { return Sample.FrameTimeMs; });
	const FMetricSummary GameSummary = BuildMetricSummary(Samples, [](const FPerformanceCaptureSample& Sample) { return Sample.GameThreadTimeMs; });
	const FMetricSummary RenderSummary = BuildMetricSummary(Samples, [](const FPerformanceCaptureSample& Sample) { return Sample.RenderThreadTimeMs; });
	const FMetricSummary RHISummary = BuildMetricSummary(Samples, [](const FPerformanceCaptureSample& Sample) { return Sample.RHITTimeMs; });

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
				.Text(FText::FromString(bIsSelectedRange ? TEXT("Performance Session Selected Range Report") : TEXT("Performance Session Report")))
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
					*SessionStartUtc.ToString(TEXT("%Y-%m-%d %H:%M:%S UTC")),
					Samples.Last().ElapsedSeconds - Samples[0].ElapsedSeconds,
					Samples.Num(),
					Events.Num(),
					*GetDominantBottleneck(Samples))))
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
						TEXT("FPS avg %.1f | min %.1f | max %.1f\nFrame avg %.2f ms | min %.2f | max %.2f\nGame avg %.2f ms | Render avg %.2f ms | RHI avg %.2f ms"),
						FrameMsToFPS(FrameSummary.Average),
						FrameMsToFPS(FrameSummary.Max),
						FrameMsToFPS(FMath::Max(FrameSummary.Min, KINDA_SMALL_NUMBER)),
						FrameSummary.Average,
						FrameSummary.Min,
						FrameSummary.Max,
						GameSummary.Average,
						RenderSummary.Average,
						RHISummary.Average)))
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
				.Samples(&Samples)
				.Events(&Events)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				BuildSessionEventsListWidget(Events)
			]
		];

	const FVector2D DrawSize(1200.0f, Events.Num() > 0 ? 940.0f : 820.0f);
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

bool SPerformanceGraphWidget::SaveCaptureEventsToCsv(const FString& FilePath) const
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

bool SPerformanceGraphWidget::SaveCaptureEventsToJson(const FString& FilePath) const
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

void SPerformanceGraphWidget::DrawBackground(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size) const
{
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		MakePaintGeometry(AllottedGeometry),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor(0.035f, 0.04f, 0.05f, 0.98f));
}

void SPerformanceGraphWidget::DrawGrid(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float /*DisplayMaxTime*/) const
{
	const int32 NumGridLines = 5;
	const FLinearColor GridColor(0.45f, 0.50f, 0.56f, 0.18f);
	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);
	const float GraphWidth = Size.X - GraphLeftPaddingPx - GraphRightPaddingPx;

	for (int32 GridIndex = 0; GridIndex <= NumGridLines; ++GridIndex)
	{
		const float Y = GraphTopPaddingPx + GraphHeight * GridIndex / NumGridLines;
		const TArray<FVector2D> Points = { FVector2D(GraphLeftPaddingPx, Y), FVector2D(GraphLeftPaddingPx + GraphWidth, Y) };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			MakePaintGeometry(AllottedGeometry),
			Points,
			ESlateDrawEffect::None,
			GridColor,
			true,
			1.0f);
	}

	const int32 VerticalGridLines = 8;
	for (int32 GridIndex = 0; GridIndex <= VerticalGridLines; ++GridIndex)
	{
		const float X = GraphLeftPaddingPx + GraphWidth * GridIndex / VerticalGridLines;
		const TArray<FVector2D> Points = { FVector2D(X, GraphTopPaddingPx), FVector2D(X, GraphTopPaddingPx + GraphHeight) };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			MakePaintGeometry(AllottedGeometry),
			Points,
			ESlateDrawEffect::None,
			GridColor.CopyWithNewOpacity(0.08f),
			true,
			1.0f);

		if (bIsSavedSessionMode && SavedSessionSamples.Num() > 0)
		{
			const int32 StartIndex = GetSavedVisibleStartIndex();
			const int32 EndIndex = GetSavedVisibleEndIndex();
			const double StartSeconds = SavedSessionSamples.IsValidIndex(StartIndex) ? SavedSessionSamples[StartIndex].ElapsedSeconds : 0.0;
			const double EndSeconds = SavedSessionSamples.IsValidIndex(EndIndex) ? SavedSessionSamples[EndIndex].ElapsedSeconds : StartSeconds;
			const double LabelSeconds = FMath::Lerp(StartSeconds, EndSeconds, static_cast<double>(GridIndex) / static_cast<double>(VerticalGridLines));
			const float TimeLabelY = GraphTopPaddingPx + GraphHeight + 4.0f;

			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 1,
				MakePaintGeometry(AllottedGeometry, FVector2D(X - 18.0f, TimeLabelY), FVector2D(44.0f, 14.0f)),
				FString::Printf(TEXT("%.1fs"), LabelSeconds),
				FCoreStyle::Get().GetFontStyle("SmallFont"),
				ESlateDrawEffect::None,
				FLinearColor(0.75f, 0.78f, 0.82f, 0.9f));
		}
	}
}

void SPerformanceGraphWidget::DrawYAxisLabels(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float DisplayMaxTime) const
{
	const int32 NumGridLines = 5;
	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);

	for (int32 GridIndex = 0; GridIndex <= NumGridLines; ++GridIndex)
	{
		const float Alpha = 1.0f - static_cast<float>(GridIndex) / NumGridLines;
		const float ValueMs = DisplayMaxTime * Alpha;
		const float Y = GraphTopPaddingPx + GraphHeight * GridIndex / NumGridLines - 8.0f;

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			MakePaintGeometry(AllottedGeometry, FVector2D(6.0f, Y), FVector2D(40.0f, 16.0f)),
			FString::Printf(TEXT("%.0f"), ValueMs),
			FCoreStyle::Get().GetFontStyle("SmallFont"),
			ESlateDrawEffect::None,
			FLinearColor(0.75f, 0.78f, 0.82f, 0.9f));
	}
}

void SPerformanceGraphWidget::DrawLegend(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	float LegendY = 56.0f;

	for (const FName& SeriesName : SeriesDrawOrder)
	{
		const FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName);
		if (!Series)
		{
			continue;
		}

		const bool bVisible = IsSeriesVisibleBool(SeriesName);
		const FLinearColor BoxColor = bVisible ? Series->Color : FLinearColor(0.25f, 0.25f, 0.25f, 0.35f);

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			MakePaintGeometry(AllottedGeometry, FVector2D(190.0f, LegendY), FVector2D(10.0f, 10.0f)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			BoxColor);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			MakePaintGeometry(AllottedGeometry, FVector2D(60.0f, LegendY - 2.0f), FVector2D(160.0f, 16.0f)),
			SeriesName.ToString(),
			FCoreStyle::Get().GetFontStyle("SmallFont"),
			ESlateDrawEffect::None,
			FLinearColor::White);

		LegendY += 14.0f;
	}
}

void SPerformanceGraphWidget::DrawAlertZone(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float DisplayMaxTime) const
{
	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);
	const float GraphWidth = Size.X - GraphLeftPaddingPx - GraphRightPaddingPx;
	const float LowestBudgetMs = GetLowestActiveBudget();
	if (LowestBudgetMs <= 0.0f)
	{
		return;
	}

	const float ThresholdRatio = FMath::Clamp(LowestBudgetMs / DisplayMaxTime, 0.0f, 1.0f);
	const float AlertY = GraphTopPaddingPx + GraphHeight * (1.0f - ThresholdRatio);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		MakePaintGeometry(AllottedGeometry, FVector2D(GraphLeftPaddingPx, GraphTopPaddingPx), FVector2D(GraphWidth, FMath::Max(0.0f, AlertY - GraphTopPaddingPx))),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor(0.55f, 0.08f, 0.08f, 0.18f));
}

void SPerformanceGraphWidget::DrawAlertLine(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float DisplayMaxTime) const
{
	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);
	const float GraphWidth = Size.X - GraphLeftPaddingPx - GraphRightPaddingPx;
	TSet<int32> DrawnBudgetKeys;
	float LabelOffsetY = 0.0f;

	for (const FName& SeriesName : SeriesDrawOrder)
	{
		if (!IsSeriesVisibleBool(SeriesName))
		{
			continue;
		}

		const float BudgetMs = GetSeriesBudget(SeriesName);
		if (BudgetMs <= 0.0f)
		{
			continue;
		}

		const int32 BudgetKey = FMath::RoundToInt(BudgetMs * 100.0f);
		if (DrawnBudgetKeys.Contains(BudgetKey))
		{
			continue;
		}

		DrawnBudgetKeys.Add(BudgetKey);

		const FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName);
		const FLinearColor LineColor = Series ? Series->Color.CopyWithNewOpacity(0.92f) : FLinearColor(1.0f, 0.55f, 0.55f, 0.95f);
		const float ThresholdRatio = FMath::Clamp(BudgetMs / DisplayMaxTime, 0.0f, 1.0f);
		const float AlertY = GraphTopPaddingPx + GraphHeight * (1.0f - ThresholdRatio);
		const TArray<FVector2D> Points = { FVector2D(GraphLeftPaddingPx, AlertY), FVector2D(GraphLeftPaddingPx + GraphWidth, AlertY) };

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId,
			MakePaintGeometry(AllottedGeometry),
			Points,
			ESlateDrawEffect::None,
			LineColor,
			true,
			1.2f);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			MakePaintGeometry(AllottedGeometry, FVector2D(Size.X - 124.0f, AlertY - 18.0f + LabelOffsetY), FVector2D(118.0f, 16.0f)),
			FString::Printf(TEXT("%.2f ms"), BudgetMs),
			FCoreStyle::Get().GetFontStyle("SmallFont"),
			ESlateDrawEffect::None,
			LineColor);

		LabelOffsetY += 14.0f;
	}
}

void SPerformanceGraphWidget::DrawEventMarkers(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size) const
{
	const TArray<FPerformanceCaptureEvent>& Events = bIsSavedSessionMode ? SavedSessionEvents : CaptureSessionEvents;
	const TArray<FPerformanceCaptureSample>& Samples = bIsSavedSessionMode ? SavedSessionSamples : CaptureSessionSamples;
	if (Events.Num() == 0 || Samples.Num() == 0)
	{
		return;
	}

	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);
	const float GraphWidth = Size.X - GraphLeftPaddingPx - GraphRightPaddingPx;
	const float GraphBottom = GraphTopPaddingPx + GraphHeight;
	const int32 FirstVisibleSampleIndex = bIsSavedSessionMode ? GetSavedVisibleStartIndex() : FMath::Max(0, Samples.Num() - MaxSamples);
	const int32 LastVisibleSampleIndex = bIsSavedSessionMode ? GetSavedVisibleEndIndex() : Samples.Num() - 1;
	const double VisibleStartSeconds = Samples[FirstVisibleSampleIndex].ElapsedSeconds;
	const double VisibleEndSeconds = Samples[LastVisibleSampleIndex].ElapsedSeconds;
	const double VisibleDurationSeconds = FMath::Max(VisibleEndSeconds - VisibleStartSeconds, KINDA_SMALL_NUMBER);

	int32 DrawnLabels = 0;
	float LastLabelX = -1000.0f;

	for (const FPerformanceCaptureEvent& Event : Events)
	{
		if (Event.ElapsedSeconds < VisibleStartSeconds || Event.ElapsedSeconds > VisibleEndSeconds)
		{
			continue;
		}

		const float NormalizedTime = FMath::Clamp(static_cast<float>((Event.ElapsedSeconds - VisibleStartSeconds) / VisibleDurationSeconds), 0.0f, 1.0f);
		const float X = GraphLeftPaddingPx + GraphWidth * NormalizedTime;
		const FLinearColor MarkerColor = Event.Color.A > 0.0f ? Event.Color : GetSeverityFallbackColor(Event.Severity);
		const TArray<FVector2D> MarkerPoints = { FVector2D(X, GraphTopPaddingPx), FVector2D(X, GraphBottom) };
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		MakePaintGeometry(AllottedGeometry),
		MarkerPoints,
		ESlateDrawEffect::None,
		MarkerColor.CopyWithNewOpacity(0.88f),
			true,
			1.2f);

		if (DrawnLabels < 10 && FMath::Abs(X - LastLabelX) > 72.0f)
		{
			const FString Label = bIsSavedSessionMode ? (Event.Category.IsEmpty() ? TEXT("Event") : Event.Category) : Event.Name.ToString();
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 1,
				MakePaintGeometry(AllottedGeometry, FVector2D(FMath::Min(X + 4.0f, GraphLeftPaddingPx + GraphWidth - 110.0f), GraphTopPaddingPx + 4.0f), FVector2D(108.0f, 16.0f)),
				Label,
				FCoreStyle::Get().GetFontStyle("SmallFont"),
				ESlateDrawEffect::None,
				MarkerColor);

			LastLabelX = X;
			++DrawnLabels;
		}
	}
}

void SPerformanceGraphWidget::DrawGraphLines(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float XStep, float DisplayMaxTime) const
{
	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);
	const float GraphBottom = GraphTopPaddingPx + GraphHeight;

	if (bIsSavedSessionMode)
	{
		const int32 StartIndex = GetSavedVisibleStartIndex();
		const int32 EndIndex = GetSavedVisibleEndIndex();
		if (SavedSessionSamples.Num() == 0 || StartIndex > EndIndex)
		{
			return;
		}

		for (const FName& SeriesName : SeriesDrawOrder)
		{
			if (!IsSeriesVisibleBool(SeriesName))
			{
				continue;
			}

			const FPerformanceGraphSeries* LiveSeries = GraphSeries.Find(SeriesName);
			const FLinearColor SeriesColor = LiveSeries ? LiveSeries->Color : FLinearColor::White;
			TArray<FVector2D> Points;
			Points.Reserve(EndIndex - StartIndex + 1);

			for (int32 SampleIndex = StartIndex; SampleIndex <= EndIndex; ++SampleIndex)
			{
				const float X = GraphLeftPaddingPx + (SampleIndex - StartIndex) * XStep;
				const float ClampedValue = FMath::Clamp(GetSavedSeriesValue(SavedSessionSamples[SampleIndex], SeriesName), 0.0f, DisplayMaxTime);
				const float Y = GraphBottom - (ClampedValue / DisplayMaxTime) * GraphHeight;
				Points.Add(FVector2D(X, Y));
			}

			for (int32 PointIndex = 1; PointIndex < Points.Num(); ++PointIndex)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					LayerId,
					MakePaintGeometry(AllottedGeometry),
					{ Points[PointIndex - 1], Points[PointIndex] },
					ESlateDrawEffect::PreMultipliedAlpha,
					SeriesColor,
					true,
					2.0f);
			}

			for (int32 SampleIndex = StartIndex; SampleIndex <= EndIndex; ++SampleIndex)
			{
				const float SampleValue = GetSavedSeriesValue(SavedSessionSamples[SampleIndex], SeriesName);
				if (!IsSeriesThresholdExceeded(SeriesName, SampleValue))
				{
					continue;
				}

				const float X = GraphLeftPaddingPx + (SampleIndex - StartIndex) * XStep;
				const float ClampedValue = FMath::Clamp(SampleValue, 0.0f, DisplayMaxTime);
				const float Y = GraphBottom - (ClampedValue / DisplayMaxTime) * GraphHeight;

				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 1,
					MakePaintGeometry(AllottedGeometry, FVector2D(X - 2.0f, Y - 2.0f), FVector2D(4.0f, 4.0f)),
					FCoreStyle::Get().GetBrush("WhiteBrush"),
					ESlateDrawEffect::None,
					FLinearColor(1.0f, 0.30f, 0.30f, 0.95f));
			}
		}

		return;
	}

	for (const FName& SeriesName : SeriesDrawOrder)
	{
		if (!IsSeriesVisibleBool(SeriesName))
		{
			continue;
		}

		const FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName);
		if (!Series)
		{
			continue;
		}

		TArray<FVector2D> Points;
		Points.Reserve(Series->Samples.Num());

		for (int32 SampleIndex = 0; SampleIndex < Series->Samples.Num(); ++SampleIndex)
		{
			const float X = GraphLeftPaddingPx + SampleIndex * XStep;
			const float ClampedValue = FMath::Clamp(Series->Samples[SampleIndex].TimeMs, 0.0f, DisplayMaxTime);
			const float YRatio = ClampedValue / DisplayMaxTime;
			const float Y = GraphBottom - YRatio * GraphHeight;
			Points.Add(FVector2D(X, Y));
		}

		for (int32 PointIndex = 1; PointIndex < Points.Num(); ++PointIndex)
		{
			const TArray<FVector2D> SegmentPoints = { Points[PointIndex - 1], Points[PointIndex] };
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				MakePaintGeometry(AllottedGeometry),
				SegmentPoints,
				ESlateDrawEffect::PreMultipliedAlpha,
				Series->Color,
				true,
				2.0f);
		}

		for (int32 SampleIndex = 0; SampleIndex < Series->Samples.Num(); ++SampleIndex)
		{
			const float SampleValue = Series->Samples[SampleIndex].TimeMs;
			if (!IsSeriesThresholdExceeded(SeriesName, SampleValue))
			{
				continue;
			}

			const float X = GraphLeftPaddingPx + SampleIndex * XStep;
			const float ClampedValue = FMath::Clamp(SampleValue, 0.0f, DisplayMaxTime);
			const float YRatio = ClampedValue / DisplayMaxTime;
			const float Y = GraphBottom - YRatio * GraphHeight;

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 1,
				MakePaintGeometry(AllottedGeometry, FVector2D(X - 2.0f, Y - 2.0f), FVector2D(4.0f, 4.0f)),
				FCoreStyle::Get().GetBrush("WhiteBrush"),
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 0.30f, 0.30f, 0.95f));
		}
	}
}

void SPerformanceGraphWidget::DrawSelectedRangeOverlay(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float XStep) const
{
	if (!bIsSavedSessionMode || !HasSelectedRange())
	{
		return;
	}

	const int32 VisibleStartIndex = GetSavedVisibleStartIndex();
	const int32 VisibleEndIndex = GetSavedVisibleEndIndex();
	if (VisibleStartIndex == INDEX_NONE || VisibleEndIndex == INDEX_NONE || SelectedRangeEndIndex < VisibleStartIndex || SelectedRangeStartIndex > VisibleEndIndex)
	{
		return;
	}

	const int32 ClampedStartIndex = FMath::Max(SelectedRangeStartIndex, VisibleStartIndex);
	const int32 ClampedEndIndex = FMath::Min(SelectedRangeEndIndex, VisibleEndIndex);
	const int32 RelativeStartIndex = ClampedStartIndex - VisibleStartIndex;
	const int32 RelativeEndIndex = ClampedEndIndex - VisibleStartIndex;
	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (SavedOverviewGapPx + SavedOverviewHeightPx);
	const float SelectionLeft = GraphLeftPaddingPx + RelativeStartIndex * XStep;
	const float SelectionRight = GraphLeftPaddingPx + RelativeEndIndex * XStep;
	const float SelectionWidth = FMath::Max(SelectionRight - SelectionLeft, 2.0f);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		MakePaintGeometry(AllottedGeometry, FVector2D(SelectionLeft, GraphTopPaddingPx), FVector2D(SelectionWidth, GraphHeight)),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor(0.45f, 0.72f, 1.0f, 0.14f));

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId + 1,
		MakePaintGeometry(AllottedGeometry),
		{
			FVector2D(SelectionLeft, GraphTopPaddingPx),
			FVector2D(SelectionLeft, GraphTopPaddingPx + GraphHeight),
			FVector2D(SelectionLeft + SelectionWidth, GraphTopPaddingPx + GraphHeight),
			FVector2D(SelectionLeft + SelectionWidth, GraphTopPaddingPx),
			FVector2D(SelectionLeft, GraphTopPaddingPx)
		},
		ESlateDrawEffect::None,
		FLinearColor(0.65f, 0.82f, 1.0f, 0.85f),
		true,
		1.2f);
}

void SPerformanceGraphWidget::DrawHoverLine(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size) const
{
	if (bIsSavedSessionMode && IsPointInSavedOverview(HoverPosition, Size))
	{
		return;
	}

	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);
	const float ClampedX = FMath::Clamp(HoverPosition.X, GraphLeftPaddingPx, Size.X - GraphRightPaddingPx);
	const TArray<FVector2D> Points = { FVector2D(ClampedX, GraphTopPaddingPx), FVector2D(ClampedX, GraphTopPaddingPx + GraphHeight) };

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId,
		MakePaintGeometry(AllottedGeometry),
		Points,
		ESlateDrawEffect::None,
		FLinearColor(1.0f, 1.0f, 1.0f, 0.18f),
		true,
		1.0f);
}

void SPerformanceGraphWidget::DrawHoverTooltip(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Size, float XStep, float /*DisplayMaxTime*/) const
{
	if (bIsSavedSessionMode && IsPointInSavedOverview(HoverPosition, Size))
	{
		return;
	}

	const float GraphHeight = Size.Y - GraphTopPaddingPx - GraphBottomPaddingPx - (bIsSavedSessionMode ? (SavedOverviewGapPx + SavedOverviewHeightPx) : 0.0f);
	const int32 VisibleSampleCount = bIsSavedSessionMode ? GetSavedVisibleSampleCount() : MaxSamples;
	const int32 HoverIndex = FMath::Clamp(FMath::RoundToInt((HoverPosition.X - GraphLeftPaddingPx) / XStep), 0, FMath::Max(VisibleSampleCount - 1, 0));
	const float GraphWidth = Size.X - GraphLeftPaddingPx - GraphRightPaddingPx;
	const TArray<FPerformanceCaptureSample>& Samples = bIsSavedSessionMode ? SavedSessionSamples : CaptureSessionSamples;
	const TArray<FPerformanceCaptureEvent>& Events = bIsSavedSessionMode ? SavedSessionEvents : CaptureSessionEvents;
	const int32 FirstVisibleSampleIndex = bIsSavedSessionMode ? GetSavedVisibleStartIndex() : (Samples.Num() > 0 ? FMath::Max(0, Samples.Num() - MaxSamples) : 0);
	const int32 LastVisibleSampleIndex = bIsSavedSessionMode ? GetSavedVisibleEndIndex() : (Samples.Num() > 0 ? Samples.Num() - 1 : INDEX_NONE);
	const double VisibleStartSeconds = Samples.IsValidIndex(FirstVisibleSampleIndex) ? Samples[FirstVisibleSampleIndex].ElapsedSeconds : 0.0;
	const double VisibleEndSeconds = Samples.IsValidIndex(LastVisibleSampleIndex) ? Samples[LastVisibleSampleIndex].ElapsedSeconds : 0.0;
	const double VisibleDurationSeconds = FMath::Max(VisibleEndSeconds - VisibleStartSeconds, KINDA_SMALL_NUMBER);
	TArray<TPair<FName, float>> HoverValues;
	TArray<const FPerformanceCaptureEvent*> HoverEvents;

	if (bIsSavedSessionMode && Samples.IsValidIndex(FirstVisibleSampleIndex + HoverIndex))
	{
		const FPerformanceCaptureSample& Sample = Samples[FirstVisibleSampleIndex + HoverIndex];
		for (const FName& SeriesName : SeriesDrawOrder)
		{
			if (!IsSeriesVisibleBool(SeriesName))
			{
				continue;
			}

			HoverValues.Emplace(SeriesName, GetSavedSeriesValue(Sample, SeriesName));
		}
	}
	else
	{
		for (const FName& SeriesName : SeriesDrawOrder)
		{
			if (!IsSeriesVisibleBool(SeriesName))
			{
				continue;
			}

			const FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName);
			if (Series && Series->Samples.IsValidIndex(HoverIndex))
			{
				HoverValues.Emplace(SeriesName, Series->Samples[HoverIndex].TimeMs);
			}
		}
	}

	for (const FPerformanceCaptureEvent& Event : Events)
	{
		if (Event.ElapsedSeconds < VisibleStartSeconds || Event.ElapsedSeconds > VisibleEndSeconds)
		{
			continue;
		}

		const float EventX = GraphLeftPaddingPx + GraphWidth * FMath::Clamp(static_cast<float>((Event.ElapsedSeconds - VisibleStartSeconds) / VisibleDurationSeconds), 0.0f, 1.0f);
		if (FMath::Abs(EventX - HoverPosition.X) <= 6.0f)
		{
			HoverEvents.Add(&Event);
		}
	}

	if (HoverValues.Num() == 0 && HoverEvents.Num() == 0)
	{
		return;
	}

	HoverValues.Sort([](const TPair<FName, float>& A, const TPair<FName, float>& B)
	{
		return A.Value > B.Value;
	});

	const int32 MaxMetricLinesToShow = FMath::Min(5, HoverValues.Num());
	const int32 MaxEventLinesToShow = FMath::Min(3, HoverEvents.Num());
	const FVector2D TooltipSize(248.0f, 20.0f + MaxMetricLinesToShow * 16.0f + MaxEventLinesToShow * 16.0f);
	FVector2D TooltipPos = HoverPosition + FVector2D(12.0f, -10.0f);
	TooltipPos.X = FMath::Min(TooltipPos.X, Size.X - TooltipSize.X - 6.0f);
	TooltipPos.Y = FMath::Clamp(TooltipPos.Y, GraphTopPaddingPx, GraphTopPaddingPx + GraphHeight - TooltipSize.Y);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		MakePaintGeometry(AllottedGeometry, TooltipPos, TooltipSize),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor(0.02f, 0.03f, 0.04f, 0.92f));

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 1,
		MakePaintGeometry(AllottedGeometry, TooltipPos + FVector2D(8.0f, 4.0f), FVector2D(TooltipSize.X - 12.0f, 14.0f)),
		bIsSavedSessionMode && Samples.IsValidIndex(FirstVisibleSampleIndex + HoverIndex)
			? FString::Printf(TEXT("Sample %d | %.2fs"), FirstVisibleSampleIndex + HoverIndex, Samples[FirstVisibleSampleIndex + HoverIndex].ElapsedSeconds)
			: FString::Printf(TEXT("Sample %d"), HoverIndex),
		FCoreStyle::Get().GetFontStyle("SmallFont"),
		ESlateDrawEffect::None,
		FLinearColor(0.78f, 0.82f, 0.86f, 0.95f));

	for (int32 ValueIndex = 0; ValueIndex < MaxMetricLinesToShow; ++ValueIndex)
	{
		const TPair<FName, float>& Entry = HoverValues[ValueIndex];
		const FVector2D RowPos = TooltipPos + FVector2D(8.0f, 22.0f + ValueIndex * 16.0f);
		const float BudgetMs = GetSeriesBudget(Entry.Key);
		const FString MetricText = BudgetMs > 0.0f
			? FString::Printf(TEXT("%s: %.2f / %.2f ms"), *Entry.Key.ToString(), Entry.Value, BudgetMs)
			: FString::Printf(TEXT("%s: %.2f ms"), *Entry.Key.ToString(), Entry.Value);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			MakePaintGeometry(AllottedGeometry, RowPos, FVector2D(TooltipSize.X - 12.0f, 14.0f)),
			MetricText,
			FCoreStyle::Get().GetFontStyle("SmallFont"),
			ESlateDrawEffect::None,
			IsSeriesThresholdExceeded(Entry.Key, Entry.Value) ? FLinearColor(1.0f, 0.55f, 0.55f, 0.98f) : FLinearColor::White);
	}

	for (int32 EventIndex = 0; EventIndex < MaxEventLinesToShow; ++EventIndex)
	{
		const FPerformanceCaptureEvent& Event = *HoverEvents[EventIndex];
		const FVector2D RowPos = TooltipPos + FVector2D(8.0f, 22.0f + (MaxMetricLinesToShow + EventIndex) * 16.0f);
		const FLinearColor MarkerColor = Event.Color.A > 0.0f ? Event.Color : GetSeverityFallbackColor(Event.Severity);

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			MakePaintGeometry(AllottedGeometry, RowPos, FVector2D(TooltipSize.X - 12.0f, 14.0f)),
			FString::Printf(TEXT("[%s] %s"), *Event.Category, *Event.Name.ToString()),
			FCoreStyle::Get().GetFontStyle("SmallFont"),
			ESlateDrawEffect::None,
			MarkerColor);
	}
}

void SPerformanceGraphWidget::DrawTable(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, const FVector2D& Size, int32 LayerId) const
{
	const FVector2D PanelPos(Size.X - 218.0f, 12.0f);
	const FVector2D PanelSize(206.0f, 106.0f);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		MakePaintGeometry(AllottedGeometry, PanelPos, PanelSize),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor(0.02f, 0.03f, 0.04f, 0.82f));

	auto DrawRow = [&](int32 RowIndex, const TCHAR* Label, FName SeriesName, float FallbackCurrentValue, float FallbackMaxValue, const FLinearColor& Color)
	{
		FPerformanceSeriesStats Stats;
		const bool bHasStats = GetSeriesStats(SeriesName, Stats);
		const float CurrentValue = bHasStats ? Stats.CurrentMs : FallbackCurrentValue;
		const float MaxValue = bHasStats ? Stats.MaxMs : FallbackMaxValue;
		const float BudgetMs = GetSeriesBudget(SeriesName);
		const FVector2D RowPos = PanelPos + FVector2D(8.0f, 8.0f + RowIndex * 18.0f);
		const FLinearColor ValueColor = IsSeriesThresholdExceeded(SeriesName, CurrentValue)
			? FLinearColor(1.0f, 0.55f, 0.55f, 0.98f)
			: Color;

		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			MakePaintGeometry(AllottedGeometry, RowPos, FVector2D(188.0f, 14.0f)),
			FString::Printf(TEXT("%s %.2f ms"), Label, CurrentValue),
			FCoreStyle::Get().GetFontStyle("SmallFont"),
			ESlateDrawEffect::None,
			ValueColor);

		if (BudgetMs > 0.0f)
		{
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 1,
				MakePaintGeometry(AllottedGeometry, RowPos + FVector2D(92.0f, 0.0f), FVector2D(70.0f, 14.0f)),
				FString::Printf(TEXT("b %.2f"), BudgetMs),
				FCoreStyle::Get().GetFontStyle("SmallFont"),
				ESlateDrawEffect::None,
				FLinearColor(0.82f, 0.84f, 0.86f, 0.9f));
		}

		if (bShowUnitMaxTimes)
		{
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 1,
				MakePaintGeometry(AllottedGeometry, RowPos + FVector2D(144.0f, 0.0f), FVector2D(52.0f, 14.0f)),
				FString::Printf(TEXT("max %.2f"), MaxValue),
				FCoreStyle::Get().GetFontStyle("SmallFont"),
				ESlateDrawEffect::None,
				FLinearColor(0.82f, 0.84f, 0.86f, 0.9f));
		}
	};

	int32 RowIndex = 0;
	if (bShowFrameRateTime)
	{
		DrawRow(RowIndex++, TEXT("Frame"), TEXT("FrameTime"), CurrentStats.FrameTime, MaxFrameTime, FLinearColor(0.55f, 1.0f, 0.55f));
	}
	if (bShowGameThreadTime)
	{
		DrawRow(RowIndex++, TEXT("Game "), TEXT("GameThread"), CurrentStats.GameThreadTime, MaxGameThreadTime, FLinearColor(1.0f, 0.50f, 0.50f));
	}
	if (bShowRenderThreadTime)
	{
		DrawRow(RowIndex++, TEXT("Draw "), TEXT("RenderThread"), CurrentStats.RenderThreadTime, MaxRenderThreadTime, FLinearColor(0.55f, 0.70f, 1.0f));
	}
	if (bShowRHITTime && IsRunningRHIInSeparateThread())
	{
		DrawRow(RowIndex, TEXT("RHIT "), TEXT("RHITTime"), CurrentStats.RHITTime, MaxRHITTime, FLinearColor(1.0f, 0.55f, 1.0f));
	}

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 1,
		MakePaintGeometry(AllottedGeometry, PanelPos + FVector2D(8.0f, PanelSize.Y - 18.0f), FVector2D(150.0f, 14.0f)),
		bIsSavedSessionMode ? FString::Printf(TEXT("Saved events %d"), SavedSessionEvents.Num()) : FString::Printf(TEXT("Events %d"), CaptureSessionEvents.Num()),
		FCoreStyle::Get().GetFontStyle("SmallFont"),
		ESlateDrawEffect::None,
		FLinearColor(0.95f, 0.75f, 0.18f, 0.95f));
}

bool SPerformanceGraphWidget::UpdatePerformanceData()
{
	if (!GEngine)
	{
		DataSourceText = FText::FromString(TEXT("Source: Engine unavailable"));
		return false;
	}

	bool bUpdated = false;
	DataSourceText = FText::FromString(TEXT("Source: No live stat unit data"));

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType == EWorldType::Game || WorldContext.WorldType == EWorldType::PIE)
		{
			if (UWorld* World = WorldContext.World())
			{
				if (!bStatUnitEnabled && World->GetGameViewport())
				{
					GEngine->SetEngineStat(World, World->GetGameViewport(), TEXT("Unit"), true);
					bStatUnitEnabled = true;
				}

				if (World->GetGameViewport() && World->GetGameViewport()->Viewport)
				{
					if (const FStatUnitData* StatUnitData = World->GetGameViewport()->GetStatUnitData())
					{
						UpdateStatsFromData(*StatUnitData);
						bUpdated = true;
						DataSourceText = FText::FromString(WorldContext.WorldType == EWorldType::PIE ? TEXT("Source: PIE") : TEXT("Source: Game"));
					}
				}
				break;
			}
		}
	}

	if (!bUpdated && GEditor)
	{
		if (const FViewport* ActiveViewport = GEditor->GetActiveViewport())
		{
			if (ActiveViewport->GetClient())
			{
				if (const FStatUnitData* StatUnitData = ActiveViewport->GetClient()->GetStatUnitData())
				{
					UpdateStatsFromData(*StatUnitData);
					bUpdated = true;
					DataSourceText = FText::FromString(TEXT("Source: Editor Viewport"));
				}
			}
		}
	}

	if (!bUpdated)
	{
		return false;
	}

	MaxFrameTime = FMath::Max(MaxFrameTime, CurrentStats.FrameTime);
	MaxGameThreadTime = FMath::Max(MaxGameThreadTime, CurrentStats.GameThreadTime);
	MaxRenderThreadTime = FMath::Max(MaxRenderThreadTime, CurrentStats.RenderThreadTime);
	MaxRHITTime = FMath::Max(MaxRHITTime, CurrentStats.RHITTime);
	return true;
}

void SPerformanceGraphWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	LastKnownGraphSize = AllottedGeometry.GetLocalSize();

	if (bIsPaused || bIsSavedSessionMode)
	{
		return;
	}

	SyncRuntimeEvents();
	if (!UpdatePerformanceData())
	{
		return;
	}

	AddSample(TEXT("FrameTime"), CurrentStats.FrameTime);
	AddSample(TEXT("GameThread"), CurrentStats.GameThreadTime);
	AddSample(TEXT("RenderThread"), CurrentStats.RenderThreadTime);
	AddSample(TEXT("InputTime"), CurrentStats.InputLatencyTime);
	AddSample(TEXT("RHITTime"), CurrentStats.RHITTime);

	for (int32 GPUIndex = 0; GPUIndex < MAX_NUM_GPUS; ++GPUIndex)
	{
		AddSample(FName(*FString::Printf(TEXT("GPU %d FrameTime"), GPUIndex)), CurrentStats.GPUFrameTime[GPUIndex]);
	}

	AppendCaptureSample();

	float MaxObserved = 1.0f;
	for (const FName& SeriesName : SeriesDrawOrder)
	{
		if (!IsSeriesVisibleBool(SeriesName))
		{
			continue;
		}

		const FPerformanceGraphSeries* Series = GraphSeries.Find(SeriesName);
		if (!Series)
		{
			continue;
		}

		for (const FPerformanceGraphSample& Sample : Series->Samples)
		{
			MaxObserved = FMath::Max(MaxObserved, Sample.TimeMs);
		}
	}

	SmoothedMaxObservedTime = FMath::FInterpTo(SmoothedMaxObservedTime, MaxObserved, InDeltaTime, 8.0f);
}

FReply SPerformanceGraphWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	if (bIsSavedSessionMode && MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && HasSelectedRange() && IsPointInMainGraph(LocalPosition, MyGeometry.GetLocalSize()))
	{
		ClearSelectedRange();
		return FReply::Handled();
	}

	if (bIsSavedSessionMode && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (MouseEvent.IsShiftDown() && IsPointInMainGraph(LocalPosition, MyGeometry.GetLocalSize()) && !IsPointInSavedOverview(LocalPosition, MyGeometry.GetLocalSize()))
		{
			bIsSelectingSavedRange = true;
			SavedRangeSelectionAnchor = LocalPosition;
			UpdateSelectedRangeFromGraphPositions(LocalPosition, LocalPosition, MyGeometry.GetLocalSize());
			return FReply::Handled().CaptureMouse(AsShared());
		}

		if (IsPointInSavedOverview(LocalPosition, MyGeometry.GetLocalSize()))
		{
			bIsDraggingSavedOverview = true;
			SavedSessionDragAnchor = LocalPosition;
			const float OverviewWidth = FMath::Max(1.0f, MyGeometry.GetLocalSize().X - GraphLeftPaddingPx - GraphRightPaddingPx);
			CenterSavedSessionOnNormalizedPosition((LocalPosition.X - GraphLeftPaddingPx) / OverviewWidth);
		}
		else
		{
			SavedSessionDragAnchor = LocalPosition;
			bIsPanningSavedSession = true;
		}

		return FReply::Handled().CaptureMouse(AsShared());
	}

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SPerformanceGraphWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsSavedSessionMode && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
	{
		if (bIsSelectingSavedRange)
		{
			UpdateSelectedRangeFromGraphPositions(SavedRangeSelectionAnchor, MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()), MyGeometry.GetLocalSize());
		}

		bIsSelectingSavedRange = false;
		bIsPanningSavedSession = false;
		bIsDraggingSavedOverview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SPerformanceGraphWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (bIsSavedSessionMode && bIsSelectingSavedRange)
	{
		UpdateSelectedRangeFromGraphPositions(SavedRangeSelectionAnchor, LocalPosition, MyGeometry.GetLocalSize());
	}
	else if (bIsSavedSessionMode && bIsDraggingSavedOverview)
	{
		const float OverviewWidth = FMath::Max(1.0f, MyGeometry.GetLocalSize().X - GraphLeftPaddingPx - GraphRightPaddingPx);
		CenterSavedSessionOnNormalizedPosition((LocalPosition.X - GraphLeftPaddingPx) / OverviewWidth);
	}
	else if (bIsSavedSessionMode && bIsPanningSavedSession && SavedSessionSamples.Num() > 0)
	{
		const float GraphWidth = FMath::Max(1.0f, MyGeometry.GetLocalSize().X - GraphLeftPaddingPx - GraphRightPaddingPx);
		const float PixelsPerSample = GraphWidth / FMath::Max(GetSavedVisibleSampleCount() - 1, 1);
		const float DeltaX = LocalPosition.X - SavedSessionDragAnchor.X;
		if (FMath::Abs(DeltaX) >= PixelsPerSample)
		{
			PanSavedSession(-FMath::RoundToInt(DeltaX / PixelsPerSample));
			SavedSessionDragAnchor = LocalPosition;
		}
	}

	HoverPosition = LocalPosition;
	bIsHovering = true;
	return (bIsSelectingSavedRange || bIsPanningSavedSession || bIsDraggingSavedOverview) ? FReply::Handled() : FReply::Unhandled();
}

FReply SPerformanceGraphWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bIsSavedSessionMode)
	{
		return SCompoundWidget::OnMouseWheel(MyGeometry, MouseEvent);
	}

	ZoomSavedSession(MouseEvent.GetWheelDelta() > 0.0f ? 0.75f : 1.25f);
	HoverPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

void SPerformanceGraphWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);
	bIsHovering = false;
	bIsSelectingSavedRange = false;
	bIsPanningSavedSession = false;
	bIsDraggingSavedOverview = false;
}

int32 SPerformanceGraphWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float GraphWidth = FMath::Max(1.0f, Size.X - GraphLeftPaddingPx - GraphRightPaddingPx);
	const int32 SampleSlots = bIsSavedSessionMode ? GetSavedVisibleSampleCount() : MaxSamples;
	const float XStep = GraphWidth / FMath::Max(SampleSlots - 1, 1);
	const float DisplayMaxTime = GetDisplayMaxTime();

	DrawBackground(AllottedGeometry, OutDrawElements, LayerId, Size);
	DrawAlertZone(AllottedGeometry, OutDrawElements, LayerId + 1, Size, DisplayMaxTime);
	DrawGrid(AllottedGeometry, OutDrawElements, LayerId + 2, Size, DisplayMaxTime);
	DrawYAxisLabels(AllottedGeometry, OutDrawElements, LayerId + 3, Size, DisplayMaxTime);
	DrawAlertLine(AllottedGeometry, OutDrawElements, LayerId + 4, Size, DisplayMaxTime);
	DrawEventMarkers(AllottedGeometry, OutDrawElements, LayerId + 5, Size);
	DrawGraphLines(AllottedGeometry, OutDrawElements, LayerId + 7, Size, XStep, DisplayMaxTime);
	DrawSelectedRangeOverlay(AllottedGeometry, OutDrawElements, LayerId + 8, Size, XStep);
	if (bIsSavedSessionMode && SavedSessionSamples.Num() > 1)
	{
		const float OverviewLeft = GraphLeftPaddingPx;
		const float OverviewTop = Size.Y - GraphBottomPaddingPx - SavedOverviewHeightPx;
		const float OverviewWidth = FMath::Max(1.0f, Size.X - GraphLeftPaddingPx - GraphRightPaddingPx);
		const float OverviewHeight = SavedOverviewHeightPx;
		const float OverviewBottom = OverviewTop + OverviewHeight;
		float OverviewMaxTime = 1.0f;

		for (const FPerformanceCaptureSample& Sample : SavedSessionSamples)
		{
			OverviewMaxTime = FMath::Max(OverviewMaxTime, Sample.FrameTimeMs);
			OverviewMaxTime = FMath::Max(OverviewMaxTime, Sample.GameThreadTimeMs);
			OverviewMaxTime = FMath::Max(OverviewMaxTime, Sample.RenderThreadTimeMs);
			OverviewMaxTime = FMath::Max(OverviewMaxTime, Sample.RHITTimeMs);
		}

		OverviewMaxTime = FMath::Max(OverviewMaxTime * 1.15f, MinGraphScaleMs);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 9,
			MakePaintGeometry(AllottedGeometry, FVector2D(OverviewLeft, OverviewTop), FVector2D(OverviewWidth, OverviewHeight)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.06f, 0.07f, 0.09f, 0.96f));

		auto DrawOverviewSeries = [&](FName SeriesName)
		{
			if (!IsSeriesVisibleBool(SeriesName))
			{
				return;
			}

			const FPerformanceGraphSeries* LiveSeries = GraphSeries.Find(SeriesName);
			const FLinearColor SeriesColor = LiveSeries ? LiveSeries->Color.CopyWithNewOpacity(0.55f) : FLinearColor::White;
			TArray<FVector2D> Points;
			Points.Reserve(SavedSessionSamples.Num());
			const float OverviewXStep = OverviewWidth / FMath::Max(SavedSessionSamples.Num() - 1, 1);
			for (int32 SampleIndex = 0; SampleIndex < SavedSessionSamples.Num(); ++SampleIndex)
			{
				const float X = OverviewLeft + SampleIndex * OverviewXStep;
				const float Y = OverviewBottom - (FMath::Clamp(GetSavedSeriesValue(SavedSessionSamples[SampleIndex], SeriesName), 0.0f, OverviewMaxTime) / OverviewMaxTime) * (OverviewHeight - 6.0f);
				Points.Add(FVector2D(X, Y));
			}

			for (int32 PointIndex = 1; PointIndex < Points.Num(); ++PointIndex)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					LayerId + 10,
					MakePaintGeometry(AllottedGeometry),
					{ Points[PointIndex - 1], Points[PointIndex] },
					ESlateDrawEffect::PreMultipliedAlpha,
					SeriesColor,
					true,
					1.0f);
			}
		};

		DrawOverviewSeries(TEXT("FrameTime"));
		DrawOverviewSeries(TEXT("GameThread"));
		DrawOverviewSeries(TEXT("RenderThread"));
		DrawOverviewSeries(TEXT("RHITTime"));

		const float StartAlpha = static_cast<float>(GetSavedVisibleStartIndex()) / FMath::Max(SavedSessionSamples.Num(), 1);
		const float EndAlpha = static_cast<float>(GetSavedVisibleEndIndex() + 1) / FMath::Max(SavedSessionSamples.Num(), 1);
		const float WindowLeft = OverviewLeft + OverviewWidth * StartAlpha;
		const float WindowRight = OverviewLeft + OverviewWidth * EndAlpha;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 11,
			MakePaintGeometry(AllottedGeometry, FVector2D(OverviewLeft, OverviewTop), FVector2D(WindowLeft - OverviewLeft, OverviewHeight)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.01f, 0.01f, 0.02f, 0.45f));
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 11,
			MakePaintGeometry(AllottedGeometry, FVector2D(WindowRight, OverviewTop), FVector2D(OverviewLeft + OverviewWidth - WindowRight, OverviewHeight)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.01f, 0.01f, 0.02f, 0.45f));
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 12,
			MakePaintGeometry(AllottedGeometry, FVector2D(WindowLeft, OverviewTop), FVector2D(FMath::Max(WindowRight - WindowLeft, 2.0f), OverviewHeight)),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.85f, 0.88f, 0.95f, 0.10f));
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 13,
			MakePaintGeometry(AllottedGeometry),
			{ FVector2D(WindowLeft, OverviewTop), FVector2D(WindowLeft, OverviewBottom), FVector2D(WindowRight, OverviewBottom), FVector2D(WindowRight, OverviewTop), FVector2D(WindowLeft, OverviewTop) },
			ESlateDrawEffect::None,
			FLinearColor(0.95f, 0.97f, 1.0f, 0.85f),
			true,
			1.2f);
	}
	if (bIsHovering)
	{
		DrawHoverLine(AllottedGeometry, OutDrawElements, LayerId + 8, Size);
		DrawHoverTooltip(AllottedGeometry, OutDrawElements, LayerId + 9, Size, XStep, DisplayMaxTime);
	}
	DrawLegend(AllottedGeometry, OutDrawElements, LayerId + 10);
	DrawTable(AllottedGeometry, OutDrawElements, Size, LayerId + 11);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 20, InWidgetStyle, bParentEnabled);
}
