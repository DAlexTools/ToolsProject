// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "ActorTreeItem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Widgets/Input/SCheckBox.h"
#include "ISceneOutliner.h"
#include "LevelEditor.h"
#include "Editor/GroupActor.h"
#include "ActorGroupingUtils.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Engine/EngineTypes.h"
#include "Selection.h"
#include "EditorActorFolders.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "OutlinerToolkitTypes.h"
#include "Settings/OutlinerToolkitSettings.h"
#include "Audit/Core/OutlinerAuditTypes.h"
#include "Editor.h"
#include "LevelEditorViewport.h"

#define LOCTEXT_NAMESPACE "FOutlinerToolkitUtils"

namespace
{
	/**
	 * Global clipboard storage used by Outliner Toolkit operations.
	 *
	 * Stores actor settings copied from the Scene Outliner and allows them
	 * to be pasted to other actors.
	 */
	FOutlinerActorSettingsClipboard GOutlinerActorSettingsClipboard;

	/**
	 * Displays a modal text input dialog and returns the entered value.
	 *
	 * The dialog blocks execution until the user confirms or cancels the operation.
	 * If the user presses OK, the entered text is returned. If the dialog is canceled,
	 * an empty optional is returned.
	 *
	 * @param Title Title displayed in the dialog window.
	 * @param Label Description text displayed above the input field.
	 * @param DefaultValue Initial value displayed in the text box.
	 *
	 * @return Entered text if confirmed, otherwise an empty optional.
	 */
	TOptional<FString> ShowTextInputDialog(const FText& Title, const FText& Label, const FString& DefaultValue = FString())
	{
		TOptional<FString> Result;
		TSharedPtr<SEditableTextBox> TextBox;
		TSharedPtr<SWindow> Window;

		/* clang-format off */
		Window = SNew(SWindow)
			.Title(Title)
			.ClientSize(FVector2D(360.0f, 120.0f))
			.SupportsMinimize(false)
			.SupportsMaximize(false);

		Window->SetContent(
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(12.0f, 12.0f, 12.0f, 6.0f)
			[
				SNew(STextBlock)
					.Text(Label)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(12.0f, 0.0f, 12.0f, 12.0f)
			[
				SAssignNew(TextBox, SEditableTextBox)
					.Text(FText::FromString(DefaultValue))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(12.0f, 0.0f, 12.0f, 12.0f)
			[
				SNew(SUniformGridPanel)
					.SlotPadding(FMargin(6.0f, 0.0f))

					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
							.Text(FText::FromString("OK"))
							.OnClicked_Lambda([&Result, &Window, &TextBox]()
								{
									Result = TextBox.IsValid() ? TextBox->GetText().ToString() : FString();
									if (Window.IsValid())
									{
										Window->RequestDestroyWindow();
									}
									return FReply::Handled();
								})
					]

				+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
							.Text(FText::FromString("Cancel"))
							.OnClicked_Lambda([&Window]()
								{
									if (Window.IsValid())
									{
										Window->RequestDestroyWindow();
									}
									return FReply::Handled();
								})
					]
			]);
		/* clang-format on */
		FSlateApplication::Get().AddModalWindow(Window.ToSharedRef(), FSlateApplication::Get().FindBestParentWindowForDialogs(nullptr));
		return Result;
	}

