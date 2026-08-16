#include "PerformanceCaptureBlueprintLibrary.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "PerformanceCaptureSubsystem.h"

void UPerformanceCaptureBlueprintLibrary::StartPerformanceCapture(UObject* WorldContextObject)
{
	if (UPerformanceCaptureSubsystem* Subsystem = ResolveSubsystem(WorldContextObject))
	{
		Subsystem->StartCaptureSession();
	}
}

void UPerformanceCaptureBlueprintLibrary::StartTimedPerformanceCapture(UObject* WorldContextObject, float DurationSeconds,
	const FPerformanceCaptureThresholdSettings& Thresholds, bool bExitProcessOnCompletion, bool bFailExitCodeOnThresholdFailure)
{
	if (UPerformanceCaptureSubsystem* Subsystem = ResolveSubsystem(WorldContextObject))
	{
		Subsystem->StartTimedCaptureSession(DurationSeconds, Thresholds, bExitProcessOnCompletion, bFailExitCodeOnThresholdFailure);
	}
}

FPerformanceCaptureReport UPerformanceCaptureBlueprintLibrary::StopPerformanceCapture(UObject* WorldContextObject)
{
	if (UPerformanceCaptureSubsystem* Subsystem = ResolveSubsystem(WorldContextObject))
	{
		return Subsystem->StopCaptureSession();
	}

	return FPerformanceCaptureReport();
}

bool UPerformanceCaptureBlueprintLibrary::IsPerformanceCaptureActive(UObject* WorldContextObject)
{
	if (UPerformanceCaptureSubsystem* Subsystem = ResolveSubsystem(WorldContextObject))
	{
		return Subsystem->IsCaptureSessionActive();
	}

	return false;
}

int32 UPerformanceCaptureBlueprintLibrary::GetPerformanceCaptureSampleCount(UObject* WorldContextObject)
{
	if (UPerformanceCaptureSubsystem* Subsystem = ResolveSubsystem(WorldContextObject))
	{
		return Subsystem->GetCaptureSampleCount();
	}

	return 0;
}

float UPerformanceCaptureBlueprintLibrary::GetPerformanceCaptureDurationSeconds(UObject* WorldContextObject)
{
	if (UPerformanceCaptureSubsystem* Subsystem = ResolveSubsystem(WorldContextObject))
	{
		return Subsystem->GetCaptureSessionDurationSeconds();
	}

	return 0.0f;
}

FPerformanceCaptureReport UPerformanceCaptureBlueprintLibrary::GetLastPerformanceCaptureReport(UObject* WorldContextObject)
{
	if (UPerformanceCaptureSubsystem* Subsystem = ResolveSubsystem(WorldContextObject))
	{
		return Subsystem->GetLastCaptureReport();
	}

	return FPerformanceCaptureReport();
}

void UPerformanceCaptureBlueprintLibrary::AddPerformanceMarker(UObject* WorldContextObject, FName Name, const FString& Category,
	const FString& Details, EPerformanceCaptureEventSeverity Severity, FLinearColor Color)
{
	if (UPerformanceCaptureSubsystem* Subsystem = ResolveSubsystem(WorldContextObject))
	{
		Subsystem->AddEventMarker(Name, Category, Details, Severity, Color);
	}
}

UPerformanceCaptureSubsystem* UPerformanceCaptureBlueprintLibrary::ResolveSubsystem(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UPerformanceCaptureSubsystem>();
		}
	}

	return nullptr;
}
