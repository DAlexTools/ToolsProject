#include "SPerformanceInspectorPanel.h"

#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "IDesktopPlatform.h"
#include "ISettingsModule.h"
#include "Misc/Paths.h"
#include "SPerformanceGraphWidget.h"
#include "Settings/PerformanceInspectorAutomationSettings.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SPerformanceInspectorPanel"

void SPerformanceInspectorPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(10.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				BuildMenuBar()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				BuildToolbar()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SBorder)
				.Padding(8.0f)
				[
					SNew(STextBlock)
					.Text(this, &SPerformanceInspectorPanel::GetSummaryText)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SBorder)
				.Padding(8.0f)
				[
					SNew(STextBlock)
					.Text(this, &SPerformanceInspectorPanel::GetCaptureSummaryText)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(PerformanceGraphWidget, SPerformanceGraphWidget)
			]
		]
	];

	if (PerformanceGraphWidget.IsValid())
	{
		PerformanceGraphWidget->SetPaused(bIsPaused);
	}
}

TSharedRef<SWidget> SPerformanceInspectorPanel::BuildMenuBar()
{
	FMenuBarBuilder MenuBarBuilder(nullptr);

	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("PluginMenuLabel", "Plugin"),
		LOCTEXT("PluginMenuTooltip", "Plugin settings and data locations."),
		FNewMenuDelegate::CreateSP(this, &SPerformanceInspectorPanel::FillPluginMenu));

	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("SessionMenuLabel", "Session"),
		LOCTEXT("SessionMenuTooltip", "Open, inspect and manage capture sessions."),
		FNewMenuDelegate::CreateSP(this, &SPerformanceInspectorPanel::FillSessionMenu));

	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("ViewMenuLabel", "View"),
		LOCTEXT("ViewMenuTooltip", "Graph view and history controls."),
		FNewMenuDelegate::CreateSP(this, &SPerformanceInspectorPanel::FillViewMenu));

	return MenuBarBuilder.MakeWidget();
}

void SPerformanceInspectorPanel::FillPluginMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("Plugin", LOCTEXT("PluginSectionLabel", "Plugin"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("PluginSettingsLabel", "Performance Inspector Settings"),
		LOCTEXT("PluginSettingsTooltip", "Open the plugin Developer Settings for PIE automation and capture defaults."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::OpenPluginSettings)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("PluginCaptureFolderLabel", "Open Capture Folder"),
		LOCTEXT("PluginCaptureFolderTooltip", "Open the Saved/PerformanceCaptures folder used by the plugin."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::OpenCaptureDirectory),
			FCanExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::CanOpenCaptureDirectory)));
	MenuBuilder.EndSection();
}

void SPerformanceInspectorPanel::FillSessionMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("Sessions", LOCTEXT("QuickLinksSessionsSection", "Sessions"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SessionOpenSavedLabel", "Open Saved Session..."),
		LOCTEXT("SessionOpenSavedTooltip", "Load a saved capture session from disk."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { OnOpenSavedClicked(); })));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SessionLastCaptureLabel", "Open Last Capture Json"),
		LOCTEXT("SessionLastCaptureTooltip", "Open the most recently saved JSON report in the default external application."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::OpenLastCaptureJson),
			FCanExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::CanOpenLastCaptureJson)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SessionCurrentFolderLabel", "Reveal Current Saved Session"),
		LOCTEXT("SessionCurrentFolderTooltip", "Open the folder for the currently loaded saved session."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::OpenCurrentSavedSessionFolder),
			FCanExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::CanOpenCurrentSavedSessionFolder)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SessionExportSummaryPngLabel", "Export Summary PNG..."),
		LOCTEXT("SessionExportSummaryPngTooltip", "Render the current live session or opened saved session to a PNG report."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::ExportCurrentSummaryPng),
			FCanExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::CanExportCurrentSummaryPng)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SessionReturnLiveLabel", "Return to Live View"),
		LOCTEXT("SessionReturnLiveTooltip", "Exit saved session mode and return to the live graph."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]() { OnReturnToLiveClicked(); }),
			FCanExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::CanReturnToLiveView)));
	MenuBuilder.EndSection();
}

