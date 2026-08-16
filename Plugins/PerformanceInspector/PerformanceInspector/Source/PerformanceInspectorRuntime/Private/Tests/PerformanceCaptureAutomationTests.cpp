#include "PerformanceCaptureSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

namespace
{
FRuntimePerformanceCaptureSample MakeSample(double ElapsedSeconds, float FrameMs, float GameMs, float RenderMs, float InputMs, float RHIMs)
{
	FRuntimePerformanceCaptureSample Sample;
	Sample.TimestampUtc = FDateTime::UtcNow();
	Sample.ElapsedSeconds = ElapsedSeconds;
	Sample.FrameTimeMs = FrameMs;
	Sample.GameThreadTimeMs = GameMs;
	Sample.RenderThreadTimeMs = RenderMs;
	Sample.InputLatencyTimeMs = InputMs;
	Sample.RHITTimeMs = RHIMs;
	return Sample;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerformanceInspectorBuildReportPopulatesMetricsTest,
	"PerformanceInspector.Automation.BuildReport.PopulatesMetrics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPerformanceInspectorBuildReportPopulatesMetricsTest::RunTest(const FString& Parameters)
{
	TArray<FRuntimePerformanceCaptureSample> Samples;
	Samples.Add(MakeSample(0.0, 16.0f, 7.0f, 5.0f, 10.0f, 3.0f));
	Samples.Add(MakeSample(1.0, 20.0f, 8.0f, 6.0f, 12.0f, 4.0f));
	Samples.Add(MakeSample(2.0, 24.0f, 10.0f, 7.0f, 14.0f, 5.0f));

	FPerformanceCaptureReport Report;
	const bool bBuilt = UPerformanceCaptureSubsystem::BuildReportFromSamples(
		Samples,
		TArray<FPerformanceCaptureEvent>(),
		FDateTime::UtcNow(),
		FPerformanceCaptureThresholdSettings(),
		Report);

	TestTrue(TEXT("BuildReportFromSamples should succeed for non-empty samples"), bBuilt);
	TestEqual(TEXT("Sample count should be preserved"), Report.SampleCount, 3);
	TestTrue(TEXT("Duration should match the last sample"), FMath::IsNearlyEqual(Report.DurationSeconds, 2.0f));
	TestTrue(TEXT("Average frame time should be computed"), FMath::IsNearlyEqual(Report.AverageFrameTimeMs, 20.0f));
	TestTrue(TEXT("Max frame time should be computed"), FMath::IsNearlyEqual(Report.MaxFrameTimeMs, 24.0f));
	TestTrue(TEXT("Average game thread time should be computed"), FMath::IsNearlyEqual(Report.AverageGameThreadTimeMs, 8.333333f, 0.001f));
	TestTrue(TEXT("Average render thread time should be computed"), FMath::IsNearlyEqual(Report.AverageRenderThreadTimeMs, 6.0f));
	TestTrue(TEXT("Average input latency should be computed"), FMath::IsNearlyEqual(Report.AverageInputLatencyTimeMs, 12.0f));
	TestTrue(TEXT("Average RHI time should be computed"), FMath::IsNearlyEqual(Report.AverageRHITTimeMs, 4.0f));
	TestFalse(TEXT("No thresholds should mean no threshold configuration"), Report.bThresholdsConfigured);
	TestTrue(TEXT("No thresholds should pass"), Report.bPassedThresholds);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerformanceInspectorThresholdEvaluationFailsWhenExceededTest,
	"PerformanceInspector.Automation.Thresholds.FailsWhenExceeded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPerformanceInspectorThresholdEvaluationFailsWhenExceededTest::RunTest(const FString& Parameters)
{
	FPerformanceCaptureReport Report;
	Report.AverageFPS = 48.0f;
	Report.AverageFrameTimeMs = 21.5f;
	Report.MaxFrameTimeMs = 40.0f;
	Report.AverageGameThreadTimeMs = 11.0f;
	Report.MaxGameThreadTimeMs = 18.0f;

	FPerformanceCaptureThresholdSettings Thresholds;
	Thresholds.MinAverageFPS = 55.0f;
	Thresholds.MaxAverageFrameTimeMs = 18.0f;
	Thresholds.MaxPeakFrameTimeMs = 33.0f;
	Thresholds.MaxPeakGameThreadTimeMs = 16.0f;

	UPerformanceCaptureSubsystem::EvaluateReportAgainstThresholds(Report, Thresholds);

	TestTrue(TEXT("Thresholds should be marked as configured"), Report.bThresholdsConfigured);
	TestFalse(TEXT("Exceeded thresholds should fail the report"), Report.bPassedThresholds);
	TestEqual(TEXT("All exceeded thresholds should be reported"), Report.ThresholdFailures.Num(), 4);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPerformanceInspectorThresholdEvaluationPassesWhenWithinBudgetTest,
	"PerformanceInspector.Automation.Thresholds.PassesWhenWithinBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPerformanceInspectorThresholdEvaluationPassesWhenWithinBudgetTest::RunTest(const FString& Parameters)
{
	FPerformanceCaptureReport Report;
	Report.AverageFPS = 61.0f;
	Report.AverageFrameTimeMs = 16.0f;
	Report.MaxFrameTimeMs = 22.0f;
	Report.AverageGameThreadTimeMs = 8.0f;
	Report.MaxGameThreadTimeMs = 12.0f;
	Report.AverageRenderThreadTimeMs = 7.0f;
	Report.MaxRenderThreadTimeMs = 10.0f;

	FPerformanceCaptureThresholdSettings Thresholds;
	Thresholds.MinAverageFPS = 60.0f;
	Thresholds.MaxAverageFrameTimeMs = 16.67f;
	Thresholds.MaxPeakFrameTimeMs = 25.0f;
	Thresholds.MaxPeakGameThreadTimeMs = 16.67f;
	Thresholds.MaxPeakRenderThreadTimeMs = 16.67f;

	UPerformanceCaptureSubsystem::EvaluateReportAgainstThresholds(Report, Thresholds);

	TestTrue(TEXT("Configured thresholds should be recognized"), Report.bThresholdsConfigured);
	TestTrue(TEXT("Within-budget metrics should pass"), Report.bPassedThresholds);
	TestEqual(TEXT("No failures should be recorded when metrics stay within threshold"), Report.ThresholdFailures.Num(), 0);

	return true;
}

#endif
