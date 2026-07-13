// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/SHeaderRow.h"

/**
 * @brief Column identifiers used by the Data Asset list view.
 */
namespace DataAssetListColumns
{
	/** @brief Revision control state column identifier. */
	inline static const FName ColumnID_RC("RevisionControl");

	/** @brief Data validation state column identifier. */
	inline static const FName ColumnID_Validation("Validation");

	/** @brief Asset name column identifier. */
	inline static const FName ColumnID_Name("Name");

	/** @brief Asset type column identifier. */
	inline static const FName ColumnID_Type("Type");

	/** @brief Asset disk size column identifier. */
	inline static const FName ColumnID_DiskSize("DiskSize");

	/** @brief Asset package path column identifier. */
	inline static const FName ColumnID_Path("Path");
} // namespace DataAssetListColumns

/**
 * @brief Visibility flags for optional asset list columns.
 */
struct FColumnVisibilityFlags final
{
	bool bShowTypeColumn = true;
	bool bShowDiskSizeColumn = true;
	bool bShowPathColumn = true;
	bool bShowRevisionColumn = true;
	bool bShowValidationColumn = true;
};

/**
 * @brief Stores column order, visibility, and column builder callbacks for the asset list header.
 */
struct FColumnData final
{
	FColumnVisibilityFlags ColumnVisibility;

	inline static constexpr uint32 NumColumnAdders = 16;
	TMap<FName, TFunction<void(TSharedPtr<SHeaderRow>)>, TFixedSetAllocator<NumColumnAdders>> ColumnAdders;

	static constexpr uint32 NumColumnOrder = 7;
	TArray<FName, TFixedAllocator<NumColumnOrder>> ColumnOrder;

	/**
	 * @brief Resets the asset list column order to the default sequence.
	 */
	FORCEINLINE void InitializeColumnOrder()
	{
		ColumnOrder.Reset();
		ColumnOrder.Add(DataAssetListColumns::ColumnID_RC);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_Validation);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_Name);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_Type);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_DiskSize);
		ColumnOrder.Add(DataAssetListColumns::ColumnID_Path);
	}

	/**
	 * @brief Registers callbacks that add each supported column to a header row.
	 * @tparam AddColumnFunc Callable used for regular text columns.
	 * @tparam CreateRevisionFunc Callable used to create the revision control column.
	 * @param AddColumnToHeader Callback that appends a regular column to a header row.
	 * @param CreateRevisionControlColumn Callback that returns the revision control column arguments.
	 */
	template <typename AddColumnFunc, typename CreateRevisionFunc>
	void InitializeColumnAdders(AddColumnFunc&& AddColumnToHeader, CreateRevisionFunc&& CreateRevisionControlColumn)
	{
		ColumnAdders.Add(DataAssetListColumns::ColumnID_RC,
			[this, CreateRevisionControlColumn](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowRevisionColumn)
				{
					HeaderRow->AddColumn(CreateRevisionControlColumn());
				}
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_Validation,
			[this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowValidationColumn)
				{
					AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_Validation, TEXT("Validation"), 0.12f);
				}
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_Name,
			[this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_Name, TEXT("Name"), 0.4f);
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_Type,
			[this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowTypeColumn)
				{
					AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_Type, TEXT("Type"), 0.3f);
				}
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_DiskSize,
			[this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowDiskSizeColumn)
				{
					AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_DiskSize, TEXT("DiskSize"), 0.15f);
				}
			});

		ColumnAdders.Add(DataAssetListColumns::ColumnID_Path,
			[this, AddColumnToHeader](TSharedPtr<SHeaderRow> HeaderRow)
			{
				if (ColumnVisibility.bShowPathColumn)
				{
					AddColumnToHeader(HeaderRow, DataAssetListColumns::ColumnID_Path, TEXT("Path"), 0.3f);
				}
			});
	}

	/**
	 * @brief Rebuilds an existing header row using the current visibility flags.
	 * @param HeaderRow Header row to clear and repopulate.
	 */
	FORCEINLINE void UpdateColumnVisibility(TSharedPtr<SHeaderRow> HeaderRow)
	{
		if (!HeaderRow.IsValid())
		{
			return;
		}

		HeaderRow->ClearColumns();

		for (const FName& ColumnId : ColumnOrder)
		{
			if (const TFunction<void(TSharedPtr<SHeaderRow>)>* AddFunc = ColumnAdders.Find(ColumnId))
			{
				(*AddFunc)(HeaderRow);
			}
		}
	}

	/**
	 * @brief Builds a new header row from the registered column adders.
	 * @return New header row containing currently visible columns.
	 */
	FORCEINLINE TSharedRef<SHeaderRow> BuildHeaderRow() const
	{
		TSharedRef<SHeaderRow> HeaderRow = SNew(SHeaderRow).Cursor(EMouseCursor::Hand);
		const TSharedPtr<SHeaderRow> HeaderRowPtr = HeaderRow;

		for (const FName& ColumnId : ColumnOrder)
		{
			if (const TFunction<void(TSharedPtr<SHeaderRow>)>* const AddFunc = ColumnAdders.Find(ColumnId))
			{
				(*AddFunc)(HeaderRowPtr);
			}
		}

		return HeaderRow;
	}

	/**
	 * @brief Toggles all optional columns between visible and hidden states.
	 */
	FORCEINLINE void ToggleAllColumnsVisibility()
	{
		const bool bShouldHide = ColumnVisibility.bShowDiskSizeColumn || ColumnVisibility.bShowPathColumn || ColumnVisibility.bShowTypeColumn
			|| ColumnVisibility.bShowRevisionColumn || ColumnVisibility.bShowValidationColumn;
		const bool bNewVisibility = !bShouldHide;

		ColumnVisibility.bShowDiskSizeColumn = bNewVisibility;
		ColumnVisibility.bShowPathColumn = bNewVisibility;
		ColumnVisibility.bShowTypeColumn = bNewVisibility;
		ColumnVisibility.bShowRevisionColumn = bNewVisibility;
		ColumnVisibility.bShowValidationColumn = bNewVisibility;
	}

	/**
	 * @brief Checks whether every optional column is hidden.
	 * @return true when all optional columns are hidden.
	 */
	FORCEINLINE bool AreAllColumnsHidden() const
	{
		return !ColumnVisibility.bShowDiskSizeColumn && !ColumnVisibility.bShowPathColumn && !ColumnVisibility.bShowTypeColumn
			&& !ColumnVisibility.bShowRevisionColumn && !ColumnVisibility.bShowValidationColumn;
	}
};