void SPerformanceInspectorPanel::FillViewMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("Graph", LOCTEXT("ViewGraphSectionLabel", "Graph"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewAutoScaleLabel", "Toggle Auto Scale"),
		LOCTEXT("ViewAutoScaleTooltip", "Switch between auto scale and fixed scale for the graph."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::ToggleAutoScale)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewFocusAllLabel", "Focus All Metrics"),
		LOCTEXT("ViewFocusAllTooltip", "Show all metric series."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (PerformanceGraphWidget.IsValid())
			{
				PerformanceGraphWidget->FocusAllMetrics();
			}
		})));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewFocusFrameLabel", "Focus Frame"),
		LOCTEXT("ViewFocusFrameTooltip", "Show only frame time."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (PerformanceGraphWidget.IsValid())
			{
				PerformanceGraphWidget->FocusFrameMetric();
			}
		})));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewFocusGameLabel", "Focus Game"),
		LOCTEXT("ViewFocusGameTooltip", "Show only game thread time."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (PerformanceGraphWidget.IsValid())
			{
				PerformanceGraphWidget->FocusGameThreadMetric();
			}
		})));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewFocusRenderLabel", "Focus Render"),
		LOCTEXT("ViewFocusRenderTooltip", "Show only render thread time."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (PerformanceGraphWidget.IsValid())
			{
				PerformanceGraphWidget->FocusRenderThreadMetric();
			}
		})));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewFocusRhiLabel", "Focus RHI"),
		LOCTEXT("ViewFocusRhiTooltip", "Show only RHI thread time."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (PerformanceGraphWidget.IsValid())
			{
				PerformanceGraphWidget->FocusRHIMetric();
			}
		})));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewFocusGpuLabel", "Focus GPU"),
		LOCTEXT("ViewFocusGpuTooltip", "Show only GPU series."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			if (PerformanceGraphWidget.IsValid())
			{
				PerformanceGraphWidget->FocusGPUMetrics();
			}
		})));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("History", LOCTEXT("ViewHistorySectionLabel", "History"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewHistory100Label", "History 100"),
		LOCTEXT("ViewHistory100Tooltip", "Show 100 samples in live mode."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { OnHistoryClicked(100); })));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewHistory300Label", "History 300"),
		LOCTEXT("ViewHistory300Tooltip", "Show 300 samples in live mode."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { OnHistoryClicked(300); })));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewHistory1000Label", "History 1000"),
		LOCTEXT("ViewHistory1000Tooltip", "Show 1000 samples in live mode."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this]() { OnHistoryClicked(1000); })));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("SavedSession", LOCTEXT("ViewSavedSessionSectionLabel", "Saved Session"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewZoomInLabel", "Zoom In"),
		LOCTEXT("ViewZoomInTooltip", "Zoom into the currently loaded saved session."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]() { OnZoomInClicked(); }),
			FCanExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::CanReturnToLiveView)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewZoomOutLabel", "Zoom Out"),
		LOCTEXT("ViewZoomOutTooltip", "Zoom out of the currently loaded saved session."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]() { OnZoomOutClicked(); }),
			FCanExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::CanReturnToLiveView)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewFitLabel", "Fit Session"),
		LOCTEXT("ViewFitTooltip", "Fit the currently loaded saved session into view."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]() { OnFitSessionClicked(); }),
			FCanExecuteAction::CreateSP(this, &SPerformanceInspectorPanel::CanReturnToLiveView)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ViewClearSelectionLabel", "Clear Range Selection"),
		LOCTEXT("ViewClearSelectionTooltip", "Clear the current selected sample range."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this]()
			{
				if (PerformanceGraphWidget.IsValid())
				{
					PerformanceGraphWidget->ClearSelectedRange();
				}
			}),
			FCanExecuteAction::CreateLambda([this]()
			{
				return PerformanceGraphWidget.IsValid() && PerformanceGraphWidget->HasSelectedRange();
			})));
	MenuBuilder.EndSection();
}

