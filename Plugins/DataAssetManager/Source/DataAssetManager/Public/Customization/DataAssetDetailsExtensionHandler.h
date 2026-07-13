// Copyright (c) 2026 DimAlek. All Rights Reserved.

#pragma once

#include "Containers/Map.h"
#include "Containers/Set.h"
#include "IDetailPropertyExtensionHandler.h"

class IPropertyUtilities;
class SWidget;

/**
 * @brief Detail panel extension handler that adds custom controls for supported Data Asset property rows.
 */
class FDataAssetDetailsExtensionHandler final : public IDetailPropertyExtensionHandler
{
public:
	/**
	 * @brief Checks whether the supplied property handle can receive custom detail row UI.
	 * @param InObjectClass Class that owns the property.
	 * @param PropertyHandle Property handle being inspected.
	 * @return true when the handler can extend the property row.
	 */
	virtual bool IsPropertyExtendable(const UClass* InObjectClass, const IPropertyHandle& PropertyHandle) const override;

	/**
	 * @brief Adds custom widgets to an extendable property row.
	 * @param InWidgetRow Detail row being extended.
	 * @param InDetailBuilder Detail layout builder that owns the row.
	 * @param InObjectClass Class that owns the property.
	 * @param PropertyHandle Property handle being extended.
	 */
	virtual void ExtendWidgetRow(FDetailWidgetRow& InWidgetRow, const IDetailLayoutBuilder& InDetailBuilder, const UClass* InObjectClass, TSharedPtr<IPropertyHandle> PropertyHandle) override;

private:
	struct FRandomNumericRange
	{
		double Min = 0.0;
		double Max = 0.0;
	};

	FRandomNumericRange& GetOrCreateRandomNumericRange(const TSharedPtr<IPropertyHandle>& PropertyHandle);
	void SetRandomNumericRangeMin(const TSharedPtr<IPropertyHandle>& PropertyHandle, double Value);
	void SetRandomNumericRangeMax(const TSharedPtr<IPropertyHandle>& PropertyHandle, double Value);
	bool IsRandomRangeExpanded(const TSharedPtr<IPropertyHandle>& PropertyHandle) const;
	void ToggleRandomRangeExpanded(const TSharedPtr<IPropertyHandle>& PropertyHandle);
	TSharedRef<SWidget> MakeRandomRangeToggleButton(const TSharedPtr<IPropertyHandle>& PropertyHandle);
	TSharedRef<SWidget> MakeRandomRangeControls(const TSharedPtr<IPropertyHandle>& PropertyHandle);
	TSharedRef<SWidget> MakeRandomizeButton(const TSharedPtr<IPropertyHandle>& PropertyHandle, const TWeakPtr<IPropertyUtilities>& WeakPropertyUtilities);
	void RandomizeProperty(const TSharedPtr<IPropertyHandle>& PropertyHandle, const TWeakPtr<IPropertyUtilities> WeakPropertyUtilities);

	TMap<FString, FRandomNumericRange> RandomNumericRanges;
	TSet<FString> ExpandedRandomRangeKeys;
};
