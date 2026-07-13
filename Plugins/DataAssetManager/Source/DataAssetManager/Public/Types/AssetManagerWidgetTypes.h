// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "Types/DataAssetValidationTypes.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Views/SListView.h"

/**
 * @brief Stores delegate handles registered by the Data Asset Manager widget.
 */
struct FManagerDelegateHandles final
{
	FDelegateHandle AssetAddedDelegateHandle{};
	FDelegateHandle AssetRemovedDelegateHandle{};
	FDelegateHandle AssetRenamedDelegateHandle{};
	FDelegateHandle FilesLoadedHandle{};
	FDelegateHandle PackageDirtyStateChangedHandle{};
};

/**
 * @brief Holds Slate widgets owned by the Data Asset Manager panel.
 */
struct FAssetManagerWidgets final
{
	TSharedPtr<class SWidget> MenuBar = nullptr;
	TSharedPtr<class SSplitter> Splitter = nullptr;
	TSharedPtr<class IDetailsView> DetailsView = nullptr;
	TSharedPtr<class SComboButton> ComboButton = nullptr;
	TSharedPtr<class SEditableText> EditableTextWidget = nullptr;
	TSharedPtr<class SFilterSearchBox> ListViewSearchBox = nullptr;
	TSharedPtr<class SListView<TSharedPtr<FAssetData>>> AssetListView = nullptr;

	bool IsValidDetailsView() const
	{
		return DetailsView.IsValid();
	}

	bool IsValidAssetListView() const
	{
		return AssetListView.IsValid();
	}

	bool IsValidComboButton() const
	{
		return ComboButton.IsValid();
	}
};

/**
 * @brief Runtime data collections and filters used by the asset manager panel.
 */
struct FAssetManagerData
{
	TArray<TSharedPtr<FAssetData>> DataAssets;
	TArray<TSharedPtr<FAssetData>> FilteredDataAssets;
	TArray<TSharedPtr<FAssetData>> DeletionDataAssets;
	TSharedPtr<FAssetData> SelectedAsset = nullptr;
	TSet<FString> ActiveFilters;
	TSet<FString> ActivePluginFilters;
	TSet<FName> InvalidAssetPackages;
	TMap<FName, FDataAssetValidationState> ValidationStates;
	bool bShowModifiedOnly = false;
	bool bShowInvalidOnly = false;

	bool IsValidSelectedAsset() const
	{
		return SelectedAsset.IsValid();
	}
};

/**
 * @brief Tracks editable text widgets used for inline asset rename operations.
 */
struct FEditableWidgets final
{
	TMap<TPair<FName, FName>, TSharedPtr<SEditableText>> EditableTextWidgets;
	bool bCanRename = true;
	bool bRenamedProgress = false;

	/**
	 * @brief Registers an editable text widget for a specific asset.
	 * @param AssetData Asset data used as the widget lookup key.
	 * @param EditableText Editable text widget associated with the asset.
	 */
	FORCEINLINE void AddEditableTextWidget(const FAssetData* AssetData, const TSharedPtr<SEditableText>& EditableText)
	{
		if (!AssetData || !EditableText.IsValid())
		{
			return;
		}

		EditableTextWidgets.Add({ AssetData->PackagePath, AssetData->AssetName }, EditableText);
	}
};