TSharedRef<SWidget> SPerformanceInspectorPanel::BuildToolbar()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(SButton)
			.Text(this, &SPerformanceInspectorPanel::GetStartButtonText)
			.OnClicked(this, &SPerformanceInspectorPanel::OnPauseClicked)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 12.0f, 0.0f)
		[
			SNew(SButton)
			.Text(this, &SPerformanceInspectorPanel::GetCaptureButtonText)
			.OnClicked(this, &SPerformanceInspectorPanel::OnCaptureClicked)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 12.0f, 0.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ClearLabel", "Clear"))
			.OnClicked(this, &SPerformanceInspectorPanel::OnClearClicked)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 12.0f, 0.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("OpenSavedLabel", "Open Session"))
			.OnClicked(this, &SPerformanceInspectorPanel::OnOpenSavedClicked)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(SButton)
			.Visibility(this, &SPerformanceInspectorPanel::GetReturnToLiveVisibility)
			.Text(LOCTEXT("ReturnLiveLabel", "Live"))
			.OnClicked(this, &SPerformanceInspectorPanel::OnReturnToLiveClicked)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SButton)
			.Visibility(this, &SPerformanceInspectorPanel::GetSavedSessionControlsVisibility)
			.Text(LOCTEXT("ZoomOutLabel", "Zoom -"))
			.OnClicked(this, &SPerformanceInspectorPanel::OnZoomOutClicked)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			SNew(SButton)
			.Visibility(this, &SPerformanceInspectorPanel::GetSavedSessionControlsVisibility)
			.Text(LOCTEXT("ZoomInLabel", "Zoom +"))
			.OnClicked(this, &SPerformanceInspectorPanel::OnZoomInClicked)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 12.0f, 0.0f)
		[
			SNew(SButton)
			.Visibility(this, &SPerformanceInspectorPanel::GetSavedSessionControlsVisibility)
			.Text(LOCTEXT("FitSessionLabel", "Fit"))
			.OnClicked(this, &SPerformanceInspectorPanel::OnFitSessionClicked)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(SButton)
			.Text(this, &SPerformanceInspectorPanel::GetHistoryButtonText, 100)
			.OnClicked(this, &SPerformanceInspectorPanel::OnHistoryClicked, 100)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(SButton)
			.Text(this, &SPerformanceInspectorPanel::GetHistoryButtonText, 300)
			.OnClicked(this, &SPerformanceInspectorPanel::OnHistoryClicked, 300)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 12.0f, 0.0f)
		[
			SNew(SButton)
			.Text(this, &SPerformanceInspectorPanel::GetHistoryButtonText, 1000)
			.OnClicked(this, &SPerformanceInspectorPanel::OnHistoryClicked, 1000)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 12.0f, 0.0f)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]()
			{
				return PerformanceGraphWidget.IsValid() && PerformanceGraphWidget->IsAutoScaleEnabled()
					? ECheckBoxState::Checked
					: ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged(this, &SPerformanceInspectorPanel::OnAutoScaleChanged)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AutoScaleLabel", "Auto Scale"))
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(this, &SPerformanceInspectorPanel::GetStatusText)
		];
}

FReply SPerformanceInspectorPanel::OnPauseClicked()
{
	bIsPaused = !bIsPaused;
	PerformanceGraphWidget->SetPaused(bIsPaused);
	
	return FReply::Handled();
}

FText SPerformanceInspectorPanel::GetStartButtonText() const
{
	return bIsPaused ? LOCTEXT("StartLabel", "Start") : LOCTEXT("StopLabel", "Stop");
}

FReply SPerformanceInspectorPanel::OnClearClicked()
{
	PerformanceGraphWidget->ClearSamples();
	return FReply::Handled();
}

FReply SPerformanceInspectorPanel::OnCaptureClicked()
{
	if (PerformanceGraphWidget->IsCaptureSessionActive())
	{
		FString CsvPath;
		FString JsonPath;
		PerformanceGraphWidget->StopCaptureSession(CsvPath, JsonPath);
	}
	else
	{
		PerformanceGraphWidget->StartCaptureSession();
	}

	return FReply::Handled();
}

FReply SPerformanceInspectorPanel::OnOpenSavedClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}

	void* ParentWindowHandle = nullptr;
	if (const TSharedPtr<SWindow> Window = FSlateApplication::Get().FindBestParentWindowForDialogs(nullptr))
	{
		ParentWindowHandle = Window->GetNativeWindow()->GetOSWindowHandle();
	}

	TArray<FString> SelectedFiles;
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("Open Performance Capture Session"),
		FPaths::ProjectSavedDir(),
		TEXT(""),
		TEXT("Performance Capture Json (*.json)|*.json"),
		EFileDialogFlags::None,
		SelectedFiles);

	if (bOpened && SelectedFiles.Num() > 0)
	{
		PerformanceGraphWidget->LoadCaptureSessionFromJson(FPaths::ConvertRelativePathToFull(SelectedFiles[0]));
	}

	return FReply::Handled();
}

FReply SPerformanceInspectorPanel::OnReturnToLiveClicked()
{
	PerformanceGraphWidget->ExitSavedSessionMode();
	return FReply::Handled();
}

FReply SPerformanceInspectorPanel::OnZoomInClicked()
{
	PerformanceGraphWidget->ZoomSavedSession(0.75f);
	return FReply::Handled();
}

FReply SPerformanceInspectorPanel::OnZoomOutClicked()
{
	PerformanceGraphWidget->ZoomSavedSession(1.25f);
	return FReply::Handled();
}

FReply SPerformanceInspectorPanel::OnFitSessionClicked()
{
	PerformanceGraphWidget->ResetSavedSessionView();
	return FReply::Handled();
}

