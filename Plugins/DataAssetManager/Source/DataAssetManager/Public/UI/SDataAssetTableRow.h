// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RevisionControlStyle/RevisionControlStyle.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"
#include "DataAssetManagerTypes.h"

/**
 * @class SDataAssetTableRow
 * @brief A multi-column table row widget for displaying asset data in the Data Asset Manager.
 * @inherits SMultiColumnTableRow<TSharedPtr<FAssetData>>
 *
 * This class represents a single row in the asset table, handling display, editing,
 * and user interactions for individual asset entries.
 */
class DATAASSETMANAGER_API SDataAssetTableRow : public SMultiColumnTableRow<TSharedPtr<FAssetData>>
{

public:
    /** @brief Delegate for handling asset rename operations */
    DECLARE_DELEGATE_ThreeParams(FOnAssetRenamed, TSharedPtr<FAssetData>, const FText&, ETextCommit::Type);

    /** @brief Delegate for creating context menus */
    DECLARE_DELEGATE_TwoParams(FOnCreateContextMenu, const FGeometry&, const FPointerEvent&);

    /** @brief Delegate for handling double-click events on assets */
    DECLARE_DELEGATE_TwoParams(FOnAssetDoubleClicked, const FGeometry&, const FPointerEvent&);

    /** @brief Delegate for registering editable text widgets */
    DECLARE_DELEGATE_TwoParams(FOnRegisterEditableText, TSharedPtr<FAssetData>, TSharedRef<SEditableText>);

    /** @brief Delegate for handling mouse button down events with return value */
    DECLARE_DELEGATE_RetVal_TwoParams(FReply, FOnAssetMouseButtonDown, const FGeometry&, const FPointerEvent&);

public:
    /**
     * @brief Slate widget argument declarations
     */
    SLATE_BEGIN_ARGS(SDataAssetTableRow) {}
        /** @brief The asset data item this row represents */
        SLATE_ARGUMENT(TSharedPtr<FAssetData>, Item)
        /** @brief The owning Data Asset Manager widget */
        SLATE_ARGUMENT(TSharedPtr<class SDataAssetManagerWidget>, Owner)

        /** @brief Event called when an asset is renamed */
        SLATE_EVENT(FOnAssetRenamed, OnAssetRenamed)
        /** @brief Event called to create a context menu */
        SLATE_EVENT(FOnCreateContextMenu, OnCreateContextMenu)
        /** @brief Event called when an asset is double-clicked */
        SLATE_EVENT(FOnAssetDoubleClicked, OnAssetDoubleClicked)
        /** @brief Event called to register an editable text widget */
        SLATE_EVENT(FOnRegisterEditableText, OnRegisterEditableText)
        /** @brief Event called when mouse button is pressed on asset */
        SLATE_EVENT(FOnAssetMouseButtonDown, OnMouseButtonDown)

    SLATE_END_ARGS()

    /**
     * @brief Constructs the table row widget
     * @param InArgs - Slate construction arguments
     * @param InOwnerTable - The table that owns this row
     */
    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);

    /**
     * @brief Destructor
     */
    virtual ~SDataAssetTableRow();

    /**
     * @brief Generates widget for a specific column
     * @param ColumnId - The ID of the column to generate widget for
     * @return Shared reference to the generated widget
     */
    virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;


    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    /**
     * @brief Adds a dirty state change handler for the specified package
     * @param PackageName - The name of the package to monitor for dirty state changes
     */
    void AddDirtyEventHandler(const FString& PackageName);


    /**
     * @brief Mouse button down handler for this table row widget.
     *
     * This function is bound to the Slate event `OnMouseButtonDown`
     * and is triggered when the user presses any mouse button while the
     * cursor is over the row.
     * Typical use cases: selecting the row, initiating drag behavior,
     * or focusing the widget.
     *
     * @param InGeometry    Current widget geometry (layout, size, position).
     * @param PointerEvent  Detailed information about the mouse press,
     *                      including cursor position and pressed button.
     *
     * @return FReply       The Slate event reply indicating whether the event
     *                      was handled and what action should follow.
     */
    FReply OnMouseButtonClickedHandler(const FGeometry& InGeometry, const FPointerEvent& PointerEvent);

    /**
     * @brief Mouse double-click handler for this table row widget.
     *
     * This function is bound to the Slate event `OnMouseDoubleClick`
     * and is triggered when the user performs a double-click within
     * the row area.
     * Typical use cases: opening the asset, navigating to details,
     * or performing an action associated with the row.
     *
     * @param InGeometry    Current widget geometry at the moment of the event.
     * @param PointerEvent  Information about the double-click interaction,
     *                      including button, cursor location, and modifiers.
     *
     * @return FReply       The Slate reply determining how the double-click
     *                      event should be processed.
     */
    FReply OnMouseDoubleButtonClickedHandler(const FGeometry& InGeometry, const FPointerEvent& PointerEvent);

private:
    /** @brief Flag indicating if the asset has unsaved changes */
    bool bIsDirty ;

    /** @brief The asset data represented by this row */
    TSharedPtr<FAssetData> Item = nullptr;

    /** @brief Widget displaying dirty state indicator */
    TSharedPtr<SImage> DirtyBrushWidget = nullptr;

    /** @brief Delegate instance for asset rename events */
    FOnAssetRenamed OnAssetRenamed{};

    /** @brief Delegate instance for context menu creation */
    FOnCreateContextMenu OnCreateContextMenu{};

    /** @brief Delegate instance for asset double-click events */
    FOnAssetDoubleClicked OnAssetDoubleClicked{};

    /** @brief Delegate instance for editable text registration */
    FOnRegisterEditableText OnRegisterEditableText{};

    /** @brief Delegate instance for mouse button down events */
    FOnAssetMouseButtonDown MouseButtonDown{};

    /** @brief Handle for package dirty state change delegate */
    FDelegateHandle OnPackageDirtyStateChangedHandle{};

    /** @brief Handle for package dirty Saved change delegate */
    FDelegateHandle OnPackageSavedHandle{};

    FString CurrentPackageName;

    //void PackageSavedDesc(const FString& SavedPackageFileName, UObject* PackageObj);
    //void PackageDirtyDesc(UPackage* DirtyPackage, bool IsDirty);

};