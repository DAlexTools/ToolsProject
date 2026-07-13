// Copyright (c) 2026 DimAlek. All Rights Reserved.


#include "Audit/Widgets/OutlinerAuditContextMenuBuilder.h"
#include "Audit/Widgets/SOutlinerToolkitAuditPanel.h"

#define LOCTEXT_NAMESPACE "FOutlinerAuditContextMenuBuilder"

//TSharedPtr<SWidget> FOutlinerAuditContextMenuBuilder::Build(SOutlinerToolkitAuditPanel& Panel)
//{
//	//FMenuBuilder MenuBuilder(true, nullptr);
//
//	//MenuBuilder.BeginSection("Actor", LOCTEXT("ActorContextActorSection", "Actor"));
//
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextSelectActor", "Select Actor"),
//	//	LOCTEXT("ActorContextSelectActorTooltip", "Select this audit actor."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.SelectInViewport"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::SelectSelectedActor),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActor)));
//
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextFocusActor", "Focus Actor"),
//	//	LOCTEXT("ActorContextFocusActorTooltip",
//	//		"Select and frame this audit actor."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::FocusSelectedActor),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActor)));
//
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextOpenActor", "Open Actor Blueprint"),
//	//	LOCTEXT("ActorContextOpenActorTooltip",
//	//		"Open this actor's Blueprint asset."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::OpenSelectedActor),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorBlueprint)));
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextOpenStaticMeshes", "Open Static Meshes"),
//	//	LOCTEXT("ActorContextOpenStaticMeshesTooltip",
//	//		"Open every unique Static Mesh asset used by this actor."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.StaticMesh"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::OpenSelectedActorStaticMeshes),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorStaticMeshes)));
//	//MenuBuilder.EndSection();
//
//	//MenuBuilder.BeginSection("Details", LOCTEXT("ActorContextDetailsSection", "Details"));
//
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextShowDetails", "Show Details"),
//	//	LOCTEXT("ActorContextShowDetailsTooltip", "Open the details panel for every visible issue on this actor."),
//	//	FSlateIcon(),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::ShowSelectedActorDetails),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorData)));
//	//MenuBuilder.EndSection();
//
//	//MenuBuilder.BeginSection("Copy", LOCTEXT("ActorContextCopySection", "Copy"));
//
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextCopyActorPath", "Copy Actor Path"),
//	//	LOCTEXT("ActorContextCopyActorPathTooltip", "Copy the actor object path to the clipboard."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::CopySelectedActorPath),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActor)));
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextCopyIssues", "Copy Actor Issues"),
//	//	LOCTEXT("ActorContextCopyIssuesTooltip", "Copy every visible audit issue for this actor to the clipboard."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::CopySelectedActorIssuesText),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorData)));
//	//
//	//MenuBuilder.EndSection();
//
//	//MenuBuilder.BeginSection("Actions",
//	//	LOCTEXT("ActorContextActionsSection", "Actions"));
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextIgnore", "Ignore Actor Issues"),
//	//	LOCTEXT("ActorContextIgnoreTooltip", "Persistently hide every visible issue for this actor."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Hidden"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::IgnoreSelectedActorIssues),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasSelectedActorData)));
//
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextRestore", "Restore Actor Issues"),
//	//	LOCTEXT("ActorContextRestoreTooltip",
//	//		"Remove persistent ignore for every ignored issue on this actor."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visible"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::RestoreSelectedActorIssues),
//	//		FCanExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::HasRestorableSelectedActorIssues)));
//
//	//MenuBuilder.AddMenuEntry(
//	//	LOCTEXT("ActorContextFix", "Fix Actor Issues"),
//	//	LOCTEXT("ActorContextFixTooltip",
//	//		"Apply every automatic fix available for this actor."),
//	//	FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
//	//	FUIAction(FExecuteAction::CreateSP(this, &SOutlinerToolkitAuditPanel::FixSelectedActorIssues),
//	//		FCanExecuteAction::CreateSP(this,&SOutlinerToolkitAuditPanel::HasFixableSelectedActorIssues)));
//
//	//MenuBuilder.EndSection();
//
//	//return MenuBuilder.MakeWidget();
//
//}
#undef LOCTEXT_NAMESPACE