FReply SPerformanceInspectorPanel::OnHistoryClicked(int32 InMaxSamples)
{
	PerformanceGraphWidget->SetMaxSamples(InMaxSamples);
	return FReply::Handled();
}

void SPerformanceInspectorPanel::OnAutoScaleChanged(ECheckBoxState NewState)
{
	PerformanceGraphWidget->SetAutoScaleEnabled(NewState == ECheckBoxState::Checked);
}

void SPerformanceInspectorPanel::OpenPluginSettings()
{
	const UPerformanceInspectorAutomationSettings* Settings = GetDefault<UPerformanceInspectorAutomationSettings>();
	if (!Settings)
	{
		return;
	}

	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(
		Settings->GetContainerName(),
		Settings->GetCategoryName(),
		Settings->GetSectionName());
}

void SPerformanceInspectorPanel::OpenCaptureDirectory()
{
	const FString CaptureDirectory = PerformanceGraphWidget->GetCaptureDirectoryPath();
	if (CaptureDirectory.IsEmpty())
	{
		return;
	}

	IFileManager::Get().MakeDirectory(*CaptureDirectory, true);
	RevealFolder(CaptureDirectory);
}

void SPerformanceInspectorPanel::OpenLastCaptureJson()
{
	OpenFileExternally(PerformanceGraphWidget->GetLastCaptureJsonPath());
}

void SPerformanceInspectorPanel::OpenCurrentSavedSessionFolder()
{
	const FString SavedSessionPath = PerformanceGraphWidget->GetSavedSessionSourcePath();
	if (SavedSessionPath.IsEmpty())
	{
		return;
	}

	RevealFolder(FPaths::GetPath(SavedSessionPath));
}

void SPerformanceInspectorPanel::ExportCurrentSummaryPng()
{
	if (!PerformanceGraphWidget.IsValid())
	{
		return;
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	const FString CaptureDirectory = PerformanceGraphWidget->GetCaptureDirectoryPath();
	IFileManager::Get().MakeDirectory(*CaptureDirectory, true);

	const FString DefaultFileName = FString::Printf(TEXT("performance_summary_%s.png"), *FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S")));
	void* ParentWindowHandle = nullptr;
	if (const TSharedPtr<SWindow> Window = FSlateApplication::Get().FindBestParentWindowForDialogs(nullptr))
	{
		ParentWindowHandle = Window->GetNativeWindow()->GetOSWindowHandle();
	}

	TArray<FString> SelectedFiles;
	const bool bSelected = DesktopPlatform->SaveFileDialog(
		ParentWindowHandle,
		TEXT("Export Performance Summary PNG"),
		CaptureDirectory,
		DefaultFileName,
		TEXT("PNG Image (*.png)|*.png"),
		EFileDialogFlags::None,
		SelectedFiles);

	if (bSelected && SelectedFiles.Num() > 0)
	{
		PerformanceGraphWidget->ExportCurrentSummaryPng(FPaths::ConvertRelativePathToFull(SelectedFiles[0]));
	}
}

void SPerformanceInspectorPanel::ToggleAutoScale()
{
	PerformanceGraphWidget->SetAutoScaleEnabled(!PerformanceGraphWidget->IsAutoScaleEnabled());
}

bool SPerformanceInspectorPanel::CanOpenCaptureDirectory() const
{
	return PerformanceGraphWidget.IsValid() && !PerformanceGraphWidget->GetCaptureDirectoryPath().IsEmpty();
}

bool SPerformanceInspectorPanel::CanOpenLastCaptureJson() const
{
	return PerformanceGraphWidget.IsValid() && FPaths::FileExists(PerformanceGraphWidget->GetLastCaptureJsonPath());
}

bool SPerformanceInspectorPanel::CanOpenCurrentSavedSessionFolder() const
{
	return PerformanceGraphWidget.IsValid() && FPaths::FileExists(PerformanceGraphWidget->GetSavedSessionSourcePath());
}

bool SPerformanceInspectorPanel::CanReturnToLiveView() const
{
	return PerformanceGraphWidget.IsValid() && PerformanceGraphWidget->IsSavedSessionModeActive();
}

bool SPerformanceInspectorPanel::CanExportCurrentSummaryPng() const
{
	return PerformanceGraphWidget.IsValid() && PerformanceGraphWidget->CanExportCurrentSummaryPng();
}

void SPerformanceInspectorPanel::RevealFolder(const FString& FolderPath) const
{
	if (FolderPath.IsEmpty())
	{
		return;
	}

	const FString AbsoluteFolderPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FolderPath);
	FPlatformProcess::ExploreFolder(*AbsoluteFolderPath);
}

void SPerformanceInspectorPanel::OpenFileExternally(const FString& FilePath) const
{
	if (FilePath.IsEmpty())
	{
		return;
	}

	const FString AbsoluteFilePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FilePath);
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*AbsoluteFilePath, nullptr, ELaunchVerb::Open);
}

