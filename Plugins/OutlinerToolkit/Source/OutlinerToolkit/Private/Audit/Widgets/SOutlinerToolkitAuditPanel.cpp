// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Audit/Widgets/SOutlinerToolkitAuditPanel.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GameFramework/Actor.h"
#include "Registry/OutlinerMenuSectionRegistry.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "Audit/Widgets/SOutlinerAuditActorRow.h"
#include "Audit/Widgets/SOutlinerActorIssueDetailRow.h"
#include "Audit/Core/OutlinerAuditUtils.h"
#include "Audit/Core/OutlinerAuditConstants.h"
#include "Audit/Services/OutlinerAuditEditorService.h"
#include "Audit/Services/OutlinerAuditRunnerService.h"
#include "Audit/Core/OutlinerAuditFormatting.h"
#include "Audit/Core/OutlinerAuditBitmask.h"
#include "Audit/Engine/OutlinerAuditActorHelpers.h"

#define LOCTEXT_NAMESPACE "SOutlinerToolkitAuditPanel"


void SOutlinerToolkitAuditPanel::Construct(const FArguments& InArgs)
{
	AuditScopeOptions.Reset();
	AuditScopeOptions.Add(MakeShared<EOutlinerAuditScope>(EOutlinerAuditScope::SelectedActors));
	AuditScopeOptions.Add(MakeShared<EOutlinerAuditScope>(EOutlinerAuditScope::CurrentLevel));
	AuditScopeOptions.Add(MakeShared<EOutlinerAuditScope>(EOutlinerAuditScope::VisibleActors));
	AuditScopeOptions.Add(MakeShared<EOutlinerAuditScope>(EOutlinerAuditScope::WholeWorld));
	SelectedAuditScopeOption = AuditScopeOptions.Last();
	LoadPersistentIgnoredIssueKeys();

	ChildSlot
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(8.0f)
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
						[
							SNew(SSeparator)
								.Orientation(EOrientation::Orient_Horizontal)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							BuildCriteriaWidget()
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.0f, 0.0f, 6.0f, 6.0f)
								[
									SNew(SButton)
										.Text(LOCTEXT("RunAuditButton", "Run Audit"))
										.ToolTipText(LOCTEXT("RunAuditTooltip", "Scan the current editor world for " "common Scene Outliner issues."))
										.IsEnabled(this, &SOutlinerToolkitAuditPanel::HasEnabledCriteria)
										.OnClicked(this, &SOutlinerToolkitAuditPanel::OnRunAuditClicked)
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.0f, 0.0f, 6.0f, 6.0f)
								[
									SNew(SButton)
										.Text(LOCTEXT("SelectActorButton", "Select Actor"))
										.ToolTipText(LOCTEXT("SelectActorTooltip", "Select the highlighted audit actor."))
										.IsEnabled(this, &SOutlinerToolkitAuditPanel::HasSelectedActor)
										.OnClicked(this, &SOutlinerToolkitAuditPanel::OnSelectActorClicked)
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.0f, 0.0f, 6.0f, 6.0f)
								[
									SNew(SButton)
										.Text(LOCTEXT("SelectAllActorsButton", "Select All Listed"))
										.ToolTipText(LOCTEXT("SelectAllActorsTooltip", "Select every valid actor currently " "listed in the audit results."))
										.IsEnabled_Lambda([this]()
											{
												return !FilteredActorResults.IsEmpty();
											})
										.OnClicked(this, &SOutlinerToolkitAuditPanel::OnSelectAllActorsClicked)
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.0f, 0.0f, 6.0f, 6.0f)
								[
									SNew(SButton)
										.Text(LOCTEXT("FixSelectedActorButton", "Fix Actor Issues"))
										.ToolTipText(LOCTEXT("FixSelectedActorTooltip", "Apply every conservative automatic fix " "available for the highlighted actor."))
										.IsEnabled(this, &SOutlinerToolkitAuditPanel::HasFixableSelectedActorIssues)
										.OnClicked(this, &SOutlinerToolkitAuditPanel::OnFixSelectedActorClicked)
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.0f, 0.0f, 6.0f, 6.0f)
								[
									SNew(SButton)
										.Text(LOCTEXT("IgnoreSelectedActorButton", "Ignore Actor Issues"))
										.ToolTipText(LOCTEXT("IgnoreSelectedActorTooltip", "Hide every visible issue for the " "highlighted actor until this " "panel is reopened."))
										.IsEnabled(this, &SOutlinerToolkitAuditPanel::HasSelectedActorData)
										.OnClicked(this, &SOutlinerToolkitAuditPanel::OnIgnoreSelectedActorClicked)
								]

								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.HAlign(HAlign_Right)
								.VAlign(VAlign_Center)
								.Padding(8.0f, 0.0f, 0.0f, 6.0f)
								[
									SNew(STextBlock)
										.Text(this, &SOutlinerToolkitAuditPanel::GetSummaryText)
										.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
						]

						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SAssignNew(ActorListView, SListView<FOutlinerAuditActorResultPtr>)
										.ListItemsSource(&FilteredActorResults)
										.SelectionMode(ESelectionMode::Single)
										.OnGenerateRow(this, &SOutlinerToolkitAuditPanel::OnGenerateActorRow)
										.OnSelectionChanged(this, &SOutlinerToolkitAuditPanel::OnActorSelectionChanged)
										.OnMouseButtonDoubleClick(this, &SOutlinerToolkitAuditPanel::OnActorDoubleClicked)
										.OnContextMenuOpening(this, &SOutlinerToolkitAuditPanel::BuildActorContextMenu)
										.HeaderRow(BuildHeaderRow())
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(8.0f, 0.0f, 0.0f, 0.0f)
								[
									SNew(SBox)
										.WidthOverride(360.0f)
										.Visibility(this, &SOutlinerToolkitAuditPanel::GetDetailsPanelVisibility)
										[
											BuildDetailsPanel()
										]
								]
						]
				]
		];

	RefreshAudit();
}

void SOutlinerToolkitAuditPanel::LoadPersistentIgnoredIssueKeys()
{
	FOutlinerAuditEditorService::LoadPersistentIgnoredIssueKeys(IgnoredIssueKeys);
}

void SOutlinerToolkitAuditPanel::SavePersistentIgnoredIssueKeys() const
{
	FOutlinerAuditEditorService::SavePersistentIgnoredIssueKeys(IgnoredIssueKeys);
}

void SOutlinerToolkitAuditPanel::RefreshAudit()
{
	AllAuditIssues.Reset();
	FilteredAuditIssues.Reset();
	FilteredActorResults.Reset();
	SelectedActorResult.Reset();
	bDetailsPanelVisible = false;
	RefreshDetailsPanelItems();
	ErrorCount = 0;
	WarningCount = 0;
	InfoCount = 0;
	AuditedActorCount = 0;

	if (HasEnabledCriteria())
	{
		FOutlinerAuditRunRequest Request;
		Request.Scope = AuditScope;
		Request.EnabledCriteriaMask = EnabledCriteriaMask;
		Request.IgnoredIssueKeys = &IgnoredIssueKeys;

		FOutlinerAuditRunResult Result = FOutlinerAuditRunnerService::RunAudit(Request);
		AuditedActorCount = Result.AuditedActorCount;
		AllAuditIssues = MoveTemp(Result.Issues);
	}

	RefreshFilterOptions();
	ApplyResultFilters();
}