	/**
	 * Displays a temporary editor notification.
	 *
	 * Creates a standard Slate notification message that automatically expires
	 * after the specified duration.
	 *
	 * @param Message Notification text to display.
	 * @param Duration Time in seconds before the notification disappears.
	 */
	void ShowOutlinerNotification(const FText& Message, float Duration = 3.0f)
	{
		FNotificationInfo Info(Message);
		Info.ExpireDuration = Duration;
		Info.bUseThrobber = false;
		Info.bUseSuccessFailIcons = true;
		Info.Image = FAppStyle::Get().GetBrush(TEXT("NotificationList.DefaultMessage"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}

} // namespace

/**
 * Helper utilities and shared state used by the Outliner Toolkit module.
 */
namespace OutlinerColumnUtils
{
	/**
	 * @brief Retrieves all components of the specified type from an actor.
	 *
	 * @tparam ComponentType Component class to search for.
	 * @param Actor Actor whose components will be collected.
	 * @return Array containing all components of the requested type.
	 *         Returns an empty array if the actor is null or no matching components exist.
	 */
	template<typename ComponentType>
		requires TIsDerivedFrom<ComponentType, UActorComponent>::Value
	[[nodiscard]] TArray<ComponentType*> GetActorComponents(AActor* Actor)
	{
		TArray<ComponentType*> Components;
		if (Actor)
		{
			Actor->GetComponents<ComponentType>(Components);
		}

		return Components;
	}

	/**
	 * @brief Determines the checkbox state based on a predicate evaluated across multiple components.
	 *
	 * Returns:
	 * - Checked if the predicate evaluates to true for all components.
	 * - Unchecked if the predicate evaluates to false for all components.
	 * - Undetermined if component values differ or the array is empty.
	 *
	 * @tparam ComponentType Component type.
	 * @tparam PredicateType Callable object that returns a boolean-like value for a component.
	 * @param Components Components to evaluate.
	 * @param Predicate Predicate used to extract the state from each component.
	 * @return Corresponding checkbox state.
	 */
	template<typename ComponentType, typename PredicateType>
	[[nodiscard]] ECheckBoxState GetComponentCheckState(const TArray<ComponentType*>& Components, PredicateType&& Predicate)
	{
		if (Components.IsEmpty())
		{
			return ECheckBoxState::Undetermined;
		}

		const bool bFirstValue = !!Predicate(Components[0]);
		for (int32 Index = 1; Index < Components.Num(); ++Index)
		{
			if ((!!Predicate(Components[Index])) != bFirstValue)
			{
				return ECheckBoxState::Undetermined;
			}
		}

		return bFirstValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	/**
	 * @brief Applies a modification function to each valid component in the array.
	 *
	 * Calls Modify() on every valid component before invoking the supplied function.
	 *
	 * @tparam ComponentType Component type.
	 * @tparam FuncType Callable object executed for each component.
	 * @param Components Components to process.
	 * @param Func Function to apply to each valid component.
	 */
	template<typename ComponentType, typename FuncType>
	void ApplyToComponents(const TArray<ComponentType*>& Components, FuncType&& Func)
	{
		for (ComponentType* Component : Components)
		{
			if (!Component)
			{
				continue;
			}

			Component->Modify();
			Func(Component);
		}
	}

	/**
	 * @brief Retrieves a value from multiple primitive components if all values are identical.
	 *
	 * The predicate is evaluated for every component. If all returned values are equal,
	 * the common value is returned. Otherwise, an unset optional is returned.
	 *
	 * @tparam ValueType Type of the value being compared.
	 * @tparam PredicateType Callable object used to extract the value from a component.
	 * @param Predicate Value extraction predicate.
	 * @param Components Primitive components to inspect.
	 * @return Optional containing the shared value, or an unset optional if values differ
	 *         or the component array is empty.
	 */
	template<typename ValueType, typename PredicateType>
	TOptional<ValueType> GetUniformComponentValue(PredicateType&& Predicate, const TArray<UPrimitiveComponent*>& Components)
	{
		if (Components.IsEmpty())
		{
			return TOptional<ValueType>();
		}

		const ValueType FirstValue = Predicate(Components[0]);
		for (int32 Index = 1; Index < Components.Num(); ++Index)
		{
			if (Predicate(Components[Index]) != FirstValue)
			{
				return TOptional<ValueType>();
			}
		}

		return FirstValue;
	}

	/**
	 * @brief Retrieves a value from multiple scene components if all values are identical.
	 *
	 * The predicate is evaluated for every component. If all returned values are equal,
	 * the common value is returned. Otherwise, an unset optional is returned.
	 *
	 * @tparam ValueType Type of the value being compared.
	 * @tparam PredicateType Callable object used to extract the value from a component.
	 * @param Predicate Value extraction predicate.
	 * @param Components Scene components to inspect.
	 * @return Optional containing the shared value, or an unset optional if values differ
	 *         or the component array is empty.
	 */
	template<typename ValueType, typename PredicateType>
	TOptional<ValueType> GetUniformSceneComponentValue(PredicateType&& Predicate, const TArray<USceneComponent*>& Components)
	{
		if (Components.IsEmpty())
		{
			return TOptional<ValueType>();
		}

		const ValueType FirstValue = Predicate(Components[0]);
		for (int32 Index = 1; Index < Components.Num(); ++Index)
		{
			if (Predicate(Components[Index]) != FirstValue)
			{
				return TOptional<ValueType>();
			}
		}

		return FirstValue;
	}

	/**
	 * @brief Resolves an actor from a Scene Outliner tree item reference.
	 *
	 * Attempts to cast the tree item to an actor tree item and returns the
	 * associated actor if the item is valid.
	 *
	 * @param TreeItem Scene Outliner tree item reference.
	 * @return Pointer to the resolved actor, or nullptr if the item is not an actor
	 *         item or is no longer valid.
	 */
	[[nodiscard]] inline AActor* ResolveActor(FSceneOutlinerTreeItemRef TreeItem)
	{
		const FActorTreeItem* ActorItem = TreeItem->CastTo<FActorTreeItem>();
		if (!ActorItem || !ActorItem->IsValid())
		{
			return nullptr;
		}

		return ActorItem->Actor.Get();
	}

	/**
	 * @brief Resolves an actor from a Scene Outliner tree item.
	 *
	 * Attempts to cast the tree item to an actor tree item and returns the
	 * associated actor if the item is valid.
	 *
	 * @param Item Scene Outliner tree item.
	 * @return Pointer to the resolved actor, or nullptr if the item is not an actor
	 *         item or is no longer valid.
	 */
	[[nodiscard]] inline AActor* ResolveActor(const ISceneOutlinerTreeItem& Item)
	{
		const FActorTreeItem* ActorItem = Item.CastTo<FActorTreeItem>();
		if (!ActorItem || !ActorItem->IsValid())
		{
			return nullptr;
		}

		return ActorItem->Actor.Get();
	}

	/**
	 * @brief Converts an actor's tags into a comma-separated string.
	 *
	 * @param Actor Actor whose tags should be serialized.
	 * @return Comma-separated list of tag names. Returns an empty string if the
	 *         actor is null or has no tags.
	 */
	[[nodiscard]] inline FString JoinActorTags(const AActor* Actor)
	{
		if (!Actor || Actor->Tags.IsEmpty())
		{
			return TEXT("");
		}

		TArray<FString> TagStrings;
		TagStrings.Reserve(Actor->Tags.Num());

		for (const FName& Tag : Actor->Tags)
		{
			TagStrings.Add(Tag.ToString());
		}

		return FString::Join(TagStrings, TEXT(", "));
	}

	/**
	 * @brief Parses a comma-separated tag string into an array of unique tags.
	 *
	 * Leading and trailing whitespace is removed from each tag entry.
	 * Empty entries are ignored.
	 *
	 * @param TagsString Comma-separated list of tag names.
	 * @return Array of unique parsed tags.
	 */
	[[nodiscard]] inline TArray<FName> ParseTagsString(const FString& TagsString)
	{
		TArray<FString> RawTags;
		TagsString.ParseIntoArray(RawTags, TEXT(","), true);

		TArray<FName> ParsedTags;
		for (FString& RawTag : RawTags)
		{
			RawTag.TrimStartAndEndInline();
			if (!RawTag.IsEmpty())
			{
				ParsedTags.AddUnique(FName(*RawTag));
			}
		}

		return ParsedTags;
	}

	/**
	 * @brief Refreshes all Scene Outliner instances in the Level Editor.
	 *
	 * @param bFullRefresh If true, performs a full refresh of each outliner.
	 *                     Otherwise, performs a lightweight refresh.
	 */
	inline void RefreshLevelEditorOutliners(bool bFullRefresh)
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
		{
			return;
		}

		TWeakPtr<ILevelEditor> LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor")).GetLevelEditorInstance();
		if (!LevelEditor.IsValid())
		{
			return;
		}

		for (TWeakPtr<ISceneOutliner> SceneOutlinerPtr : LevelEditor.Pin()->GetAllSceneOutliners())
		{
			if (TSharedPtr<ISceneOutliner> SceneOutliner = SceneOutlinerPtr.Pin())
			{
				if (bFullRefresh)
				{
					SceneOutliner->FullRefresh();
				}
				else
				{
					SceneOutliner->Refresh();
				}
			}
		}
	}

	/**
	 * @brief Retrieves all currently selected actors in the editor.
	 *
	 * @return Array containing all selected actors.
	 */
	[[nodiscard]] inline TArray<AActor*> GetSelectedActors()
	{
		TArray<AActor*> SelectedActors;
		check(GEditor);
		for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
		{
			if (AActor* Actor = Cast<AActor>(*It))
			{
				SelectedActors.Add(Actor);
			}
		}

		return SelectedActors;
	}

	/**
	 * @brief Retrieves all selected group actors in the editor.
	 *
	 * @return Array containing selected group actors.
	 */
	inline TArray<AGroupActor*> GetSelectedGroupActors()
	{
		TArray<AGroupActor*> SelectedGroups;
		for (AActor* Actor : GetSelectedActors())
		{
			if (AGroupActor* GroupActor = Cast<AGroupActor>(Actor))
			{
				check(GroupActor);
				SelectedGroups.Add(GroupActor);
			}
		}

		return SelectedGroups;
	}

	/**
	 * @brief Replaces the current editor selection with the specified actors.
	 *
	 * Existing selection is cleared before selecting the provided actors.
	 *
	 * @param ActorsToSelect Actors to select.
	 */
	[[nodiscard]] inline void SelectActors(const TArray<AActor*>& ActorsToSelect)
	{
		check(GEditor);
		GEditor->SelectNone(false, true, false);

		for (AActor* Actor : ActorsToSelect)
		{
			if (IsValid(Actor))
			{
				GEditor->SelectActor(Actor, true, false, true);
			}
		}

		GEditor->NoteSelectionChange();
	}

	/**
	 * @brief Selects a single actor in the editor.
	 *
	 * Existing selection is cleared before the actor is selected.
	 *
	 * @param ActorToSelect Actor to select.
	 */
	[[nodiscard]] inline void SelectActor(AActor* ActorToSelect)
	{
		TArray<AActor*> ActorsToSelect;
		if (IsValid(ActorToSelect))
		{
			ActorsToSelect.Add(ActorToSelect);
		}

		SelectActors(ActorsToSelect);
	}

	/**
	 * @brief Generates a unique folder name for a new actor group.
	 *
	 * If the specified folder name already exists, a numeric suffix is appended
	 * until a unique folder name is found.
	 *
	 * Examples:
	 * - Group
	 * - Group_1
	 * - Group_2
	 *
	 * @param World World in which the folder will be created.
	 * @param BaseFolderName Desired base folder name.
	 * @return Unique folder name that does not currently exist in the world.
	 */
	[[nodiscard]] inline FName CreateUniqueGroupFolderName(UWorld& World, const FName& BaseFolderName)
	{
		FName FinalFolderName = BaseFolderName;
		int32 Index = 1;

		while (FActorFolders::Get().ContainsFolder(World, FFolder(FFolder::GetInvalidRootObject(), FinalFolderName)))
		{
			FinalFolderName = FName(*FString::Printf(TEXT("%s_%d"), *BaseFolderName.ToString(), Index++));
		}

		return FinalFolderName;
	}

	/**
	 * @brief Moves the specified actors to a folder.
	 *
	 * Calls Modify() on each valid actor before updating its folder path
	 * to support the editor undo system.
	 *
	 * @param ActorsToMove Actors to move.
	 * @param FolderName Destination folder path.
	 */
	inline void MoveActorsToFolder(const TArray<AActor*>& ActorsToMove, const FName& FolderName)
	{
		for (AActor* Actor : ActorsToMove)
		{
			if (IsValid(Actor))
			{
				Actor->Modify();
				Actor->SetFolderPath(FolderName);
			}
		}
	}

	/**
	 * @brief Deletes a folder if it no longer contains any actors.
	 *
	 * The folder is only removed if:
	 * - The folder name is valid.
	 * - No actors remain assigned to the folder.
	 * - The folder currently exists.
	 *
	 * @param World World containing the folder.
	 * @param FolderName Folder path to check and potentially delete.
	 */
	inline void DeleteFolderIfEmpty(UWorld& World, const FName& FolderName)
	{
		if (FolderName.IsNone())
		{
			return;
		}

		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			if (It->GetFolderPath() == FolderName)
			{
				return;
			}
		}

		const FFolder Folder(FFolder::GetInvalidRootObject(), FolderName);
		if (FActorFolders::Get().ContainsFolder(World, Folder))
		{
			FActorFolders::Get().DeleteFolder(World, Folder);
		}
	}

	/**
	 * @brief Groups the currently selected actors into a new actor group.
	 *
	 * Creates a new group actor, generates a dedicated folder for the group,
	 * moves all grouped actors into that folder, selects the created group actor,
	 * and refreshes all Scene Outliner instances.
	 *
	 * Requirements:
	 * - At least two actors must be selected.
	 * - The selected actors must be groupable.
	 *
	 * A transaction is created to support undo/redo operations.
	 */
	inline void GroupSelectedActors()
	{
		TArray<AActor*> SelectedActors = GetSelectedActors();

		if (SelectedActors.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("Need at least 2 actors to group."));
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("GroupActorsTransaction", "Group Actors"));
		UActorGroupingUtils* GroupingUtils = UActorGroupingUtils::Get();
		if (!GroupingUtils || !GroupingUtils->CanGroupActors(SelectedActors))
		{
			UE_LOG(LogTemp, Warning, TEXT("Selected actors cannot be grouped."));
			return;
		}

		AGroupActor* GroupActor = GroupingUtils->GroupActors(SelectedActors);

		if (!GroupActor)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to group actors."));
			return;
		}

