#include "PerformanceInspectorRuntime.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "PerformanceCaptureSubsystem.h"

DEFINE_LOG_CATEGORY(LogPerformanceInspectorRuntime);

namespace PerformanceInspector
{
	UPerformanceCaptureSubsystem* GetCaptureSubsystem()
	{
		if (!GEngine)
		{
			return nullptr;
		}
	
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType != EWorldType::Game && WorldContext.WorldType != EWorldType::PIE)
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

	bool TryParseBoolArgument(const FString& Value, bool& OutValue)
	{
		if (Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value == TEXT("1"))
		{
			OutValue = true;
			return true;
		}
	
		if (Value.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Value == TEXT("0"))
		{
			OutValue = false;
			return true;
		}
	
		return false;
	}
	
	void ApplyThresholdArgument(FPerformanceCaptureThresholdSettings& Thresholds, const FString& Key, const FString& Value)
	{
		float ParsedFloat = 0.0f; 
		if (!LexTryParseString(ParsedFloat, *Value))
		{
			return;
		}
	
		if (Key.Equals(TEXT("MinAverageFPS"), ESearchCase::IgnoreCase))
		{
			Thresholds.MinAverageFPS = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxAverageFrameTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxAverageFrameTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxAverageGameThreadTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxAverageGameThreadTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxAverageRenderThreadTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxAverageRenderThreadTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxAverageInputLatencyTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxAverageInputLatencyTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxAverageRHITTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxAverageRHITTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxPeakFrameTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxPeakFrameTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxPeakGameThreadTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxPeakGameThreadTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxPeakRenderThreadTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxPeakRenderThreadTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxPeakInputLatencyTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxPeakInputLatencyTimeMs = ParsedFloat;
		}
		else if (Key.Equals(TEXT("MaxPeakRHITTimeMs"), ESearchCase::IgnoreCase))
		{
			Thresholds.MaxPeakRHITTimeMs = ParsedFloat;
		}
	}
	
	void LogCaptureReport(const FPerformanceCaptureReport& Report)
	{
		UE_LOG(LogPerformanceInspectorRuntime, Display,
			TEXT("Capture saved: success=%s samples=%d duration=%.2fs avgFPS=%.2f thresholds=%s passed=%s json=%s"),
			Report.bSuccess ? TEXT("true") : TEXT("false"),
			Report.SampleCount,
			Report.DurationSeconds,
			Report.AverageFPS,
			Report.bThresholdsConfigured ? TEXT("true") : TEXT("false"),
			Report.bPassedThresholds ? TEXT("true") : TEXT("false"),
			*Report.JsonPath);
	
		for (const FString& Failure : Report.ThresholdFailures)
		{
			UE_LOG(LogPerformanceInspectorRuntime, Warning, TEXT("Threshold failure: %s"), *Failure);
		}
	}
	
	void StartTimedCaptureFromArgs(const TArray<FString>& Args)
	{
		UPerformanceCaptureSubsystem* Subsystem = GetCaptureSubsystem();
		if (!Subsystem)
		{
			UE_LOG(LogPerformanceInspectorRuntime, Warning, TEXT("PerformanceInspector.StartTimed: no active capture subsystem"));
			return;
		}
	
		if (Args.Num() == 0)
		{
			UE_LOG(LogPerformanceInspectorRuntime, Display, TEXT("Usage: PerformanceInspector.StartTimed DurationSeconds [Key=Value ...] [ExitOnCompletion=true|false] [FailOnThresholdFailure=true|false]"));
			return;
		}
	
		float DurationSeconds = 0.0f;
		if (!LexTryParseString(DurationSeconds, *Args[0]))
		{
			UE_LOG(LogPerformanceInspectorRuntime, Warning, TEXT("PerformanceInspector.StartTimed: invalid duration '%s'"), *Args[0]);
			return;
		}
	
		FPerformanceCaptureThresholdSettings Thresholds;
		bool bExitOnCompletion = false;
		bool bFailExitCodeOnThresholdFailure = false;
	
		for (int32 ArgIndex = 1; ArgIndex < Args.Num(); ++ArgIndex)
		{
			FString Key;
			FString Value;
			if (!Args[ArgIndex].Split(TEXT("="), &Key, &Value))
			{
				continue;
			}
	
			bool bParsedBool = false;
			if (Key.Equals(TEXT("ExitOnCompletion"), ESearchCase::IgnoreCase))
			{
				bParsedBool = TryParseBoolArgument(Value, bExitOnCompletion);
			}
			else if (Key.Equals(TEXT("FailOnThresholdFailure"), ESearchCase::IgnoreCase))
			{
				bParsedBool = TryParseBoolArgument(Value, bFailExitCodeOnThresholdFailure);
			}
	
			if (!bParsedBool)
			{
				ApplyThresholdArgument(Thresholds, Key, Value);
			}
		}
	
		Subsystem->StartTimedCaptureSession(DurationSeconds, Thresholds, bExitOnCompletion, bFailExitCodeOnThresholdFailure);
		UE_LOG(LogPerformanceInspectorRuntime, Display, TEXT("PerformanceInspector.StartTimed: started %.2fs capture"), DurationSeconds);
	}
	