void SOutlinerToolkitAuditPanel::RefreshFilterOptions()
{
	TSet<FString> AvailableCategories;
	TSet<FString> AvailableIssueTypes;

	for (const FOutlinerAuditIssuePtr& Issue : AllAuditIssues)
	{
		if (!Issue.IsValid())
		{
			continue;
		}

		AvailableCategories.Add(Issue->Category.ToString());
		AvailableIssueTypes.Add(Issue->Issue.ToString());
	}

	TArray<FString> SortedCategories = AvailableCategories.Array();
	TArray<FString> SortedIssueTypes = AvailableIssueTypes.Array();
	SortedCategories.Sort();
	SortedIssueTypes.Sort();

	CategoryFilterOptions.Reset();
	for (const FString& Category : SortedCategories)
	{
		CategoryFilterOptions.Add(MakeShared<FString>(Category));
	}

	IssueTypeFilterOptions.Reset();
	for (const FString& IssueType : SortedIssueTypes)
	{
		IssueTypeFilterOptions.Add(MakeShared<FString>(IssueType));
	}

	if (!bCategoryFilterCustomized)
	{
		EnabledCategoryFilters = MoveTemp(AvailableCategories);
	}
	else
	{
		for (auto It = EnabledCategoryFilters.CreateIterator(); It; ++It)
		{
			if (!AvailableCategories.Contains(*It))
			{
				It.RemoveCurrent();
			}
		}
	}

	if (!bIssueTypeFilterCustomized)
	{
		EnabledIssueTypeFilters = MoveTemp(AvailableIssueTypes);
	}
	else
	{
		for (auto It = EnabledIssueTypeFilters.CreateIterator(); It; ++It)
		{
			if (!AvailableIssueTypes.Contains(*It))
			{
				It.RemoveCurrent();
			}
		}
	}
}

void SOutlinerToolkitAuditPanel::ApplyResultFilters()
{
	FilteredAuditIssues.Reset();
	FilteredActorResults.Reset();
	SelectedActorResult.Reset();
	bDetailsPanelVisible = false;
	RefreshDetailsPanelItems();

	for (const FOutlinerAuditIssuePtr& Issue : AllAuditIssues)
	{
		if (Issue.IsValid() && DoesIssuePassResultFilters(*Issue))
		{
			FilteredAuditIssues.Add(Issue);
		}
	}

	RebuildFilteredActorResults();
	RecalculateCounts();

	if (ActorListView.IsValid())
	{
		ActorListView->ClearSelection();
		ActorListView->RequestListRefresh();
	}
}

bool SOutlinerToolkitAuditPanel::DoesIssuePassResultFilters(const FOutlinerAuditIssue& Issue) const
{
	if (Issue.bIgnored && !bShowIgnoredIssues)
	{
		return false;
	}

	if (!IsSeverityFilterEnabled(Issue.Severity))
	{
		return false;
	}

	if (!EnabledCategoryFilters.Contains(Issue.Category.ToString()))
	{
		return false;
	}

	if (!EnabledIssueTypeFilters.Contains(Issue.Issue.ToString()))
	{
		return false;
	}

	const FString ActorNameFilter = ActorNameFilterText.ToString().TrimStartAndEnd();
	if (!ActorNameFilter.IsEmpty())
	{
		const FString ActorLabel = Issue.Actor.IsValid()
			? Issue.Actor->GetActorLabel()
			: FString(TEXT("<invalid>"));
		if (!ActorLabel.Contains(*ActorNameFilter, ESearchCase::IgnoreCase))
		{
			return false;
		}
	}

	return true;
}

void SOutlinerToolkitAuditPanel::RebuildFilteredActorResults()
{
	FOutlinerAuditRunnerService::RebuildActorResults(FilteredAuditIssues, FilteredActorResults);
}

void SOutlinerToolkitAuditPanel::RecalculateCounts()
{
	ErrorCount = 0;
	WarningCount = 0;
	InfoCount = 0;

	for (const FOutlinerAuditIssuePtr& Issue : FilteredAuditIssues)
	{
		if (!Issue.IsValid())
		{
			continue;
		}

		switch (Issue->Severity)
		{
		case EOutlinerAuditSeverity::Error:
			++ErrorCount;
			break;

		case EOutlinerAuditSeverity::Warning:
			++WarningCount;
			break;

		case EOutlinerAuditSeverity::Info:
			++InfoCount;
			break;
		}
	}
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildMenuBar()
{
	FMenuBarBuilder MenuBarBuilder(nullptr);

	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("PluginMenuLabel", "Plugin"),
		LOCTEXT("PluginMenuTooltip", "Plugin settings."),
		FNewMenuDelegate::CreateSP(this, &SOutlinerToolkitAuditPanel::FillPluginMenu));

	return MenuBarBuilder.MakeWidget();
}

