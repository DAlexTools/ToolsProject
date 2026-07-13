// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailRootObjectCustomization.h"

class UDataAsset;

/**
 * @brief Customizes root object headers displayed by the Details panel.
 */
class FDetailsRootObjectCustomization : public IDetailRootObjectCustomization
{
public:
	/**
	 * @brief Creates a custom header widget for a Details panel root object set.
	 * @param InRootObjectSet Root objects currently displayed by the details view.
	 * @param InTableRow Table row associated with the customized root object.
	 * @return Header widget to display, or nullptr when default behavior should be used.
	 */
	virtual TSharedPtr<SWidget> CustomizeObjectHeader(const FDetailsObjectSet& InRootObjectSet, const TSharedPtr<ITableRow>& InTableRow) override;

	/**
	 * @brief Returns how expansion arrows are displayed for the customized header.
	 * @return Custom arrow mode because expansion is handled by the header widget.
	 */
	virtual EExpansionArrowUsage GetExpansionArrowUsage() const override;

private:
	/**
	 * @brief Builds the action menu displayed by the custom root object header.
	 * @param InRootObjectSet Root objects represented by the header.
	 * @return Menu widget containing root object actions.
	 */
	TSharedRef<SWidget> BuildHeaderMenu(const FDetailsObjectSet& InRootObjectSet);

	/**
	 * @brief Exports the cached root Data Asset to a JSON file.
	 */
	void ExportToJson();

	/**
	 * @brief Imports JSON data into the cached root Data Asset.
	 */
	void ImportFromJson();

	/**
	 * @brief Copies the cached root Data Asset reference to the clipboard.
	 */
	void CopyReferenceToClipboard() const;

	/**
	 * @brief Copies the cached root Data Asset package filename to the clipboard.
	 */
	void CopyPathToClipboard() const;

	/**
	 * @brief Runs Data Validation for the cached root Data Asset.
	 */
	void ValidateDataAsset() const;

	/**
	 * @brief Resolves the cached root object as a mutable Data Asset.
	 * @return Root Data Asset, or nullptr when the current root object is invalid.
	 */
	UDataAsset* GetRootDataAsset() const;

	/** @brief Root object set currently represented by this customization. */
	FDetailsObjectSet CachedRootObjectSet;
};
