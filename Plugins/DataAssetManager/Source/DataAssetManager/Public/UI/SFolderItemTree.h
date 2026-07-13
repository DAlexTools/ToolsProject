// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataAssetManagerTypes.h"

/**
 * @brief Multi-column row widget that renders one folder tree node.
 */
class DATAASSETMANAGER_API SFolderItemTree final : public SMultiColumnTableRow<TSharedPtr<SFolderItemTree>>
{
public:
	/** @brief Slate arguments for constructing a folder tree row. */
	SLATE_BEGIN_ARGS(SFolderItemTree) {}

		/** @brief Folder node represented by this row. */
		SLATE_ARGUMENT(TSharedPtr<FAssetTreeFolderNode>, Item)

		/** @brief Search text highlighted in the folder label. */
		SLATE_ARGUMENT(FText, HightlightText)

	SLATE_END_ARGS()

	/**
	 * @brief Constructs the folder row widget.
	 * @param InArgs Slate construction arguments.
	 * @param InTable Owning table view.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InTable);

	/**
	 * @brief Generates content for a folder tree column.
	 * @param InColumnName Column identifier.
	 * @return Widget displayed in the column.
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;

private:
	/**
	 * @brief Returns the icon brush used for the folder node.
	 * @return Folder icon brush.
	 */
	const FSlateBrush* GetFolderIcon() const;

	/**
	 * @brief Returns the display color used for the folder node.
	 * @return Folder text/icon color.
	 */
	FSlateColor GetFolderColor() const;

	/** @brief Search text highlighted in the folder name. */
	FText HighlightText{};

	/** @brief Folder node represented by this row. */
	TSharedPtr<FAssetTreeFolderNode> Item{};
};