void SOutlinerToolkitAuditPanel::FillPluginMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("Settings", LOCTEXT("PluginSettingsSectionLabel", "Settings"));
	MenuBuilder.AddMenuEntry(LOCTEXT("OpenOutlinerToolkitSettingsLabel", "Outliner Toolkit Settings"),
		LOCTEXT("OpenOutlinerToolkitSettingsTooltip", "Open Outliner Toolkit settings in Project Settings."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings"),
		FUIAction(FExecuteAction::CreateStatic( &FOutlinerMenuSectionRegistry::OpenPluginSettings)));
	MenuBuilder.EndSection();
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildCriteriaWidget()
{
	/* clang-format off */
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("CriteriaTitle", "Audit Criteria"))
							.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SComboButton)
						.ToolTipText(LOCTEXT("CriteriaComboTooltip", "Choose which audit criteria " "are enabled."))
						.ButtonContent()
						[
							SNew(STextBlock)
								.Text(this, &SOutlinerToolkitAuditPanel::GetCriteriaButtonText)
						]
						.MenuContent()
						[
							BuildCriteriaMenuContent()
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(24.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("ScopeTitle", "Scope"))
							.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						BuildAuditScopeSelector()
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					BuildFilterWidget()
				]
		];
	/* clang-format on */
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildFilterWidget()
{
	return	SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("FiltersTitle", "Filters"))
						.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[	
					SNew(SBox)
						.WidthOverride(180.0f)
						[
							SAssignNew(ActorNameSearchBox, SSearchBox)
							.HintText(LOCTEXT("ActorNameFilterHint", "Actor name"))
							.ToolTipText(LOCTEXT("ActorNameFilterTooltip","Filter listed issues by actor label."))
							.InitialText(ActorNameFilterText)
							.OnTextChanged(this, &SOutlinerToolkitAuditPanel::OnActorNameFilterTextChanged)
						]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SComboButton)
						.ToolTipText(LOCTEXT("SeverityFilterTooltip","Filter listed issues by severity."))
						.OnGetMenuContent(this,&SOutlinerToolkitAuditPanel::BuildSeverityFilterMenuContent)
						.ButtonContent()
						[
							SNew(STextBlock)
								.Text(this, &SOutlinerToolkitAuditPanel::GetSeverityFilterButtonText)
						]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SComboButton)
						.ToolTipText(LOCTEXT("CategoryFilterTooltip","Filter listed issues by category."))
						.OnGetMenuContent(this, &SOutlinerToolkitAuditPanel::BuildCategoryFilterMenuContent)
						.ButtonContent()
						[
							SNew(STextBlock)
								.Text(this, &SOutlinerToolkitAuditPanel::GetCategoryFilterButtonText)
						]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SComboButton)
						.ToolTipText(LOCTEXT("IssueTypeFilterTooltip","Filter listed issues by issue type."))
						.OnGetMenuContent(this,&SOutlinerToolkitAuditPanel::BuildIssueTypeFilterMenuContent)
						.ButtonContent()
						[
							SNew(STextBlock)
								.Text(this, &SOutlinerToolkitAuditPanel::GetIssueTypeFilterButtonText)
						]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
						.IsChecked(this, &SOutlinerToolkitAuditPanel::GetShowIgnoredCheckState)
						.OnCheckStateChanged(this, &SOutlinerToolkitAuditPanel::OnShowIgnoredCheckStateChanged)
						.ToolTipText(LOCTEXT("ShowIgnoredTooltip", "Show audit issues that were ignored persistently."))
						[
							SNew(STextBlock)
								.Text(LOCTEXT("ShowIgnoredLabel", "Show Ignored"))
						]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
						.Text(LOCTEXT("ClearFiltersButton", "Clear"))
						.ToolTipText(LOCTEXT("ClearFiltersTooltip", "Clear all result filters."))
						.IsEnabled(this, &SOutlinerToolkitAuditPanel::HasActiveResultFilters)
						.OnClicked(this, &SOutlinerToolkitAuditPanel::OnClearFiltersClicked)
				];
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildCriteriaMenuContent()
{
	TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

	MenuContent->AddSlot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f,4.0f)
		[
			SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[	
					SNew(SButton)
						.Text(LOCTEXT("EnableAllCriteriaButton", "All"))
						.ToolTipText(LOCTEXT("EnableAllCriteriaTooltip", "Enable every audit criterion."))
						.OnClicked(this, &SOutlinerToolkitAuditPanel::OnEnableAllCriteriaClicked)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
						.Text(LOCTEXT("DisableAllCriteriaButton", "None"))
						.ToolTipText(LOCTEXT("DisableAllCriteriaTooltip", "Disable every audit criterion."))
						.OnClicked(this, &SOutlinerToolkitAuditPanel::OnDisableAllCriteriaClicked)
				]
		];

	auto AddCriterion = [this, &MenuContent](EOutlinerAuditCriterion Criterion,
		const FText& Label,
		const FText& Tooltip) 
		{
			MenuContent->AddSlot()
				.AutoHeight()
				.Padding(8.0f, 2.0f)
				[
					BuildCriterionCheckBox(Criterion, Label, Tooltip)
				];
		};

	AddCriterion(EOutlinerAuditCriterion::TickEnabled,
		LOCTEXT("TickCriterion", "Tick"),
		LOCTEXT("TickCriterionTooltip",
			"Report actors with enabled actor tick."));
	AddCriterion(
		EOutlinerAuditCriterion::PhysicsEnabled,
		LOCTEXT("PhysicsCriterion", "Physics"),
		LOCTEXT("PhysicsCriterionTooltip",
			"Report actors with primitive components simulating physics."));
	AddCriterion(
		EOutlinerAuditCriterion::InvalidPhysicsMobility,
		LOCTEXT("PhysicsMobilityCriterion", "Physics Mobility"),
		LOCTEXT("PhysicsMobilityCriterionTooltip",
			"Report simulating primitive components that are not Movable."));
	AddCriterion(
		EOutlinerAuditCriterion::OverlapEvents,
		LOCTEXT("OverlapCriterion", "Overlap Events"),
		LOCTEXT("OverlapCriterionTooltip",
			"Report primitive components generating overlap events."));
	AddCriterion(
		EOutlinerAuditCriterion::MovableShadows,
		LOCTEXT("MovableShadowsCriterion", "Movable Shadows"),
		LOCTEXT("MovableShadowsCriterionTooltip",
			"Report movable primitive components that cast shadows."));
	AddCriterion(
		EOutlinerAuditCriterion::InvalidStaticMesh,
		LOCTEXT("InvalidStaticMeshCriterion", "Invalid Mesh"),
		LOCTEXT("InvalidStaticMeshCriterionTooltip",
			"Report StaticMeshComponents with no StaticMesh assigned."));
	AddCriterion(EOutlinerAuditCriterion::InvalidMaterials,
		LOCTEXT("InvalidMaterialsCriterion", "Invalid Materials"),
		LOCTEXT("InvalidMaterialsCriterionTooltip",
			"Report material slots that resolve to no material."));
	AddCriterion(EOutlinerAuditCriterion::TooManyMaterials,
		LOCTEXT("TooManyMaterialsCriterion", "Material Count"),
		LOCTEXT("TooManyMaterialsCriterionTooltip",
			"Report actors whose static mesh components exceed the "
			"material slot threshold."));
	AddCriterion(
		EOutlinerAuditCriterion::BadActorScale,
		LOCTEXT("BadActorScaleCriterion", "Actor Scale"),
		LOCTEXT("BadActorScaleCriterionTooltip",
			"Report actors with non-uniform or negative actor scale."));
	AddCriterion(EOutlinerAuditCriterion::BadComponentScale,
		LOCTEXT("BadComponentScaleCriterion", "Component Scale"),
		LOCTEXT("BadComponentScaleCriterionTooltip",
			"Report scene components with non-uniform or negative "
			"relative scale."));
	AddCriterion(
		EOutlinerAuditCriterion::TooManyComponents,
		LOCTEXT("TooManyComponentsCriterion", "Component Count"),
		LOCTEXT("TooManyComponentsCriterionTooltip",
			"Report actors exceeding the component count threshold."));
	AddCriterion(
		EOutlinerAuditCriterion::NoFolder,
		LOCTEXT("NoFolderCriterion", "No Folder"),
		LOCTEXT("NoFolderCriterionTooltip",
			"Report actors that are not assigned to an Outliner folder."));
	AddCriterion(EOutlinerAuditCriterion::EditorOnly,
		LOCTEXT("EditorOnlyCriterion", "Editor Only"),
		LOCTEXT("EditorOnlyCriterionTooltip",
			"Report actors marked as editor-only."));

	return SNew(SBox).WidthOverride(260.0f)
		[
			MenuContent
		];
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildCriterionCheckBox(EOutlinerAuditCriterion Criterion, const FText& Label, const FText& Tooltip)
{
	return SNew(SCheckBox)
		.ToolTipText(Tooltip)
		.IsChecked(this, &SOutlinerToolkitAuditPanel::GetCriterionCheckState, Criterion)
		.OnCheckStateChanged(this, &SOutlinerToolkitAuditPanel::OnCriterionCheckStateChanged, Criterion)
		[
			SNew(STextBlock).Text(Label)
		];
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildSeverityFilterMenuContent()
{
	TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

	MenuContent->AddSlot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth().Padding(0.0f, 0.0f, 4.0f,0.0f)
			[
				SNew(SButton)
					.Text(LOCTEXT("EnableAllSeverityFiltersButton", "All"))
					.ToolTipText(LOCTEXT("EnableAllSeverityFiltersTooltip","Show every severity."))
					.OnClicked(this, &SOutlinerToolkitAuditPanel::OnEnableAllSeverityFiltersClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
					.Text(LOCTEXT("DisableAllSeverityFiltersButton", "None"))
					.ToolTipText(LOCTEXT("DisableAllSeverityFiltersTooltip", "Hide every severity."))
					.OnClicked(this, &SOutlinerToolkitAuditPanel::OnDisableAllSeverityFiltersClicked)
			]
		];

	auto AddSeverity = [this, &MenuContent](EOutlinerAuditSeverity Severity) 
		{
			MenuContent->AddSlot()
				.AutoHeight()
				.Padding(8.0f, 2.0f)
				[
					SNew(SCheckBox)
						.IsChecked(this, &SOutlinerToolkitAuditPanel::GetSeverityFilterCheckState, Severity)
						.OnCheckStateChanged(this, &SOutlinerToolkitAuditPanel::OnSeverityFilterCheckStateChanged, Severity)
						[
							SNew(STextBlock)
								.Text(FOutlinerAuditReportFormatter::GetSeverityText(Severity))
								.ColorAndOpacity(FOutlinerAuditReportFormatter::GetSeverityColor(Severity))
						]
				];
		};

	AddSeverity(EOutlinerAuditSeverity::Error);
	AddSeverity(EOutlinerAuditSeverity::Warning);
	AddSeverity(EOutlinerAuditSeverity::Info);

	return SNew(SBox).WidthOverride(180.0f)
		[
			MenuContent
		];
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildCategoryFilterMenuContent()
{
	TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

	MenuContent->AddSlot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f,0.0f)
			[
				SNew(SButton)
					.Text(LOCTEXT("EnableAllCategoryFiltersButton", "All"))
					.ToolTipText(LOCTEXT("EnableAllCategoryFiltersTooltip", "Show every category."))
					.OnClicked(this, &SOutlinerToolkitAuditPanel::OnEnableAllCategoryFiltersClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
					.Text(LOCTEXT("DisableAllCategoryFiltersButton", "None"))
					.ToolTipText(LOCTEXT("DisableAllCategoryFiltersTooltip", "Hide every category."))
					.OnClicked(this, &SOutlinerToolkitAuditPanel::OnDisableAllCategoryFiltersClicked)
			]
		];

	if (CategoryFilterOptions.IsEmpty())
	{
		MenuContent->AddSlot()
			.AutoHeight()
			.Padding(8.0f,4.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoCategoryFiltersAvailable", "No categories"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
	}

	for (const TSharedPtr<FString>& Category : CategoryFilterOptions)
	{
		MenuContent->AddSlot()
			.AutoHeight()
			.Padding(8.0f, 2.0f)
			[
				SNew(SCheckBox)
					.IsChecked(this, &SOutlinerToolkitAuditPanel::GetCategoryFilterCheckState, Category)
					.OnCheckStateChanged(this,&SOutlinerToolkitAuditPanel::OnCategoryFilterCheckStateChanged,Category)
					[
						SNew(STextBlock)
							.Text(Category.IsValid() ? FText::FromString(*Category) : FText::GetEmpty())
					]
			];
	}

	return SNew(SBox).WidthOverride(220.0f)
		[
			MenuContent
		];
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildIssueTypeFilterMenuContent()
{
	TSharedRef<SVerticalBox> MenuContent = SNew(SVerticalBox);

	MenuContent->AddSlot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 4.0f,0.0f)
			[
				SNew(SButton)
					.Text(LOCTEXT("EnableAllIssueTypeFiltersButton", "All"))
					.ToolTipText(LOCTEXT("EnableAllIssueTypeFiltersTooltip", "Show every issue type."))
					.OnClicked(this, &SOutlinerToolkitAuditPanel::OnEnableAllIssueTypeFiltersClicked)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
					.Text(LOCTEXT("DisableAllIssueTypeFiltersButton", "None"))
					.ToolTipText(LOCTEXT("DisableAllIssueTypeFiltersTooltip", "Hide every issue type."))
					.OnClicked(this, &SOutlinerToolkitAuditPanel::OnDisableAllIssueTypeFiltersClicked)
			]
		];

	if (IssueTypeFilterOptions.IsEmpty())
	{
		MenuContent->AddSlot()
			.AutoHeight()
			.Padding(8.0f, 4.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("NoIssueTypeFiltersAvailable","No issue types"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
	}

	for (const TSharedPtr<FString>& IssueType : IssueTypeFilterOptions)
	{
		MenuContent->AddSlot()
			.AutoHeight()
			.Padding(8.0f,2.0f)
			[
				SNew(SCheckBox)
					.IsChecked(this,&SOutlinerToolkitAuditPanel::GetIssueTypeFilterCheckState,IssueType)
					.OnCheckStateChanged(this,&SOutlinerToolkitAuditPanel::OnIssueTypeFilterCheckStateChanged,IssueType)
					[
						SNew(STextBlock)
							.Text(IssueType.IsValid() ? FText::FromString(*IssueType) : FText::GetEmpty())
					]
			];
	}

	return SNew(SBox).WidthOverride(260.0f)
		[
			MenuContent
		];
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildAuditScopeSelector()
{
	return SNew(SBox)
		.WidthOverride(160.0f)
		[
			SNew(SComboBox<TSharedPtr<EOutlinerAuditScope>>)
				.OptionsSource(&AuditScopeOptions)
				.InitiallySelectedItem(SelectedAuditScopeOption)
				.OnGenerateWidget(this, &SOutlinerToolkitAuditPanel::MakeAuditScopeOptionWidget)
				.OnSelectionChanged(this,&SOutlinerToolkitAuditPanel::OnAuditScopeChanged)
				.ToolTipText(LOCTEXT("AuditScopeTooltip", "Choose which actors are included in the audit scan."))
				[
					SNew(STextBlock)
						.Text(this,&SOutlinerToolkitAuditPanel::GetAuditScopeText)
				]
		];
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::MakeAuditScopeOptionWidget(TSharedPtr<EOutlinerAuditScope> ScopeOption) const
{
	return SNew(STextBlock).Text(ScopeOption.IsValid() ? FOutlinerAuditReportFormatter::GetAuditScopeText(*ScopeOption.Get()) : FText::GetEmpty());
}

TSharedRef<SHeaderRow> SOutlinerToolkitAuditPanel::BuildHeaderRow() const
{
	return SNew(SHeaderRow) 
		
		+ SHeaderRow::Column(OutlinerAuditColumns::Severity)
		.FixedWidth(80.0f)
		.DefaultLabel(LOCTEXT("SeverityColumn", "Severity"))

		+ SHeaderRow::Column(OutlinerAuditColumns::Actor)
		.FillWidth(0.26f)
		.DefaultLabel(LOCTEXT("ActorColumn", "Actor"))

		+ SHeaderRow::Column(OutlinerAuditColumns::Category)
		.FillWidth(0.20f)
		.DefaultLabel(LOCTEXT("CategoriesColumn", "Categories"))

		+ SHeaderRow::Column(OutlinerAuditColumns::Issue)
		.FixedWidth(95.0f)
		.DefaultLabel(LOCTEXT("IssuesColumn", "Issues"))

		+ SHeaderRow::Column(OutlinerAuditColumns::Details)
		.FillWidth(0.54f)
		.DefaultLabel(LOCTEXT("SummaryColumn", "Summary"))

		+ SHeaderRow::Column(OutlinerAuditColumns::Fix)
		.FixedWidth(95.0f)
		.DefaultLabel(LOCTEXT("FixableColumn", "Fixable"));
}

TSharedRef<SWidget> SOutlinerToolkitAuditPanel::BuildDetailsPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("DetailsPanelTitle", "Actor Details"))
							.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
							.Text(LOCTEXT("CloseDetailsPanelButton", "Close"))
							.ToolTipText(LOCTEXT("CloseDetailsPanelTooltip", "Close the actor details panel."))
							.OnClicked(this, &SOutlinerToolkitAuditPanel::OnCloseDetailsPanelClicked)
					]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
					.Text(this, &SOutlinerToolkitAuditPanel::GetDetailsPanelActorText)
					.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
					.AutoWrapText(true)
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
					.Text(this, &SOutlinerToolkitAuditPanel::GetDetailsPanelIssueText)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.AutoWrapText(true)]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
					.Text(this, &SOutlinerToolkitAuditPanel::GetDetailsPanelSummaryText)
					.AutoWrapText(true)]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SSeparator)
					.Orientation(EOrientation::Orient_Horizontal)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("DetailsPanelRowsTitle", "Issues"))
					.Font(FAppStyle::Get().GetFontStyle("NormalFontBold"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("DetailsPanelNoRows", "No visible issues for this actor."))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.AutoWrapText(true)
					.Visibility(this, &SOutlinerToolkitAuditPanel::GetNoDetailsRowsVisibility)
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(ActorIssueDetailListView, SListView<FOutlinerAuditIssuePtr>)
					.ListItemsSource(&SelectedActorIssues)
					.SelectionMode(ESelectionMode::None)
					.OnGenerateRow(this, &SOutlinerToolkitAuditPanel::OnGenerateActorIssueDetailRow)
			]
		];
}

