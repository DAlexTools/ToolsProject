// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "OutlinerToolkitTypes.h"
#include "Audit/Core/OutlinerAuditTypes.h"

class AActor;
class FMenuBuilder;
class ITableRow;
class SHeaderRow;
class SSearchBox;
class STableViewBase;
class UObject;
class UStaticMesh;
class UWorld;


class OUTLINERTOOLKIT_API SOutlinerToolkitAuditPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOutlinerToolkitAuditPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void RefreshAudit();
	void LoadPersistentIgnoredIssueKeys();
	void SavePersistentIgnoredIssueKeys() const;
	void RefreshFilterOptions();
	void ApplyResultFilters();
	bool DoesIssuePassResultFilters(const FOutlinerAuditIssue& Issue) const;
	void RebuildFilteredActorResults();
	void RecalculateCounts();

	void FillPluginMenu(FMenuBuilder& MenuBuilder);
	TSharedRef<SWidget> BuildMenuBar();
	TSharedRef<SWidget> BuildCriteriaWidget();
	TSharedRef<SWidget> BuildFilterWidget();
	TSharedRef<SWidget> BuildCriteriaMenuContent();
	TSharedRef<SWidget> BuildCriterionCheckBox(EOutlinerAuditCriterion Criterion, const FText& Label, const FText& Tooltip);
	TSharedRef<SWidget> BuildSeverityFilterMenuContent();
	TSharedRef<SWidget> BuildCategoryFilterMenuContent();
	TSharedRef<SWidget> BuildIssueTypeFilterMenuContent();
	TSharedRef<SWidget> BuildAuditScopeSelector();
	TSharedRef<SWidget> MakeAuditScopeOptionWidget(TSharedPtr<EOutlinerAuditScope> ScopeOption) const;
	TSharedRef<SHeaderRow> BuildHeaderRow() const;
	TSharedRef<SWidget> BuildDetailsPanel();
	TSharedRef<ITableRow> OnGenerateActorRow(FOutlinerAuditActorResultPtr InItem, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> OnGenerateActorIssueDetailRow(FOutlinerAuditIssuePtr InItem, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedPtr<SWidget> BuildActorContextMenu();
	void OnActorSelectionChanged(FOutlinerAuditActorResultPtr InItem, ESelectInfo::Type SelectInfo);
	void OnActorDoubleClicked(FOutlinerAuditActorResultPtr InItem);
	void ShowSelectedActorDetails();
	FReply OnCloseDetailsPanelClicked();
	void RefreshDetailsPanelItems();

	FReply OnRunAuditClicked();
	FReply OnEnableAllCriteriaClicked();
	FReply OnDisableAllCriteriaClicked();
	FReply OnSelectActorClicked() const;
	FReply OnSelectAllActorsClicked() const;
	FReply OnFixSelectedActorClicked();
	FReply OnIgnoreSelectedActorClicked();
	FReply OnRestoreSelectedActorClicked();
	FReply OnEnableAllSeverityFiltersClicked();
	FReply OnDisableAllSeverityFiltersClicked();
	FReply OnEnableAllCategoryFiltersClicked();
	FReply OnDisableAllCategoryFiltersClicked();
	FReply OnEnableAllIssueTypeFiltersClicked();
	FReply OnDisableAllIssueTypeFiltersClicked();
	FReply OnClearFiltersClicked();

	ECheckBoxState GetCriterionCheckState(EOutlinerAuditCriterion Criterion) const;
	void OnCriterionCheckStateChanged(ECheckBoxState NewState, EOutlinerAuditCriterion Criterion);
	bool IsCriterionEnabled(EOutlinerAuditCriterion Criterion) const;
	void SetCriterionEnabled(EOutlinerAuditCriterion Criterion, bool bEnabled);
	bool HasEnabledCriteria() const;
	int32 CountEnabledCriteria() const;
	FText GetCriteriaButtonText() const;
	ECheckBoxState GetSeverityFilterCheckState(EOutlinerAuditSeverity Severity) const;
	void OnSeverityFilterCheckStateChanged(ECheckBoxState NewState, EOutlinerAuditSeverity Severity);
	ECheckBoxState GetCategoryFilterCheckState(TSharedPtr<FString> Category) const;
	void OnCategoryFilterCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FString> Category);
	ECheckBoxState GetIssueTypeFilterCheckState(TSharedPtr<FString> IssueType) const;
	void OnIssueTypeFilterCheckStateChanged(ECheckBoxState NewState, TSharedPtr<FString> IssueType);
	void OnActorNameFilterTextChanged(const FText& NewText);
	ECheckBoxState GetShowIgnoredCheckState() const;
	void OnShowIgnoredCheckStateChanged(ECheckBoxState NewState);
	bool IsSeverityFilterEnabled(EOutlinerAuditSeverity Severity) const;
	void SetSeverityFilterEnabled(EOutlinerAuditSeverity Severity, bool bEnabled);
	int32 CountEnabledSeverityFilters() const;
	bool HasActiveResultFilters() const;
	FText GetSeverityFilterButtonText() const;
	FText GetCategoryFilterButtonText() const;
	FText GetIssueTypeFilterButtonText() const;

	
	bool HasSelectedActorBlueprint() const;
	bool HasSelectedActorStaticMeshes() const;
	bool HasFixableSelectedActorIssues() const;
	bool HasRestorableSelectedActorIssues() const;
	EVisibility GetDetailsPanelVisibility() const;
	EVisibility GetNoDetailsRowsVisibility() const;
	FText GetDetailsPanelActorText() const;
	FText GetDetailsPanelIssueText() const;
	FText GetDetailsPanelSummaryText() const;
	FText GetSummaryText() const;
	FText GetAuditScopeText() const;
	void OnAuditScopeChanged(TSharedPtr<EOutlinerAuditScope> NewScope, ESelectInfo::Type SelectInfo);


	bool HasSelectedActor() const;
	bool HasSelectedActorData() const;
	void SelectSelectedActor() const;
	void FocusSelectedActor() const;
	void OpenSelectedActor() const;
	void OpenSelectedActorStaticMeshes() const;
	void CopySelectedActorPath() const;
	void CopySelectedActorIssuesText() const;
	void IgnoreSelectedActorIssues();
	void RestoreSelectedActorIssues();
	void FixSelectedActorIssues();
	UObject* GetSelectedActorBlueprintAsset() const;
	void GatherSelectedActorStaticMeshes(TArray<UStaticMesh*>& OutStaticMeshes) const;

private:
	TArray<FOutlinerAuditIssuePtr> AllAuditIssues;
	TArray<FOutlinerAuditIssuePtr> FilteredAuditIssues;
	TArray<FOutlinerAuditActorResultPtr> FilteredActorResults;
	TSet<FString> IgnoredIssueKeys;
	TSharedPtr<SListView<FOutlinerAuditActorResultPtr>> ActorListView;
	TSharedPtr<SListView<FOutlinerAuditIssuePtr>> ActorIssueDetailListView;
	TSharedPtr<SSearchBox> ActorNameSearchBox;
	FOutlinerAuditActorResultPtr SelectedActorResult;
	TArray<FOutlinerAuditIssuePtr> SelectedActorIssues;
	TArray<TSharedPtr<EOutlinerAuditScope>> AuditScopeOptions;
	TSharedPtr<EOutlinerAuditScope> SelectedAuditScopeOption;
	TArray<TSharedPtr<FString>> CategoryFilterOptions;
	TArray<TSharedPtr<FString>> IssueTypeFilterOptions;
	TSet<FString> EnabledCategoryFilters;
	TSet<FString> EnabledIssueTypeFilters;
	FText ActorNameFilterText;
	EOutlinerAuditScope AuditScope = EOutlinerAuditScope::WholeWorld;
	uint32 EnabledCriteriaMask = OutlinerAuditAllCriteriaMask;
	uint8 EnabledSeverityFilterMask = OutlinerAuditAllSeverityFilterMask;
	bool bCategoryFilterCustomized = false;
	bool bIssueTypeFilterCustomized = false;
	bool bShowIgnoredIssues = false;
	bool bDetailsPanelVisible = false;
	int32 AuditedActorCount = 0;

	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 InfoCount = 0;
};
