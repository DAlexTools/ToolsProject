// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RevisionControlStyle/RevisionControlStyle.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"
#include "DataAssetManagerTypes.h"

class SLayeredImage;

/**
 * @brief Multi-column table row used to display and edit one Data Asset entry.
 */
class DATAASSETMANAGER_API SDataAssetTableRow final : public SMultiColumnTableRow<TSharedPtr<FAssetData>>
{
public:
	/** @brief Delegate fired when inline asset rename is committed. */
	DECLARE_DELEGATE_ThreeParams(FOnAssetRenamed, TSharedPtr<FAssetData>, const FText&, ETextCommit::Type);

	/** @brief Delegate fired to open an asset context menu. */
	DECLARE_DELEGATE_TwoParams(FOnCreateContextMenu, const FGeometry&, const FPointerEvent&);

	/** @brief Delegate fired when an asset row is double-clicked. */
	DECLARE_DELEGATE_TwoParams(FOnAssetDoubleClicked, const FGeometry&, const FPointerEvent&);

	/** @brief Delegate fired when the row creates an editable text widget. */
	DECLARE_DELEGATE_TwoParams(FOnRegisterEditableText, TSharedPtr<FAssetData>, TSharedRef<SEditableText>);

	/** @brief Delegate used to query cached validation state for a row asset. */
	DECLARE_DELEGATE_RetVal_OneParam(const FDataAssetValidationState*, FOnGetValidationState, TSharedPtr<FAssetData>);

	/** @brief Delegate fired for custom row mouse button handling. */
	DECLARE_DELEGATE_RetVal_TwoParams(FReply, FOnAssetMouseButtonDown, const FGeometry&, const FPointerEvent&);

	/** @brief Slate arguments for constructing a Data Asset table row. */
	SLATE_BEGIN_ARGS(SDataAssetTableRow) {}
		/** @brief Asset data represented by the row. */
		SLATE_ARGUMENT(TSharedPtr<FAssetData>, Item)

		/** @brief Owning Data Asset Manager widget. */
		SLATE_ARGUMENT(TSharedPtr<class SDataAssetManagerWidget>, Owner)

		/** @brief Callback invoked when an asset rename is committed. */
		SLATE_EVENT(FOnAssetRenamed, OnAssetRenamed)

		/** @brief Callback invoked to create a context menu. */
		SLATE_EVENT(FOnCreateContextMenu, OnCreateContextMenu)

		/** @brief Callback invoked on double-click. */
		SLATE_EVENT(FOnAssetDoubleClicked, OnAssetDoubleClicked)

		/** @brief Callback invoked when editable text is registered. */
		SLATE_EVENT(FOnRegisterEditableText, OnRegisterEditableText)

		/** @brief Callback used to query cached validation state. */
		SLATE_EVENT(FOnGetValidationState, OnGetValidationState)

		/** @brief Callback invoked for row mouse button input. */
		SLATE_EVENT(FOnAssetMouseButtonDown, OnMouseButtonDown)
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the row widget.
	 * @param InArgs Slate construction arguments.
	 * @param InOwnerTable Owning table view.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable);

	/**
	 * @brief Destroys the row and unregisters transient event handlers.
	 */
	virtual ~SDataAssetTableRow();

	/**
	 * @brief Creates the source control icon widget for the row.
	 * @return Source control icon widget.
	 */
	TSharedRef<SWidget> GenerateSourceControlIconWidget();

	/**
	 * @brief Generates content for a table column.
	 * @param ColumnId Column identifier.
	 * @return Widget displayed in the column.
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnId) override;

	/**
	 * @brief Ticks the row to keep transient state in sync.
	 * @param AllottedGeometry Geometry assigned to the widget.
	 * @param InCurrentTime Current absolute time.
	 * @param InDeltaTime Seconds since the previous tick.
	 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

protected:
	/**
	 * @brief Builds the asset name column with inline rename support.
	 * @return Name column widget.
	 */
	TSharedRef<SWidget> BuildNameColumnWidget();

	/**
	 * @brief Builds the validation status column.
	 * @return Validation column widget.
	 */
	TSharedRef<SWidget> BuildValidationColumnWidget();

	/** @brief Returns the cached validation state for this row, if one exists. */
	const FDataAssetValidationState* GetValidationState() const;

	/** @brief Returns compact validation label text. */
	FText GetValidationLabelText() const;

	/** @brief Returns detailed validation tooltip text. */
	FText GetValidationTooltipText() const;

	/** @brief Returns validation label color. */
	FSlateColor GetValidationColor() const;

	/**
	 * @brief Registers package dirty-state tracking for the asset package.
	 * @param PackageName Package name to track.
	 */
	void AddDirtyEventHandler(const FString& PackageName);

	/** @brief Refreshes the cached dirty-state from the represented package. */
	void RefreshDirtyState();

	/**
	 * @brief Handles changes to the active source control provider.
	 * @param OldProvider Previous provider.
	 * @param NewProvider New provider.
	 */
	void HandleSourceControlProviderChanged(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider);

	/** @brief Handles source control state changes for the row asset. */
	void HandleSourceControlStateChanged();

	/**
	 * @brief Handles mouse button input on the row.
	 * @param InGeometry Row geometry.
	 * @param MouseEvent Mouse event data.
	 * @return Reply describing whether the event was handled.
	 */
	FReply HandleMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& MouseEvent);

	/**
	 * @brief Handles row double-click input.
	 * @param InGeometry Row geometry.
	 * @param MouseEvent Mouse event data.
	 * @return Reply describing whether the event was handled.
	 */
	FReply HandleMouseDoubleClick(const FGeometry& InGeometry, const FPointerEvent& MouseEvent);

	/**
	 * @brief Returns the current display name for the row asset.
	 * @return Asset name text.
	 */
	FText GetAssetNameText() const;

	/**
	 * @brief Handles committed inline asset rename text.
	 * @param Text New asset name text.
	 * @param CommitType Commit method used by Slate.
	 */
	void OnAssetNameCommitted(const FText& Text, ETextCommit::Type CommitType);

private:
	/** @brief Whether the represented package has unsaved changes. */
	bool bIsDirty = false;

	/** @brief Asset data represented by this row. */
	TSharedPtr<FAssetData> Item = nullptr;

	/** @brief Dirty-state indicator widget. */
	TSharedPtr<SImage> DirtyBrushWidget = nullptr;

	/** @brief Package that owns the represented asset. */
	UPackage* AssetPackage = nullptr;

	/** @brief Rename callback supplied by the owner widget. */
	FOnAssetRenamed OnAssetRenamed{};

	/** @brief Context menu callback supplied by the owner widget. */
	FOnCreateContextMenu OnCreateContextMenu{};

	/** @brief Double-click callback supplied by the owner widget. */
	FOnAssetDoubleClicked OnAssetDoubleClicked{};

	/** @brief Editable text registration callback supplied by the owner widget. */
	FOnRegisterEditableText OnRegisterEditableText{};

	/** @brief Validation state callback supplied by the owner widget. */
	FOnGetValidationState OnGetValidationState{};

	/** @brief Mouse button callback supplied by the owner widget. */
	FOnAssetMouseButtonDown MouseButtonDown{};

	/** @brief Delegate handle for package dirty-state notifications. */
	FDelegateHandle OnPackageDirtyStateChangedHandle{};

	/** @brief Source control state widget. */
	TSharedPtr<SLayeredImage> SCCStateWidget{};

	/** @brief Whether a valid source control state brush is available. */
	bool bHasCCStateBrush = false;

	/** @brief Delegate handle for source control state changes. */
	FDelegateHandle SourceControlStateChangedDelegateHandle;
};