TSharedRef<ITableRow> SOutlinerToolkitAuditPanel::OnGenerateActorRow(FOutlinerAuditActorResultPtr InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SOutlinerAuditActorRow, OwnerTable).ActorResult(InItem);
}

TSharedRef<ITableRow> SOutlinerToolkitAuditPanel::OnGenerateActorIssueDetailRow(FOutlinerAuditIssuePtr InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SOutlinerAuditActorIssueDetailRow, OwnerTable).Issue(InItem);
}

/* refactoring without in MenuSection for actor file */
TSharedPtr<SWidget> SOutlinerToolkitAuditPanel::BuildActorContextMenu()
{
	if (!HasSelectedActorData())
	{
		return nullptr;
	}

	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection("Actor", LOCTEXT("ActorContextActorSection", "Actor"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextSelectActor", "Select Actor"),
		LOCTEXT("ActorContextSelectActorTooltip", "Select this audit actor."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.SelectInViewport"),
		FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::SelectSelectedActor),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActor)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextFocusActor", "Focus Actor"),
		LOCTEXT("ActorContextFocusActorTooltip",
			"Select and frame this audit actor."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"),
		FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::FocusSelectedActor),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActor)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextOpenActor", "Open Actor Blueprint"),
		LOCTEXT("ActorContextOpenActorTooltip",
			"Open this actor's Blueprint asset."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
		FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::OpenSelectedActor),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorBlueprint)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextOpenStaticMeshes", "Open Static Meshes"),
		LOCTEXT("ActorContextOpenStaticMeshesTooltip",
			"Open every unique Static Mesh asset used by this actor."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.StaticMesh"),
		FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::OpenSelectedActorStaticMeshes),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorStaticMeshes)));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Details", LOCTEXT("ActorContextDetailsSection", "Details"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextShowDetails", "Show Details"),
		LOCTEXT("ActorContextShowDetailsTooltip", "Open the details panel for every visible issue on this actor."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::ShowSelectedActorDetails),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorData)));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Copy", LOCTEXT("ActorContextCopySection", "Copy"));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextCopyActorPath", "Copy Actor Path"),
		LOCTEXT("ActorContextCopyActorPathTooltip", "Copy the actor object path to the clipboard."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::CopySelectedActorPath),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActor)));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextCopyIssues", "Copy Actor Issues"),
		LOCTEXT("ActorContextCopyIssuesTooltip", "Copy every visible audit issue for this actor to the clipboard."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::CopySelectedActorIssuesText),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorData)));
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("Actions",
		LOCTEXT("ActorContextActionsSection", "Actions"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextIgnore", "Ignore Actor Issues"),
		LOCTEXT("ActorContextIgnoreTooltip", "Persistently hide every visible issue for this actor."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Hidden"),
		FUIAction(
			FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::IgnoreSelectedActorIssues),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorData)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextRestore", "Restore Actor Issues"),
		LOCTEXT("ActorContextRestoreTooltip",
			"Remove persistent ignore for every ignored issue on this actor."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visible"),
		FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::RestoreSelectedActorIssues),
			FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasRestorableSelectedActorIssues)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ActorContextFix", "Fix Actor Issues"),
		LOCTEXT("ActorContextFixTooltip",
			"Apply every automatic fix available for this actor."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
		FUIAction(
			FExecuteAction::CreateSP(
				this, &SOutlinerToolkitAuditPanel::FixSelectedActorIssues),
			FCanExecuteAction::CreateSP(
				this,
				&SOutlinerToolkitAuditPanel::HasFixableSelectedActorIssues)));

	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SOutlinerToolkitAuditPanel::OnActorSelectionChanged(FOutlinerAuditActorResultPtr InItem, ESelectInfo::Type SelectInfo)
{
	SelectedActorResult = InItem;
	if (bDetailsPanelVisible)
	{
		RefreshDetailsPanelItems();
	}
}

void SOutlinerToolkitAuditPanel::OnActorDoubleClicked(FOutlinerAuditActorResultPtr InItem)
{
	if (InItem.IsValid() && InItem->Actor.IsValid())
	{
		FOutlinerAuditEditorService::SelectAndFocusActor(InItem->Actor.Get());
	}
}

void SOutlinerToolkitAuditPanel::ShowSelectedActorDetails()
{
	if (!SelectedActorResult.IsValid())
	{
		return;
	}

	bDetailsPanelVisible = true;
	RefreshDetailsPanelItems();
}

FReply SOutlinerToolkitAuditPanel::OnCloseDetailsPanelClicked()
{
	bDetailsPanelVisible = false;
	SelectedActorIssues.Reset();
	if (ActorIssueDetailListView.IsValid())
	{
		ActorIssueDetailListView->RequestListRefresh();
	}

	return FReply::Handled();
}

void SOutlinerToolkitAuditPanel::RefreshDetailsPanelItems()
{
	SelectedActorIssues.Reset();

	if (SelectedActorResult.IsValid())
	{
		SelectedActorIssues.Reserve(SelectedActorResult->Issues.Num());
		for (const FOutlinerAuditIssuePtr& Issue : SelectedActorResult->Issues)
		{
			if (Issue.IsValid())
			{
				SelectedActorIssues.Add(Issue);
			}
		}
	}

	if (ActorIssueDetailListView.IsValid())
	{
		ActorIssueDetailListView->RequestListRefresh();
	}
}

FReply SOutlinerToolkitAuditPanel::OnRunAuditClicked()
{
	RefreshAudit();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnEnableAllCriteriaClicked()
{
	EnabledCriteriaMask = OutlinerAuditAllCriteriaMask;
	RefreshAudit();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnDisableAllCriteriaClicked()
{
	EnabledCriteriaMask = 0;
	RefreshAudit();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnSelectActorClicked() const
{
	SelectSelectedActor();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnSelectAllActorsClicked() const
{
	FOutlinerAuditEditorService::SelectActors(FOutlinerAuditActorHelpers::ExtractActors(FilteredActorResults));
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnFixSelectedActorClicked()
{
	FixSelectedActorIssues();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnIgnoreSelectedActorClicked()
{
	IgnoreSelectedActorIssues();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnRestoreSelectedActorClicked()
{
	RestoreSelectedActorIssues();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnEnableAllSeverityFiltersClicked()
{
	EnabledSeverityFilterMask = OutlinerAuditAllSeverityFilterMask;
	ApplyResultFilters();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnDisableAllSeverityFiltersClicked()
{
	EnabledSeverityFilterMask = 0;
	ApplyResultFilters();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnEnableAllCategoryFiltersClicked()
{
	bCategoryFilterCustomized = false;
	EnabledCategoryFilters.Reset();
	for (const TSharedPtr<FString>& Category : CategoryFilterOptions)
	{
		if (Category.IsValid())
		{
			EnabledCategoryFilters.Add(*Category);
		}
	}

	ApplyResultFilters();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnDisableAllCategoryFiltersClicked()
{
	bCategoryFilterCustomized = true;
	EnabledCategoryFilters.Reset();
	ApplyResultFilters();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnEnableAllIssueTypeFiltersClicked()
{
	bIssueTypeFilterCustomized = false;
	EnabledIssueTypeFilters.Reset();
	for (const TSharedPtr<FString>& IssueType : IssueTypeFilterOptions)
	{
		if (IssueType.IsValid())
		{
			EnabledIssueTypeFilters.Add(*IssueType);
		}
	}

	ApplyResultFilters();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnDisableAllIssueTypeFiltersClicked()
{
	bIssueTypeFilterCustomized = true;
	EnabledIssueTypeFilters.Reset();
	ApplyResultFilters();
	return FReply::Handled();
}

FReply SOutlinerToolkitAuditPanel::OnClearFiltersClicked()
{
	ActorNameFilterText = FText::GetEmpty();
	EnabledSeverityFilterMask = OutlinerAuditAllSeverityFilterMask;
	bCategoryFilterCustomized = false;
	bIssueTypeFilterCustomized = false;
	bShowIgnoredIssues = false;

	EnabledCategoryFilters.Reset();
	for (const TSharedPtr<FString>& Category : CategoryFilterOptions)
	{
		if (Category.IsValid())
		{
			EnabledCategoryFilters.Add(*Category);
		}
	}

	EnabledIssueTypeFilters.Reset();
	for (const TSharedPtr<FString>& IssueType : IssueTypeFilterOptions)
	{
		if (IssueType.IsValid())
		{
			EnabledIssueTypeFilters.Add(*IssueType);
		}
	}

	if (ActorNameSearchBox.IsValid())
	{
		ActorNameSearchBox->SetText(ActorNameFilterText);
	}

	ApplyResultFilters();
	return FReply::Handled();
}

ECheckBoxState SOutlinerToolkitAuditPanel::GetCriterionCheckState(EOutlinerAuditCriterion Criterion) const
{
	return IsCriterionEnabled(Criterion) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SOutlinerToolkitAuditPanel::OnCriterionCheckStateChanged(ECheckBoxState NewState, EOutlinerAuditCriterion Criterion)
{
	SetCriterionEnabled(Criterion, NewState == ECheckBoxState::Checked);
	RefreshAudit();
}

bool SOutlinerToolkitAuditPanel::IsCriterionEnabled(EOutlinerAuditCriterion Criterion) const
{
	return (EnabledCriteriaMask & FOutlinerAuditBitmask::CriterionToMask(Criterion)) != 0;
}

void SOutlinerToolkitAuditPanel::SetCriterionEnabled(EOutlinerAuditCriterion Criterion, bool bEnabled)
{
	const uint32 CriterionMask = FOutlinerAuditBitmask::CriterionToMask(Criterion);
	if (bEnabled)
	{
		EnabledCriteriaMask |= CriterionMask;
	}
	else
	{
		EnabledCriteriaMask &= ~CriterionMask;
	}
}

bool SOutlinerToolkitAuditPanel::HasEnabledCriteria() const
{
	return (EnabledCriteriaMask & OutlinerAuditAllCriteriaMask) != 0;
}

int32 SOutlinerToolkitAuditPanel::CountEnabledCriteria() const
{
	int32 EnabledCount = 0;
	for (uint8 CriterionIndex = 0;
		CriterionIndex < static_cast<uint8>(EOutlinerAuditCriterion::Count);
		++CriterionIndex)
	{
		const EOutlinerAuditCriterion Criterion =
			static_cast<EOutlinerAuditCriterion>(CriterionIndex);
		if (IsCriterionEnabled(Criterion))
		{
			++EnabledCount;
		}
	}

	return EnabledCount;
}

FText SOutlinerToolkitAuditPanel::GetCriteriaButtonText() const
{
	const int32 EnabledCount = CountEnabledCriteria();
	const int32 TotalCount = static_cast<int32>(EOutlinerAuditCriterion::Count);

	if (EnabledCount == 0)
	{
		return LOCTEXT("CriteriaButtonNone", "No criteria");
	}

	if (EnabledCount == TotalCount)
	{
		return LOCTEXT("CriteriaButtonAll", "All criteria");
	}

	return FText::Format(LOCTEXT("CriteriaButtonPartial", "{0}/{1} criteria"),
		FText::AsNumber(EnabledCount),
		FText::AsNumber(TotalCount));
}

ECheckBoxState SOutlinerToolkitAuditPanel::GetSeverityFilterCheckState(EOutlinerAuditSeverity Severity) const
{
	return IsSeverityFilterEnabled(Severity) ? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

void SOutlinerToolkitAuditPanel::OnSeverityFilterCheckStateChanged(ECheckBoxState NewState, EOutlinerAuditSeverity Severity)
{
	SetSeverityFilterEnabled(Severity, NewState == ECheckBoxState::Checked);
	ApplyResultFilters();
}

ECheckBoxState SOutlinerToolkitAuditPanel::GetCategoryFilterCheckState(TSharedPtr<FString> Category) const
{
	return Category.IsValid() && EnabledCategoryFilters.Contains(*Category)
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

void SOutlinerToolkitAuditPanel::OnCategoryFilterCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FString> Category)
{
	if (!Category.IsValid())
	{
		return;
	}

	bCategoryFilterCustomized = true;
	if (NewState == ECheckBoxState::Checked)
	{
		EnabledCategoryFilters.Add(*Category);
	}
	else
	{
		EnabledCategoryFilters.Remove(*Category);
	}

	ApplyResultFilters();
}

ECheckBoxState SOutlinerToolkitAuditPanel::GetIssueTypeFilterCheckState(TSharedPtr<FString> IssueType) const
{
	return IssueType.IsValid() && EnabledIssueTypeFilters.Contains(*IssueType)
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

void SOutlinerToolkitAuditPanel::OnIssueTypeFilterCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FString> IssueType)
{
	if (!IssueType.IsValid())
	{
		return;
	}

	bIssueTypeFilterCustomized = true;
	if (NewState == ECheckBoxState::Checked)
	{
		EnabledIssueTypeFilters.Add(*IssueType);
	}
	else
	{
		EnabledIssueTypeFilters.Remove(*IssueType);
	}

	ApplyResultFilters();
}

void SOutlinerToolkitAuditPanel::OnActorNameFilterTextChanged(const FText& NewText)
{
	ActorNameFilterText = NewText;
	ApplyResultFilters();
}

ECheckBoxState SOutlinerToolkitAuditPanel::GetShowIgnoredCheckState() const
{
	return bShowIgnoredIssues ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SOutlinerToolkitAuditPanel::OnShowIgnoredCheckStateChanged(ECheckBoxState NewState)
{
	bShowIgnoredIssues = NewState == ECheckBoxState::Checked;
	ApplyResultFilters();
}

bool SOutlinerToolkitAuditPanel::IsSeverityFilterEnabled(EOutlinerAuditSeverity Severity) const
{
	return (EnabledSeverityFilterMask & FOutlinerAuditBitmask::SeverityToMask(Severity)) != 0;
}

void SOutlinerToolkitAuditPanel::SetSeverityFilterEnabled(EOutlinerAuditSeverity Severity, bool bEnabled)
{
	const uint8 SeverityMask = FOutlinerAuditBitmask::SeverityToMask(Severity);
	if (bEnabled)
	{
		EnabledSeverityFilterMask |= SeverityMask;
	}
	else
	{
		EnabledSeverityFilterMask &= static_cast<uint8>(~SeverityMask);
	}
}

int32 SOutlinerToolkitAuditPanel::CountEnabledSeverityFilters() const
{
	int32 EnabledCount = 0;
	if (IsSeverityFilterEnabled(EOutlinerAuditSeverity::Error))
	{
		++EnabledCount;
	}
	if (IsSeverityFilterEnabled(EOutlinerAuditSeverity::Warning))
	{
		++EnabledCount;
	}
	if (IsSeverityFilterEnabled(EOutlinerAuditSeverity::Info))
	{
		++EnabledCount;
	}

	return EnabledCount;
}

/* */
bool SOutlinerToolkitAuditPanel::HasActiveResultFilters() const
{
	return !ActorNameFilterText.ToString().TrimStartAndEnd().IsEmpty() 
		|| bShowIgnoredIssues 
		|| EnabledSeverityFilterMask != OutlinerAuditAllSeverityFilterMask 
		|| OutlinerAuditUtils::CountEnabledStringFilterOptions(CategoryFilterOptions, EnabledCategoryFilters) != CategoryFilterOptions.Num() 
		|| OutlinerAuditUtils::CountEnabledStringFilterOptions(IssueTypeFilterOptions, EnabledIssueTypeFilters) != IssueTypeFilterOptions.Num();
}

FText SOutlinerToolkitAuditPanel::GetSeverityFilterButtonText() const
{
	const int32 EnabledCount = CountEnabledSeverityFilters();
	constexpr int32 TotalCount = 3;

	if (EnabledCount == 0)
	{
		return LOCTEXT("SeverityFilterNone", "No severities");
	}

	if (EnabledCount == TotalCount)
	{
		return LOCTEXT("SeverityFilterAll", "All severities");
	}

	return FText::Format(LOCTEXT("SeverityFilterPartial", "{0}/{1} severities"),
		FText::AsNumber(EnabledCount),
		FText::AsNumber(TotalCount));
}

FText SOutlinerToolkitAuditPanel::GetCategoryFilterButtonText() const
{
	const int32 TotalCount = CategoryFilterOptions.Num();
	if (TotalCount == 0)
	{
		return LOCTEXT("CategoryFilterEmpty", "No categories");
	}

	const int32 EnabledCount = OutlinerAuditUtils::CountEnabledStringFilterOptions(CategoryFilterOptions, EnabledCategoryFilters);
	if (EnabledCount == 0)
	{
		return LOCTEXT("CategoryFilterNone", "No categories");
	}

	if (EnabledCount == TotalCount)
	{
		return LOCTEXT("CategoryFilterAll", "All categories");
	}

	return FText::Format(LOCTEXT("CategoryFilterPartial", "{0}/{1} categories"),
		FText::AsNumber(EnabledCount),
		FText::AsNumber(TotalCount));
}

FText SOutlinerToolkitAuditPanel::GetIssueTypeFilterButtonText() const
{
	const int32 TotalCount = IssueTypeFilterOptions.Num();
	if (TotalCount == 0)
	{
		return LOCTEXT("IssueTypeFilterEmpty", "No issue types");
	}

	const int32 EnabledCount = OutlinerAuditUtils::CountEnabledStringFilterOptions(IssueTypeFilterOptions, EnabledIssueTypeFilters);
	if (EnabledCount == 0)
	{
		return LOCTEXT("IssueTypeFilterNone", "No issue types");
	}

	if (EnabledCount == TotalCount)
	{
		return LOCTEXT("IssueTypeFilterAll", "All issue types");
	}

	return FText::Format(LOCTEXT("IssueTypeFilterPartial", "{0}/{1} issue types"),
		FText::AsNumber(EnabledCount),
		FText::AsNumber(TotalCount));
}

bool SOutlinerToolkitAuditPanel::HasSelectedActorData() const
{
	return SelectedActorResult.IsValid();
}

bool SOutlinerToolkitAuditPanel::HasSelectedActor() const
{
	return HasSelectedActorData() && SelectedActorResult->Actor.IsValid();
}

bool SOutlinerToolkitAuditPanel::HasSelectedActorBlueprint() const
{
	return IsValid(GetSelectedActorBlueprintAsset());
}

bool SOutlinerToolkitAuditPanel::HasSelectedActorStaticMeshes() const
{
	TArray<UStaticMesh*> StaticMeshes;
	GatherSelectedActorStaticMeshes(StaticMeshes);
	return !StaticMeshes.IsEmpty();
}

bool SOutlinerToolkitAuditPanel::HasFixableSelectedActorIssues() const
{
	return HasSelectedActorData() && SelectedActorResult->FixableIssueCount > 0;
}

bool SOutlinerToolkitAuditPanel::HasRestorableSelectedActorIssues() const
{
	return HasSelectedActorData() && SelectedActorResult->IgnoredIssueCount > 0;
}

EVisibility SOutlinerToolkitAuditPanel::GetDetailsPanelVisibility() const
{
	return bDetailsPanelVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SOutlinerToolkitAuditPanel::GetNoDetailsRowsVisibility() const
{
	return SelectedActorIssues.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SOutlinerToolkitAuditPanel::GetDetailsPanelActorText() const
{
	return HasSelectedActorData()
		? FOutlinerAuditReportFormatter::GetActorLabelText(SelectedActorResult->Actor)
		: LOCTEXT("DetailsPanelNoActor", "No actor selected");
}

FText SOutlinerToolkitAuditPanel::GetDetailsPanelIssueText() const
{
	return FOutlinerAuditReportFormatter::GetDetailsPanelIssueText(SelectedActorResult);
}

FText SOutlinerToolkitAuditPanel::GetDetailsPanelSummaryText() const
{
	return FOutlinerAuditReportFormatter::GetDetailsPanelSummaryText(SelectedActorResult);
}

FText SOutlinerToolkitAuditPanel::GetSummaryText() const
{
	if (!HasEnabledCriteria())
	{
		return LOCTEXT("NoCriteriaSummary", "No audit criteria enabled.");
	}

	if (AuditedActorCount == 0)
	{
		return FText::Format(
			LOCTEXT("NoActorsInScopeSummary", "No actors in {0} scope."),
			GetAuditScopeText());
	}

	if (AllAuditIssues.IsEmpty())
	{
		return FText::Format(LOCTEXT("NoIssuesSummary",
			"No audit issues found in {0} ({1} actors)."),
			GetAuditScopeText(),
			FText::AsNumber(AuditedActorCount));
	}

	if (FilteredAuditIssues.IsEmpty())
	{
		return FText::Format(
			LOCTEXT("NoFilteredIssuesSummary",
				"No issues match filters ({0} total in {1}, {2} actors)."),
			FText::AsNumber(AllAuditIssues.Num()), GetAuditScopeText(),
			FText::AsNumber(AuditedActorCount));
	}

	if (HasActiveResultFilters())
	{
		return FText::Format(
			LOCTEXT("FilteredAuditSummary",
				"{0} actors / {1} issues shown ({2} total issues in {3}, {4} "
				"actors scanned): {5} errors, {6} warnings, {7} info"),
			FText::AsNumber(FilteredActorResults.Num()),
			FText::AsNumber(FilteredAuditIssues.Num()),
			FText::AsNumber(AllAuditIssues.Num()), GetAuditScopeText(),
			FText::AsNumber(AuditedActorCount), FText::AsNumber(ErrorCount),
			FText::AsNumber(WarningCount), FText::AsNumber(InfoCount));
	}

	return FText::Format(
		LOCTEXT("AuditSummary", "{0} actors / {1} issues in {2} ({3} actors "
			"scanned): {4} errors, {5} warnings, {6} info"),
		FText::AsNumber(FilteredActorResults.Num()),
		FText::AsNumber(FilteredAuditIssues.Num()), GetAuditScopeText(),
		FText::AsNumber(AuditedActorCount), FText::AsNumber(ErrorCount),
		FText::AsNumber(WarningCount), FText::AsNumber(InfoCount));
}

FText SOutlinerToolkitAuditPanel::GetAuditScopeText() const
{
	return FOutlinerAuditReportFormatter::GetAuditScopeText(AuditScope);
}

void SOutlinerToolkitAuditPanel::OnAuditScopeChanged(TSharedPtr<EOutlinerAuditScope> NewScope, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;

	if (!NewScope.IsValid())
	{
		return;
	}

	SelectedAuditScopeOption = NewScope;
	const EOutlinerAuditScope NewAuditScope = *NewScope.Get();
	if (AuditScope == NewAuditScope)
	{
		return;
	}

	AuditScope = NewAuditScope;
	RefreshAudit();
}

void SOutlinerToolkitAuditPanel::SelectSelectedActor() const 
{
	if (!SelectedActorResult.IsValid())
	{
		return;
	}

	FOutlinerAuditEditorService::SelectActor(SelectedActorResult->Actor.Get());
}

void SOutlinerToolkitAuditPanel::FocusSelectedActor() const
{
	if (HasSelectedActor())
	{
		FOutlinerAuditEditorService::SelectAndFocusActor(SelectedActorResult->Actor.Get());
	}
}

void SOutlinerToolkitAuditPanel::OpenSelectedActor() const
{
	if (!GEditor)
	{
		return;
	}

	UObject* BlueprintAsset = GetSelectedActorBlueprintAsset();
	if (!IsValid(BlueprintAsset))
	{
		return;
	}

	if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		AssetEditorSubsystem->OpenEditorForAsset(BlueprintAsset);
		return;
	}

	GEditor->EditObject(BlueprintAsset);
}

void SOutlinerToolkitAuditPanel::OpenSelectedActorStaticMeshes() const
{
	if (!SelectedActorResult.IsValid() || !SelectedActorResult->Actor.IsValid())
	{
		return;
	}

	AActor* Actor = SelectedActorResult->Actor.Get();
	FOutlinerAuditEditorService::OpenActorStaticMeshes(Actor);
}

void SOutlinerToolkitAuditPanel::CopySelectedActorPath() const
{
	if (!SelectedActorResult.IsValid() || !SelectedActorResult->Actor.IsValid())
	{
		return;
	}

	const FString ActorPath = SelectedActorResult->Actor->GetPathName();
	FOutlinerAuditEditorService::CopyToClipboard(ActorPath);
}

void SOutlinerToolkitAuditPanel::CopySelectedActorIssuesText() const
{
	if (!SelectedActorResult.IsValid())
	{
		return;
	}

	//const FString ActorLabel = SelectedActorResult->Actor.IsValid()
	//	? SelectedActorResult->Actor->GetActorLabel()
	//	: FString(TEXT("<invalid>"));

	//const FString ActorPath = SelectedActorResult->Actor.IsValid()
	//	? SelectedActorResult->Actor->GetPathName()
	//	: FString(TEXT("<invalid>"));

	//FString IssueText =
	//	FString::Printf(TEXT("Actor: %s\r\nPath: %s\r\nWorst Severity: "
	//		"%s\r\nVisible Issues: %d\r\nFixable Issues: %d"),
	//		*ActorLabel, *ActorPath,
	//		*OutlinerAuditUtils::GetSeverityText(
	//			SelectedActorResult->WorstSeverity)
	//		.ToString(),
	//		SelectedActorResult->Issues.Num(),
	//		SelectedActorResult->FixableIssueCount);

	//for (const FOutlinerAuditIssuePtr& Issue : SelectedActorResult->Issues)
	//{
	//	if (!Issue.IsValid())
	//	{
	//		continue;
	//	}

	//	IssueText += FString::Printf(
	//		TEXT("\r\n\r\n[%s%s] %s / %s\r\n%s"),
	//		*OutlinerAuditUtils::GetSeverityText(Issue->Severity)
	//		.ToString(),
	//		Issue->bIgnored ? TEXT(", ignored") : TEXT(""),
	//		*Issue->Category.ToString(), *Issue->Issue.ToString(),
	//		*Issue->Details.ToString());

	//	for (const FOutlinerAuditDetailEntry& DetailEntry : Issue->DetailEntries)
	//	{
	//		IssueText +=
	//			FString::Printf(TEXT("\r\n- %s: %s"), *DetailEntry.Subject.ToString(),
	//				*DetailEntry.Detail.ToString());
	//	}
	//}

	//FPlatformApplicationMisc::ClipboardCopy(*IssueText);

	const FString Report = FOutlinerAuditReportFormatter::BuildActorIssuesReport(*SelectedActorResult);
	FOutlinerAuditEditorService::CopyToClipboard(Report);
}

void SOutlinerToolkitAuditPanel::IgnoreSelectedActorIssues()
{
	if (!SelectedActorResult.IsValid())
	{
		return;
	}

	for (const FOutlinerAuditIssuePtr& Issue : SelectedActorResult->Issues)
	{
		if (Issue.IsValid())
		{
			IgnoredIssueKeys.Add(Issue->Key);
			Issue->bIgnored = true;
		}
	}

	SavePersistentIgnoredIssueKeys();
	SelectedActorResult.Reset();
	RefreshFilterOptions();
	ApplyResultFilters();
}

void SOutlinerToolkitAuditPanel::RestoreSelectedActorIssues()
{
	if (!SelectedActorResult.IsValid())
	{
		return;
	}

	bool bChanged = false;
	for (const FOutlinerAuditIssuePtr& Issue : SelectedActorResult->Issues)
	{
		if (Issue.IsValid() && Issue->bIgnored)
		{
			IgnoredIssueKeys.Remove(Issue->Key);
			Issue->bIgnored = false;
			bChanged = true;
		}
	}

	if (!bChanged)
	{
		return;
	}

	SavePersistentIgnoredIssueKeys();
	SelectedActorResult.Reset();
	RefreshFilterOptions();
	ApplyResultFilters();
}

void SOutlinerToolkitAuditPanel::FixSelectedActorIssues()
{
	if (!SelectedActorResult.IsValid() || SelectedActorResult->FixableIssueCount == 0)
	{
		return;
	}

	bool bChanged = false;
	const FScopedTransaction Transaction(LOCTEXT("FixActorAuditIssuesTransaction", "Fix Outliner Audit Actor Issues"));
	for (const FOutlinerAuditIssuePtr& Issue : SelectedActorResult->Issues)
	{
		if (Issue.IsValid() && Issue->FixAction != EOutlinerAuditFixAction::None)
		{
			bChanged |= FOutlinerAuditRunnerService::ApplyFix(*Issue);
		}
	}

	if (bChanged)
	{
		FOutlinerAuditEditorService::RefreshOutliners(true);
		RefreshAudit();
	}
}

UObject* SOutlinerToolkitAuditPanel::GetSelectedActorBlueprintAsset() const
{
	if (!SelectedActorResult.IsValid() || !SelectedActorResult->Actor.IsValid())
	{
		return nullptr;
	}

	const AActor* Actor = SelectedActorResult->Actor.Get();
	return FOutlinerAuditEditorService::GetBlueprintAsset(Actor);
}

void SOutlinerToolkitAuditPanel::GatherSelectedActorStaticMeshes(TArray<UStaticMesh*>& OutStaticMeshes) const
{
	if (!SelectedActorResult.IsValid() || !SelectedActorResult->Actor.IsValid())
	{
		return;
	}

	AActor* Actor = SelectedActorResult->Actor.Get();
	FOutlinerAuditEditorService::GatherActorStaticMeshes(Actor, OutStaticMeshes);
}

#undef LOCTEXT_NAMESPACE
