#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SPerformanceGraphWidget;
class STextBlock;
enum class ECheckBoxState : uint8;

class PERFORMANCEINSPECTOREDITOR_API SPerformanceInspectorPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPerformanceInspectorPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> BuildMenuBar();
	TSharedRef<SWidget> BuildToolbar();
	FReply OnPauseClicked();
	FReply OnClearClicked();
	FReply OnCaptureClicked();
	FReply OnOpenSavedClicked();
	FReply OnReturnToLiveClicked();
	FReply OnZoomInClicked();
	FReply OnZoomOutClicked();
	FReply OnFitSessionClicked();
	FReply OnHistoryClicked(int32 InMaxSamples);
	void OnAutoScaleChanged(ECheckBoxState NewState);
	void FillPluginMenu(FMenuBuilder& MenuBuilder);
	void FillSessionMenu(FMenuBuilder& MenuBuilder);
	void FillViewMenu(FMenuBuilder& MenuBuilder);
	void OpenPluginSettings();
	void OpenCaptureDirectory();
	void OpenLastCaptureJson();
	void OpenCurrentSavedSessionFolder();
	void ExportCurrentSummaryPng();
	void ToggleAutoScale();
	bool CanOpenCaptureDirectory() const;
	bool CanOpenLastCaptureJson() const;
	bool CanOpenCurrentSavedSessionFolder() const;
	bool CanReturnToLiveView() const;
	bool CanExportCurrentSummaryPng() const;
	void RevealFolder(const FString& FolderPath) const;
	void OpenFileExternally(const FString& FilePath) const;
	FText GetStartButtonText() const;
	FText GetHistoryButtonText(int32 InMaxSamples) const;
	FText GetCaptureButtonText() const;
	FText GetStatusText() const;
	FText GetSummaryText() const;
	FText GetCaptureSummaryText() const;
	EVisibility GetSavedSessionControlsVisibility() const;
	EVisibility GetReturnToLiveVisibility() const;

	bool bIsPaused = true;

	TSharedPtr<SPerformanceGraphWidget> PerformanceGraphWidget;
};