		UWorld* World = GroupActor->GetWorld();
		if (!World)
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid world for grouped actor."));
			return;
		}

		const FName FinalFolderName = CreateUniqueGroupFolderName(*World, TEXT("Group"));

		const FFolder NewFolder(FFolder::GetInvalidRootObject(), FinalFolderName);
		FActorFolders::Get().CreateFolder(*World, NewFolder);

		TArray<AActor*> GroupedActors;
		GroupActor->GetGroupActors(GroupedActors);
		GroupedActors.Add(GroupActor);

		MoveActorsToFolder(GroupedActors, FinalFolderName);

		FActorFolders::Get().SetSelectedFolderPath(NewFolder);
		SelectActor(GroupActor);

		FNotificationInfo Info(FText::Format(
			NSLOCTEXT("OutlinerToolkit", "FolderCreated", "Folder '{0}' created. Press F2 to rename."),
			FText::FromName(FinalFolderName)));
		Info.ExpireDuration = 5.0f;
		Info.bUseThrobber = false;
		Info.bUseSuccessFailIcons = true;
		Info.Image = FAppStyle::Get().GetBrush(TEXT("NotificationList.DefaultMessage"));

		FSlateNotificationManager::Get().AddNotification(Info);
		OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
	}

	/**
	 * @brief Ungroups all currently selected group actors.
	 *
	 * Restores grouped actors to the root folder (if they were stored in the
	 * group's folder), removes empty group folders, reselects the ungrouped
	 * actors, and refreshes all Scene Outliner instances.
	 *
	 * A transaction is created to support undo/redo operations.
	 */
	inline void UngroupSelectedActors()
	{
		TArray<AGroupActor*> SelectedGroups = GetSelectedGroupActors();
		if (SelectedGroups.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("No group actors selected."));
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("UngroupActorsTransaction", "Ungroup Actors"));
		UActorGroupingUtils* GroupingUtils = UActorGroupingUtils::Get();
		if (!GroupingUtils)
		{
			UE_LOG(LogTemp, Error, TEXT("ActorGroupingUtils is unavailable."));
			return;
		}

		TArray<AActor*>				  GroupsToUngroup;
		TArray<AActor*>				  ActorsToReselect;
		TArray<TPair<UWorld*, FName>> FoldersToCleanup;

		for (AGroupActor* GroupActor : SelectedGroups)
		{
			if (!IsValid(GroupActor))
			{
				continue;
			}

			TArray<AActor*> GroupMembers;
			GroupActor->GetGroupActors(GroupMembers);
			ActorsToReselect.Append(GroupMembers);

			if (UWorld* World = GroupActor->GetWorld())
			{
				FoldersToCleanup.Emplace(World, GroupActor->GetFolderPath());
			}

			const FName GroupFolderPath = GroupActor->GetFolderPath();
			for (AActor* GroupMember : GroupMembers)
			{
				if (IsValid(GroupMember) && GroupMember->GetFolderPath() == GroupFolderPath)
				{
					GroupMember->Modify();
					GroupMember->SetFolderPath(NAME_None);
				}
			}

			GroupsToUngroup.Add(GroupActor);
		}

		if (GroupsToUngroup.Num() == 0)
		{
			return;
		}

		GroupingUtils->UngroupActors(GroupsToUngroup);
		SelectActors(ActorsToReselect);

		for (const TPair<UWorld*, FName>& FolderInfo : FoldersToCleanup)
		{
			if (FolderInfo.Key)
			{
				DeleteFolderIfEmpty(*FolderInfo.Key, FolderInfo.Value);
			}
		}

		FNotificationInfo Info(LOCTEXT("UngroupActorsNotification", "Selected groups were ungrouped."));
		Info.ExpireDuration = 3.0f;
		Info.bUseThrobber = false;
		Info.bUseSuccessFailIcons = true;
		Info.Image = FAppStyle::Get().GetBrush(TEXT("NotificationList.DefaultMessage"));

		FSlateNotificationManager::Get().AddNotification(Info);
		OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
	}

	/**
	 * @brief Copies common actor and scene component settings from the first selected actor.
	 *
	 * The following settings are stored in the actor settings clipboard:
	 * - Tick enabled state
	 * - Hidden In Game state
	 * - Component mobility
	 *
	 * Only values that are consistent across all scene components of the source
	 * actor are copied as concrete values. Mixed values are stored as unset optionals.
	 *
	 * @note Uses the first selected actor as the source.
	 * @warning Does nothing if no valid actor is selected.
	 */
	inline void CopyCommonActorSettings()
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.Num() == 0 || !IsValid(SelectedActors[0]))
		{
			UE_LOG(LogTemp, Warning, TEXT("No actor selected to copy settings from."));
			return;
		}

		AActor* SourceActor = SelectedActors[0];
		const TArray<USceneComponent*> SceneComponents = OutlinerColumnUtils::GetActorComponents<USceneComponent>(SourceActor);

		GOutlinerActorSettingsClipboard = {};
		GOutlinerActorSettingsClipboard.bHasData = true;
		GOutlinerActorSettingsClipboard.bCopyTick = true;
		GOutlinerActorSettingsClipboard.bCopyHiddenInGame = true;
		GOutlinerActorSettingsClipboard.bCopyMobility = true;

		if (SourceActor->PrimaryActorTick.bCanEverTick)
		{
			GOutlinerActorSettingsClipboard.bTickEnabled = SourceActor->IsActorTickEnabled();
		}

		GOutlinerActorSettingsClipboard.bHiddenInGame = OutlinerColumnUtils::GetUniformSceneComponentValue<bool>([](const USceneComponent* Component) -> bool { return !!Component->bHiddenInGame; }, SceneComponents);
		GOutlinerActorSettingsClipboard.Mobility = OutlinerColumnUtils::GetUniformSceneComponentValue<EComponentMobility::Type>([](const USceneComponent* Component) { return Component->Mobility; }, SceneComponents);

		ShowOutlinerNotification(LOCTEXT("CopyCommonSettingsNotification", "Common actor settings copied."));
	}

	/**
	 * @brief Copies rendering-related settings from the first selected actor.
	 *
	 * The following settings are stored in the actor settings clipboard:
	 * - Cast Shadows
	 * - Render Custom Depth
	 * - Custom Depth Stencil Value
	 *
	 * Only values that are consistent across all primitive components of the source
	 * actor are copied as concrete values. Mixed values are stored as unset optionals.
	 *
	 * @note Uses the first selected actor as the source.
	 * @warning Does nothing if no valid actor is selected.
	 */
	inline void CopyRenderingActorSettings()
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.Num() == 0 || !IsValid(SelectedActors[0]))
		{
			UE_LOG(LogTemp, Warning, TEXT("No actor selected to copy settings from."));
			return;
		}

		AActor* SourceActor = SelectedActors[0];
		const TArray<UPrimitiveComponent*> PrimitiveComponents = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(SourceActor);

		GOutlinerActorSettingsClipboard = {};
		GOutlinerActorSettingsClipboard.bHasData = true;
		GOutlinerActorSettingsClipboard.bCopyCastShadows = true;
		GOutlinerActorSettingsClipboard.bCopyRenderCustomDepth = true;
		GOutlinerActorSettingsClipboard.bCopyCustomDepthStencilValue = true;

		GOutlinerActorSettingsClipboard.bCastShadows = OutlinerColumnUtils::GetUniformComponentValue<bool>([](const UPrimitiveComponent* Component) { return !!Component->CastShadow; }, PrimitiveComponents);
		GOutlinerActorSettingsClipboard.bRenderCustomDepth = OutlinerColumnUtils::GetUniformComponentValue<bool>([](const UPrimitiveComponent* Component) { return !!Component->bRenderCustomDepth; }, PrimitiveComponents);
		GOutlinerActorSettingsClipboard.CustomDepthStencilValue = OutlinerColumnUtils::GetUniformComponentValue<int32>([](const UPrimitiveComponent* Component) { return Component->CustomDepthStencilValue; }, PrimitiveComponents);

		ShowOutlinerNotification(LOCTEXT("CopyRenderingSettingsNotification", "Rendering settings copied."));
	}

	/**
	 * @brief Copies collision-related settings from the first selected actor.
	 *
	 * The following settings are stored in the actor settings clipboard:
	 * - Generate Overlap Events
	 *
	 * Only values that are consistent across all primitive components of the source
	 * actor are copied as concrete values. Mixed values are stored as unset optionals.
	 *
	 * @note Uses the first selected actor as the source.
	 * @warning Does nothing if no valid actor is selected.
	 */
	inline void CopyCollisionActorSettings()
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.Num() == 0 || !IsValid(SelectedActors[0]))
		{
			UE_LOG(LogTemp, Warning, TEXT("No actor selected to copy settings from."));
			return;
		}

		AActor* SourceActor = SelectedActors[0];
		const TArray<UPrimitiveComponent*> PrimitiveComponents = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(SourceActor);

		GOutlinerActorSettingsClipboard = {};
		GOutlinerActorSettingsClipboard.bHasData = true;
		GOutlinerActorSettingsClipboard.bCopyGenerateOverlapEvents = true;

		GOutlinerActorSettingsClipboard.bGenerateOverlapEvents = OutlinerColumnUtils::GetUniformComponentValue<bool>([](const UPrimitiveComponent* Component) { return Component->GetGenerateOverlapEvents(); }, PrimitiveComponents);

		ShowOutlinerNotification(LOCTEXT("CopyCollisionSettingsNotification", "Collision settings copied."));
	}

	/**
	 * @brief Copies physics-related settings from the first selected actor.
	 *
	 * The following settings are stored in the actor settings clipboard:
	 * - Simulate Physics
	 *
	 * Only values that are consistent across all primitive components of the source
	 * actor are copied as concrete values. Mixed values are stored as unset optionals.
	 *
	 * @note Uses the first selected actor as the source.
	 * @warning Does nothing if no valid actor is selected.
	 */
	inline void CopyPhysicsActorSettings()
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.Num() == 0 || !IsValid(SelectedActors[0]))
		{
			UE_LOG(LogTemp, Warning, TEXT("No actor selected to copy settings from."));
			return;
		}

		AActor* SourceActor = SelectedActors[0];
		const TArray<UPrimitiveComponent*> PrimitiveComponents = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(SourceActor);

		GOutlinerActorSettingsClipboard = {};
		GOutlinerActorSettingsClipboard.bHasData = true;
		GOutlinerActorSettingsClipboard.bCopySimulatePhysics = true;

		GOutlinerActorSettingsClipboard.bSimulatePhysics = OutlinerColumnUtils::GetUniformComponentValue<bool>([](const UPrimitiveComponent* Component)
			{
				return Component->IsSimulatingPhysics();
			}, PrimitiveComponents);

		ShowOutlinerNotification(LOCTEXT("CopyPhysicsSettingsNotification", "Physics settings copied."));
	}

	/**
	 * @brief Copies actor tags from the first selected actor.
	 *
	 * All actor tags are stored in the actor settings clipboard and can later be
	 * applied to other actors.
	 *
	 * @note Uses the first selected actor as the source.
	 * @warning Does nothing if no valid actor is selected.
	 */
	inline void CopyTagsActorSettings()
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.Num() == 0 || !IsValid(SelectedActors[0]))
		{
			UE_LOG(LogTemp, Warning, TEXT("No actor selected to copy settings from."));
			return;
		}

		AActor* SourceActor = SelectedActors[0];

		GOutlinerActorSettingsClipboard = {};
		GOutlinerActorSettingsClipboard.bHasData = true;
		GOutlinerActorSettingsClipboard.bCopyTags = true;
		GOutlinerActorSettingsClipboard.Tags = SourceActor->Tags;

		ShowOutlinerNotification(LOCTEXT("CopyTagsSettingsNotification", "Actor tags copied."));
	}

	/**
	 * @brief Copies the folder assignment from the first selected actor.
	 *
	 * The actor's folder path is stored in the actor settings clipboard and can
	 * later be applied to other actors.
	 *
	 * @note Uses the first selected actor as the source.
	 * @warning Does nothing if no valid actor is selected.
	 */
	inline void CopyFolderActorSettings()
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.Num() == 0 || !IsValid(SelectedActors[0]))
		{
			UE_LOG(LogTemp, Warning, TEXT("No actor selected to copy settings from."));
			return;
		}

		AActor* SourceActor = SelectedActors[0];

		GOutlinerActorSettingsClipboard = {};
		GOutlinerActorSettingsClipboard.bHasData = true;
		GOutlinerActorSettingsClipboard.bCopyFolder = true;
		GOutlinerActorSettingsClipboard.FolderPath = SourceActor->GetFolderPath();

		ShowOutlinerNotification(LOCTEXT("CopyFolderSettingsNotification", "Actor folder copied."));
	}

	/**
	 * @brief Applies the copied actor settings to all currently selected actors.
	 *
	 * Settings are taken from the actor settings clipboard and applied only if:
	 * - The corresponding copy flag is enabled.
	 * - A valid value is present in the clipboard.
	 *
	 * Supported settings include:
	 * - Tick enabled state
	 * - Hidden In Game
	 * - Mobility
	 * - Cast Shadows
	 * - Generate Overlap Events
	 * - Render Custom Depth
	 * - Custom Depth Stencil Value
	 * - Simulate Physics
	 * - Actor Tags
	 * - Folder Path
	 *
	 * A transaction is created to support undo/redo operations.
	 *
	 * @note Physics simulation is only applied to actors compatible with the
	 *       current implementation (e.g. static mesh actors).
	 * @note Scene Outliners are automatically refreshed when required by the
	 *       applied settings.
	 * @warning Does nothing if the clipboard is empty or no actors are selected.
	 */
	inline void PasteSettingsToSelectedActors()
	{
		if (!GOutlinerActorSettingsClipboard.bHasData)
		{
			UE_LOG(LogTemp, Warning, TEXT("No copied actor settings available."));
			return;
		}

		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("No actors selected to paste settings to."));
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("PasteActorSettingsTransaction", "Paste Actor Settings"));

		for (AActor* Actor : SelectedActors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}

			const TArray<USceneComponent*> SceneComponents = OutlinerColumnUtils::GetActorComponents<USceneComponent>(Actor);
			const TArray<UPrimitiveComponent*> PrimitiveComponents = OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor);

			if (GOutlinerActorSettingsClipboard.bCopyTick && GOutlinerActorSettingsClipboard.bTickEnabled.IsSet() && Actor->PrimaryActorTick.bCanEverTick)
			{
				Actor->Modify();
				Actor->SetActorTickEnabled(GOutlinerActorSettingsClipboard.bTickEnabled.GetValue());
			}

			if (GOutlinerActorSettingsClipboard.bCopyHiddenInGame && GOutlinerActorSettingsClipboard.bHiddenInGame.IsSet())
			{
				const bool bHiddenInGame = GOutlinerActorSettingsClipboard.bHiddenInGame.GetValue();
				OutlinerColumnUtils::ApplyToComponents(SceneComponents, [bHiddenInGame](USceneComponent* Component)
					{
						Component->SetHiddenInGame(bHiddenInGame);
					});
			}

			if (GOutlinerActorSettingsClipboard.bCopyMobility && GOutlinerActorSettingsClipboard.Mobility.IsSet())
			{
				const EComponentMobility::Type Mobility = GOutlinerActorSettingsClipboard.Mobility.GetValue();
				OutlinerColumnUtils::ApplyToComponents(SceneComponents, [Mobility](USceneComponent* Component)
					{
						Component->SetMobility(Mobility);
					});
			}

			if (GOutlinerActorSettingsClipboard.bCopyCastShadows && GOutlinerActorSettingsClipboard.bCastShadows.IsSet())
			{
				const bool bCastShadows = GOutlinerActorSettingsClipboard.bCastShadows.GetValue();
				OutlinerColumnUtils::ApplyToComponents(PrimitiveComponents, [bCastShadows](UPrimitiveComponent* Component)
					{
						Component->SetCastShadow(bCastShadows);
					});
			}

			if (GOutlinerActorSettingsClipboard.bCopyGenerateOverlapEvents && GOutlinerActorSettingsClipboard.bGenerateOverlapEvents.IsSet())
			{
				const bool bGenerateOverlapEvents = GOutlinerActorSettingsClipboard.bGenerateOverlapEvents.GetValue();
				OutlinerColumnUtils::ApplyToComponents(PrimitiveComponents, [bGenerateOverlapEvents](UPrimitiveComponent* Component)
					{
						Component->SetGenerateOverlapEvents(bGenerateOverlapEvents);
					});
			}

			if (GOutlinerActorSettingsClipboard.bCopyRenderCustomDepth && GOutlinerActorSettingsClipboard.bRenderCustomDepth.IsSet())
			{
				const bool bRenderCustomDepth = GOutlinerActorSettingsClipboard.bRenderCustomDepth.GetValue();
				OutlinerColumnUtils::ApplyToComponents(PrimitiveComponents, [bRenderCustomDepth](UPrimitiveComponent* Component)
					{
						Component->SetRenderCustomDepth(bRenderCustomDepth);
					});
			}

			if (GOutlinerActorSettingsClipboard.bCopyCustomDepthStencilValue && GOutlinerActorSettingsClipboard.CustomDepthStencilValue.IsSet())
			{
				const int32 CustomDepthStencilValue = GOutlinerActorSettingsClipboard.CustomDepthStencilValue.GetValue();
				OutlinerColumnUtils::ApplyToComponents(PrimitiveComponents, [CustomDepthStencilValue](UPrimitiveComponent* Component)
					{
						Component->SetCustomDepthStencilValue(CustomDepthStencilValue);
					});
			}

			if (GOutlinerActorSettingsClipboard.bCopySimulatePhysics && GOutlinerActorSettingsClipboard.bSimulatePhysics.IsSet() && Actor->IsA<AStaticMeshActor>())
			{
				const bool bSimulatePhysics = GOutlinerActorSettingsClipboard.bSimulatePhysics.GetValue();
				if (bSimulatePhysics)
				{
					OutlinerColumnUtils::ApplyToComponents(SceneComponents, [](USceneComponent* Component)
						{
							Component->SetMobility(EComponentMobility::Movable);
						});
				}

				OutlinerColumnUtils::ApplyToComponents(PrimitiveComponents, [bSimulatePhysics](UPrimitiveComponent* Component)
					{
						Component->SetSimulatePhysics(bSimulatePhysics);
					});
			}

			if (GOutlinerActorSettingsClipboard.bCopyTags)
			{
				Actor->Modify();
				Actor->Tags = GOutlinerActorSettingsClipboard.Tags;
			}

			if (GOutlinerActorSettingsClipboard.bCopyFolder)
			{
				Actor->Modify();
				Actor->SetFolderPath(GOutlinerActorSettingsClipboard.FolderPath);
			}
		}

		ShowOutlinerNotification(LOCTEXT("PasteSettingsNotification", "Actor settings pasted."));
		if (GOutlinerActorSettingsClipboard.bCopyFolder || GOutlinerActorSettingsClipboard.bCopyTick || GOutlinerActorSettingsClipboard.bCopySimulatePhysics)
		{
			OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
		}
	}

	/**
	 * @brief Adds a prefix or suffix to the labels of all selected actors.
	 *
	 * The specified string is appended to either the beginning or end of each
	 * selected actor label.
	 *
	 * A transaction is created to support undo/redo operations.
	 *
	 * @param Affix String to add to actor labels.
	 * @param bPrefix If true, the string is added as a prefix.
	 *                If false, the string is added as a suffix.
	 *
	 * @warning Does nothing if the affix is empty or no actors are selected.
	 */
	inline void ApplyAffixToSelectedActors(const FString& Affix, bool bPrefix)
	{
		if (Affix.IsEmpty())
		{
			return;
		}

		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.Num() == 0)
		{
			return;
		}

		const FScopedTransaction Transaction(bPrefix
			? LOCTEXT("AddPrefixTransaction", "Add Prefix To Actors")
			: LOCTEXT("AddSuffixTransaction", "Add Suffix To Actors"));

		for (AActor* Actor : SelectedActors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}

			const FString OldLabel = Actor->GetActorLabel();
			const FString NewLabel = bPrefix ? (Affix + OldLabel) : (OldLabel + Affix);

			Actor->Modify();
			Actor->SetActorLabel(NewLabel);
		}

		ShowOutlinerNotification(bPrefix
			? LOCTEXT("AddPrefixNotification", "Prefix added to selected actor labels.")
			: LOCTEXT("AddSuffixNotification", "Suffix added to selected actor labels."));
	}

	/**
	 * @brief Displays a dialog for entering a label prefix and applies it to
	 * all selected actors.
	 *
	 * If the user confirms the dialog, the entered prefix is prepended to the
	 * label of each selected actor.
	 *
	 * @see ApplyAffixToSelectedActors()
	 */
	inline void AddPrefixToSelectedActors()
	{
		const TOptional<FString> Prefix = ShowTextInputDialog(
			LOCTEXT("AddPrefixDialogTitle", "Add Prefix"),
			LOCTEXT("AddPrefixDialogLabel", "Prefix to add to selected actor labels:"));

		if (Prefix.IsSet())
		{
			ApplyAffixToSelectedActors(Prefix.GetValue(), true);
		}
	}

	/**
	 * @brief Displays a dialog for entering a label suffix and applies it to
	 * all selected actors.
	 *
	 * If the user confirms the dialog, the entered suffix is appended to the
	 * label of each selected actor.
	 *
	 * @see ApplyAffixToSelectedActors()
	 */
	inline void AddSuffixToSelectedActors()
	{
		const TOptional<FString> Suffix = ShowTextInputDialog(
			LOCTEXT("AddSuffixDialogTitle", "Add Suffix"),
			LOCTEXT("AddSuffixDialogLabel", "Suffix to add to selected actor labels:"));

		if (Suffix.IsSet())
		{
			ApplyAffixToSelectedActors(Suffix.GetValue(), false);
		}
	}

	/**
	 * @brief Enables or disables ticking for all selected actors.
	 *
	 * Only actors that support ticking (`bCanEverTick`) are modified.
	 *
	 * A transaction is created to support undo/redo operations.
	 *
	 * @param bEnabled If true, ticking is enabled. If false, ticking is disabled.
	 *
	 * @warning Actors that do not support ticking are ignored.
	 */
	inline void SetSelectedActorsTickEnabled(bool bEnabled)
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.IsEmpty())
		{
			return;
		}

		const FScopedTransaction Transaction(bEnabled
			? LOCTEXT("EnableSelectedActorTickTransaction", "Enable Actor Tick")
			: LOCTEXT("DisableSelectedActorTickTransaction", "Disable Actor Tick"));

		for (AActor* Actor : SelectedActors)
		{
			if (!Actor || !Actor->PrimaryActorTick.bCanEverTick)
			{
				continue;
			}

			Actor->Modify();
			Actor->SetActorTickEnabled(bEnabled);
		}

		OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
	}

	/**
	 * @brief Sets the Hidden In Game state for all scene components of the
	 * selected actors.
	 *
	 * The specified visibility state is applied to every scene component
	 * belonging to each selected actor.
	 *
	 * A transaction is created to support undo/redo operations.
	 *
	 * @param bEnabled If true, actors will be hidden during gameplay.
	 *                 If false, actors will be visible during gameplay.
	 */
	inline void SetSelectedActorsHiddenInGame(bool bEnabled)
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.IsEmpty())
		{
			return;
		}

		const FScopedTransaction Transaction(bEnabled
			? LOCTEXT("HideSelectedActorsInGameTransaction", "Hide Actors In Game")
			: LOCTEXT("ShowSelectedActorsInGameTransaction", "Show Actors In Game"));

		for (AActor* Actor : SelectedActors)
		{
			OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<USceneComponent>(Actor),
				[bEnabled](USceneComponent* Component) {
					Component->SetHiddenInGame(bEnabled);
				});
		}

		OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
	}

	/**
	 * @brief Enables or disables shadow casting for all selected actors.
	 *
	 * The specified state is applied to every primitive component belonging
	 * to each selected actor.
	 *
	 * A transaction is created to support undo/redo operations.
	 *
	 * @param bEnabled If true, shadow casting is enabled.
	 *                 If false, shadow casting is disabled.
	 */
	inline void SetSelectedActorsCastShadows(bool bEnabled)
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.IsEmpty())
		{
			return;
		}

		const FScopedTransaction Transaction(bEnabled
			? LOCTEXT("EnableSelectedActorsCastShadowsTransaction", "Enable Cast Shadows")
			: LOCTEXT("DisableSelectedActorsCastShadowsTransaction", "Disable Cast Shadows"));

		for (AActor* Actor : SelectedActors)
		{
			OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor),
				[bEnabled](UPrimitiveComponent* Component)
				{
					Component->SetCastShadow(bEnabled);
				});
		}

		OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
	}

	/**
	 * @brief Enables or disables overlap event generation for all selected actors.
	 *
	 * The specified state is applied to every primitive component belonging
	 * to each selected actor.
	 *
	 * A transaction is created to support undo/redo operations.
	 *
	 * @param bEnabled If true, overlap events are generated.
	 *                 If false, overlap event generation is disabled.
	 */
	inline void SetSelectedActorsGenerateOverlapEvents(bool bEnabled)
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.IsEmpty())
		{
			return;
		}

		const FScopedTransaction Transaction(bEnabled
			? LOCTEXT("EnableSelectedActorsOverlapEventsTransaction", "Enable Overlap Events")
			: LOCTEXT("DisableSelectedActorsOverlapEventsTransaction", "Disable Overlap Events"));

		for (AActor* Actor : SelectedActors)
		{
			OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor),
				[bEnabled](UPrimitiveComponent* Component)
				{
					Component->SetGenerateOverlapEvents(bEnabled);
				});
		}

		OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
	}

	/**
	 * @brief Enables or disables Custom Depth rendering for all selected actors.
	 *
	 * The specified state is applied to every primitive component belonging
	 * to each selected actor.
	 *
	 * A transaction is created to support undo/redo operations.
	 *
	 * @param bEnabled If true, Custom Depth rendering is enabled.
	 *                 If false, Custom Depth rendering is disabled.
	 */
	inline void SetSelectedActorsCustomDepth(bool bEnabled)
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.IsEmpty())
		{
			return;
		}

		const FScopedTransaction Transaction(bEnabled
			? LOCTEXT("EnableSelectedActorsCustomDepthTransaction", "Enable CustomDepth")
			: LOCTEXT("DisableSelectedActorsCustomDepthTransaction", "Disable CustomDepth"));

		for (AActor* Actor : SelectedActors)
		{
			OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor),
				[bEnabled](UPrimitiveComponent* Component)
				{
					Component->SetRenderCustomDepth(bEnabled);
				});
		}

		OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
	}

	/**
	 * @brief Enables or disables physics simulation for all supported selected actors.
	 *
	 * The specified state is applied to every primitive component belonging
	 * to each supported actor.
	 *
	 * When enabling physics simulation, all scene components of the actor are
	 * first set to movable mobility to satisfy physics requirements.
	 *
	 * A transaction is created to support undo/redo operations.
	 *
	 * @param bEnabled If true, physics simulation is enabled.
	 *                 If false, physics simulation is disabled.
	 *
	 * @note The current implementation only processes supported actor types
	 *       (e.g. static mesh actors).
	 * @warning Unsupported actors are ignored.
	 */
	inline void SetSelectedActorsSimulatePhysics(bool bEnabled)
	{
		const TArray<AActor*> SelectedActors = GetSelectedActors();
		if (SelectedActors.IsEmpty())
		{
			return;
		}

		const FScopedTransaction Transaction(bEnabled
			? LOCTEXT("EnableSelectedActorsSimulatePhysicsTransaction", "Enable Simulate Physics")
			: LOCTEXT("DisableSelectedActorsSimulatePhysicsTransaction", "Disable Simulate Physics"));

		for (AActor* Actor : SelectedActors)
		{
			if (!Actor || !Actor->IsA<AStaticMeshActor>())
			{
				continue;
			}

			if (bEnabled)
			{
				OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<USceneComponent>(Actor),
					[](USceneComponent* Component)
					{
						Component->SetMobility(EComponentMobility::Movable);
					});
			}

			OutlinerColumnUtils::ApplyToComponents(OutlinerColumnUtils::GetActorComponents<UPrimitiveComponent>(Actor),
				[bEnabled](UPrimitiveComponent* Component)
				{
					Component->SetSimulatePhysics(bEnabled);
				});
		}

		OutlinerColumnUtils::RefreshLevelEditorOutliners(true);
	}

	inline void RefreshEditorMovementState()
	{
		if (GCurrentLevelEditingViewportClient)
		{
			constexpr bool bForceCachedElementRefresh = true;
			GCurrentLevelEditingViewportClient->GetElementsToManipulate(bForceCachedElementRefresh);
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports(false);
		}
	}

} // namespace OutlinerColumnUtils


#undef LOCTEXT_NAMESPACE 