FText SPerformanceInspectorPanel::GetStatusText() const
{
	if (!PerformanceGraphWidget.IsValid())
	{
		return LOCTEXT("StatusUnavailable", "Inspector unavailable");
	}

	return FText::Format(
		LOCTEXT("StatusFormat", "{0} | {1} | History {2} | {3} | {4}"),
		bIsPaused ? LOCTEXT("PausedStatus", "Paused") : LOCTEXT("LiveStatus", "Live"),
		PerformanceGraphWidget->GetDataSourceText(),
		FText::AsNumber(PerformanceGraphWidget->GetMaxSamples()),
		PerformanceGraphWidget->IsAutoScaleEnabled() ? LOCTEXT("AutoScaleStatus", "Auto Scale") : LOCTEXT("FixedScaleStatus", "Fixed Scale"),
		PerformanceGraphWidget->GetLastCaptureSummaryText());
}

FText SPerformanceInspectorPanel::GetSummaryText() const
{
	if (!PerformanceGraphWidget.IsValid())
	{
		return LOCTEXT("SummaryUnavailable", "Summary unavailable");
	}

	const auto BuildRow = [this](const FText& Label, FName SeriesName) -> FText
	{
		FPerformanceSeriesStats Stats;
		if (!PerformanceGraphWidget->GetSeriesStats(SeriesName, Stats))
		{
			return FText::Format(LOCTEXT("SummaryNoSamplesFormat", "{0}: no samples yet"), Label);
		}

		return FText::Format(
			LOCTEXT("SummaryRowFormat", "{0}: current {1} ms | avg {2} ms | max {3} ms"),
			Label,
			FText::AsNumber(Stats.CurrentMs),
			FText::AsNumber(Stats.AverageMs),
			FText::AsNumber(Stats.MaxMs));
	};

	return FText::Format(
		LOCTEXT("SummaryFormat", "{0}\n{1}\n{2}\n{3}\nCapture: {4} | {5} s | {6} samples"),
		BuildRow(LOCTEXT("FrameLabel", "Frame"), TEXT("FrameTime")),
		BuildRow(LOCTEXT("GameLabel", "Game"), TEXT("GameThread")),
		BuildRow(LOCTEXT("RenderLabel", "Render"), TEXT("RenderThread")),
		BuildRow(LOCTEXT("RhiLabel", "RHI"), TEXT("RHITTime")),
		PerformanceGraphWidget->IsCaptureSessionActive() ? LOCTEXT("RecordingState", "RECORDING") : LOCTEXT("IdleState", "idle"),
		FText::AsNumber(PerformanceGraphWidget->GetCaptureSessionDurationSeconds()),
		FText::AsNumber(PerformanceGraphWidget->GetCaptureSampleCount()));
}

FText SPerformanceInspectorPanel::GetCaptureSummaryText() const
{
	if (!PerformanceGraphWidget.IsValid())
	{
		return LOCTEXT("CaptureSummaryUnavailable", "Session Summary unavailable");
	}

	return PerformanceGraphWidget->GetCaptureSessionSummaryText();
}

FText SPerformanceInspectorPanel::GetHistoryButtonText(int32 InMaxSamples) const
{
	const bool bIsSelected = PerformanceGraphWidget.IsValid() && PerformanceGraphWidget->GetMaxSamples() == InMaxSamples;
	return FText::FromString(bIsSelected ? FString::Printf(TEXT("[%d]"), InMaxSamples) : FString::Printf(TEXT("%d"), InMaxSamples));
}

FText SPerformanceInspectorPanel::GetCaptureButtonText() const
{
	if (!PerformanceGraphWidget.IsValid() || !PerformanceGraphWidget->IsCaptureSessionActive())
	{
		return LOCTEXT("StartCaptureLabel", "Record");
	}

	return LOCTEXT("StopCaptureLabel", "Stop Recording");
}

EVisibility SPerformanceInspectorPanel::GetSavedSessionControlsVisibility() const
{
	return PerformanceGraphWidget.IsValid() && PerformanceGraphWidget->IsSavedSessionModeActive()
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

EVisibility SPerformanceInspectorPanel::GetReturnToLiveVisibility() const
{
	return GetSavedSessionControlsVisibility();
}

#undef LOCTEXT_NAMESPACE