	static FAutoConsoleCommand PerformanceInspectorStartCommand(
		TEXT("PerformanceInspector.Start"),
		TEXT("Start a live performance capture session."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			if (UPerformanceCaptureSubsystem* Subsystem = GetCaptureSubsystem())
			{
				Subsystem->StartCaptureSession();
				UE_LOG(LogPerformanceInspectorRuntime, Display, TEXT("PerformanceInspector.Start: capture started"));
			}
		}));
	
	static FAutoConsoleCommand PerformanceInspectorStartTimedCommand(
		TEXT("PerformanceInspector.StartTimed"),
		TEXT("Start a timed performance capture. Usage: PerformanceInspector.StartTimed 10 MinAverageFPS=55 MaxPeakFrameTimeMs=40 ExitOnCompletion=true"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&StartTimedCaptureFromArgs));
	
	static FAutoConsoleCommand PerformanceInspectorStopCommand(
		TEXT("PerformanceInspector.Stop"),
		TEXT("Stop the active performance capture session and save the report."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			if (UPerformanceCaptureSubsystem* Subsystem = GetCaptureSubsystem())
			{
				LogCaptureReport(Subsystem->StopCaptureSession());
			}
		}));
	
	static FAutoConsoleCommand PerformanceInspectorMarkCommand(
		TEXT("PerformanceInspector.Mark"),
		TEXT("Add a capture marker. Usage: PerformanceInspector.Mark Name [Category] [Details] [Severity]"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			if (Args.Num() == 0)
			{
				UE_LOG(LogPerformanceInspectorRuntime, Display, TEXT("Usage: PerformanceInspector.Mark Name [Category] [Details] [Severity]"));
				return;
			}
	
			if (UPerformanceCaptureSubsystem* Subsystem = GetCaptureSubsystem())
			{
				const FString Category = Args.Num() > 1 ? Args[1] : TEXT("Gameplay");
				const FString Details = Args.Num() > 2 ? Args[2] : TEXT("");
				const FString SeverityString = Args.Num() > 3 ? Args[3] : TEXT("Info");
				EPerformanceCaptureEventSeverity Severity = EPerformanceCaptureEventSeverity::Info;
				if (SeverityString.Equals(TEXT("Warning"), ESearchCase::IgnoreCase))
				{
					Severity = EPerformanceCaptureEventSeverity::Warning;
				}
				else if (SeverityString.Equals(TEXT("Critical"), ESearchCase::IgnoreCase))
				{
					Severity = EPerformanceCaptureEventSeverity::Critical;
				}
	
				Subsystem->AddEventMarker(FName(*Args[0]), Category, Details, Severity);
			}
		}));
	
	static FAutoConsoleCommand PerformanceInspectorStatusCommand(
		TEXT("PerformanceInspector.Status"),
		TEXT("Log the current capture status and the most recent report."),
		FConsoleCommandDelegate::CreateLambda([]()
		{
			if (UPerformanceCaptureSubsystem* Subsystem = GetCaptureSubsystem())
			{
				const FPerformanceCaptureReport& Report = Subsystem->GetLastCaptureReport();
				UE_LOG(LogPerformanceInspectorRuntime, Display, TEXT("PerformanceInspector.Status: active=%s samples=%d duration=%.2f"),
					Subsystem->IsCaptureSessionActive() ? TEXT("true") : TEXT("false"),
					Subsystem->GetCaptureSampleCount(),
					Subsystem->GetCaptureSessionDurationSeconds());
				if (Report.SampleCount > 0)
				{
					LogCaptureReport(Report);
				}
			}
		}));
}

void FPerformanceInspectorRuntimeModule::StartupModule()
{
}

void FPerformanceInspectorRuntimeModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FPerformanceInspectorRuntimeModule, PerformanceInspectorRuntime)
