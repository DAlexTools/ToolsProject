// Copyright (c) 2026 DimAlek. All Rights Reserved.

#include "Customization/DataAssetDetailsExtensionHandler.h"
#include "DeveloperSettings/DataAssetManagerSettings.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/DataAsset.h"
#include "IPropertyUtilities.h"
#include "Logging/DataAssetManagerLog.h"
#include "Math/Color.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "String/LexFromString.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"
#include "UObject/UnrealType.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "DataAssetDetailsExtensionHandler"

namespace
{
	const FName ClampMinMetaDataName(TEXT("ClampMin"));
	const FName ClampMaxMetaDataName(TEXT("ClampMax"));

	constexpr float ArrayMoveControlsWidth = 44.0f;
	constexpr float RandomRangeControlsWidth = 204.0f;
	constexpr float RandomRangeToggleControlWidth = 24.0f;
	constexpr float RandomNumericControlWidth = 24.0f;

	int32 GetArrayElementIndex(const IPropertyHandle& PropertyHandle)
	{
		const int32 IndexInArray = PropertyHandle.GetIndexInArray();
		return IndexInArray != INDEX_NONE ? IndexInArray : PropertyHandle.GetArrayIndex();
	}

	int32 GetArrayElementIndex(const TSharedPtr<IPropertyHandle>& PropertyHandle)
	{
		if (!PropertyHandle.IsValid())
		{
			return INDEX_NONE;
		}

		return GetArrayElementIndex(*PropertyHandle);
	}

	bool IsArrayElementMoveExtendable(const IPropertyHandle& PropertyHandle)
	{
		const TSharedPtr<IPropertyHandle> ParentHandle = PropertyHandle.GetParentHandle();
		const TSharedPtr<IPropertyHandleArray> ArrayHandle = ParentHandle.IsValid() ? ParentHandle->AsArray() : nullptr;
		return GetArrayElementIndex(PropertyHandle) != INDEX_NONE && ArrayHandle.IsValid();
	}

	const FNumericProperty* GetSupportedNumericProperty(const IPropertyHandle& PropertyHandle)
	{
		const FNumericProperty* NumericProperty = CastField<const FNumericProperty>(PropertyHandle.GetProperty());
		if (!NumericProperty || NumericProperty->IsEnum())
		{
			return nullptr;
		}

		return NumericProperty->IsInteger() || NumericProperty->IsFloatingPoint()
			? NumericProperty
			: nullptr;
	}

	const FNumericProperty* GetSupportedNumericProperty(const TSharedPtr<IPropertyHandle>& PropertyHandle)
	{
		return PropertyHandle.IsValid()
			? GetSupportedNumericProperty(*PropertyHandle)
			: nullptr;
	}

	const FStructProperty* GetSupportedColorStructProperty(const IPropertyHandle& PropertyHandle)
	{
		const FStructProperty* StructProperty = CastField<const FStructProperty>(PropertyHandle.GetProperty());
		if (!StructProperty)
		{
			return nullptr;
		}

		return StructProperty->Struct == TBaseStructure<FColor>::Get() || StructProperty->Struct == TBaseStructure<FLinearColor>::Get()
			? StructProperty
			: nullptr;
	}

	const FStructProperty* GetSupportedColorStructProperty(const TSharedPtr<IPropertyHandle>& PropertyHandle)
	{
		return PropertyHandle.IsValid()
			? GetSupportedColorStructProperty(*PropertyHandle)
			: nullptr;
	}

	bool IsPropertyRandomizable(const IPropertyHandle& PropertyHandle)
	{
		return GetSupportedNumericProperty(PropertyHandle) != nullptr
			|| GetSupportedColorStructProperty(PropertyHandle) != nullptr;
	}

	bool CanRandomizeProperty(const TSharedPtr<IPropertyHandle>& PropertyHandle)
	{
		return PropertyHandle.IsValid()
			&& PropertyHandle->IsEditable()
			&& (GetSupportedNumericProperty(PropertyHandle) != nullptr || GetSupportedColorStructProperty(PropertyHandle) != nullptr);
	}

	bool GetArrayMoveData(const TSharedPtr<IPropertyHandle>& PropertyHandle, int32 Offset, TSharedPtr<IPropertyHandle>& OutParentHandle, TSharedPtr<IPropertyHandleArray>& OutArrayHandle, int32& OutCurrentIndex, int32& OutTargetIndex)
	{
		OutParentHandle = PropertyHandle.IsValid() ? PropertyHandle->GetParentHandle() : nullptr;
		OutArrayHandle = OutParentHandle.IsValid() ? OutParentHandle->AsArray() : nullptr;
		if (!PropertyHandle.IsValid() || !OutParentHandle.IsValid() || !OutArrayHandle.IsValid())
		{
			return false;
		}

		OutCurrentIndex = GetArrayElementIndex(PropertyHandle);
		if (OutCurrentIndex == INDEX_NONE)
		{
			return false;
		}

		uint32 NumElements = 0;
		if (OutArrayHandle->GetNumElements(NumElements) != FPropertyAccess::Success)
		{
			return false;
		}

		OutTargetIndex = OutCurrentIndex + Offset;
		return OutTargetIndex >= 0 && OutTargetIndex < static_cast<int32>(NumElements);
	}

	bool CanMoveElement(const TSharedPtr<IPropertyHandle>& PropertyHandle, int32 Offset)
	{
		TSharedPtr<IPropertyHandle> ParentHandle;
		TSharedPtr<IPropertyHandleArray> ArrayHandle;
		int32 CurrentIndex = INDEX_NONE;
		int32 TargetIndex = INDEX_NONE;
		return GetArrayMoveData(PropertyHandle, Offset, ParentHandle, ArrayHandle, CurrentIndex, TargetIndex);
	}

	void MoveElementToNeighbor(const TSharedPtr<IPropertyHandle>& PropertyHandle, const TWeakPtr<IPropertyUtilities> WeakPropertyUtilities, int32 Offset);

	TSharedRef<SWidget> MakeMoveButton(const TSharedPtr<IPropertyHandle>& PropertyHandle,const TWeakPtr<IPropertyUtilities>& WeakPropertyUtilities,int32 Offset, const FName IconName, const FText& ToolTipText)
	{
		return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(1.0f))
			.ToolTipText(ToolTipText)
			.IsEnabled_Lambda([PropertyHandle, Offset]()
				{
					return CanMoveElement(PropertyHandle, Offset);
				})
			.OnClicked_Lambda([PropertyHandle, WeakPropertyUtilities, Offset]()
				{
					MoveElementToNeighbor(PropertyHandle, WeakPropertyUtilities, Offset);
					return FReply::Handled();
				})
			[
				SNew(SImage)
					.Image(FAppStyle::GetBrush(IconName))
			];
	}

	bool TryGetIntegerMetaData(const IPropertyHandle& PropertyHandle, const FName& MetaDataName, int64& OutValue)
	{
		if (!PropertyHandle.HasMetaData(MetaDataName))
		{
			return false;
		}

		const FString& MetaDataValue = PropertyHandle.GetMetaData(MetaDataName);
		if (LexTryParseString(OutValue, *MetaDataValue))
		{
			return true;
		}

		double FloatingPointValue = 0.0;
		if (LexTryParseString(FloatingPointValue, *MetaDataValue))
		{
			OutValue = static_cast<int64>(FloatingPointValue);
			return true;
		}

		return false;
	}

	bool TryGetFloatingPointMetaData(const IPropertyHandle& PropertyHandle, const FName& MetaDataName, double& OutValue)
	{
		if (!PropertyHandle.HasMetaData(MetaDataName))
		{
			return false;
		}

		return LexTryParseString(OutValue, *PropertyHandle.GetMetaData(MetaDataName));
	}

	FString GetRandomRangeKey(const IPropertyHandle& PropertyHandle)
	{
		FString PropertyPath = PropertyHandle.GeneratePathToProperty();
		if (!PropertyPath.IsEmpty())
		{
			return PropertyPath;
		}

		const FProperty* Property = PropertyHandle.GetProperty();
		return Property ? Property->GetPathName() : FString();
	}

	void GetDefaultRandomNumericRange(const IPropertyHandle& PropertyHandle, const FNumericProperty& NumericProperty, double& OutMinValue, double& OutMaxValue)
	{
		if (NumericProperty.IsInteger())
		{
			const UDataAssetManagerSettings* Settings = GetDefault<UDataAssetManagerSettings>();
			int64 MinValue = Settings ? Settings->RandomIntegerClampMin : 0;
			int64 MaxValue = Settings ? Settings->RandomIntegerClampMax : 100;
			TryGetIntegerMetaData(PropertyHandle, ClampMinMetaDataName, MinValue);
			TryGetIntegerMetaData(PropertyHandle, ClampMaxMetaDataName, MaxValue);

			if (!NumericProperty.CanHoldValue(static_cast<int64>(-1)))
			{
				MinValue = FMath::Max<int64>(MinValue, 0);
				MaxValue = FMath::Max<int64>(MaxValue, 0);
			}

			if (MinValue > MaxValue)
			{
				MaxValue = MinValue;
			}

			OutMinValue = static_cast<double>(FMath::Clamp<int64>(MinValue, MIN_int32, MAX_int32));
			OutMaxValue = static_cast<double>(FMath::Clamp<int64>(MaxValue, MIN_int32, MAX_int32));
			if (OutMinValue > OutMaxValue)
			{
				OutMaxValue = OutMinValue;
			}
			return;
		}

		const UDataAssetManagerSettings* Settings = GetDefault<UDataAssetManagerSettings>();
		double MinValue = Settings ? Settings->RandomFloatClampMin : 0.0;
		double MaxValue = Settings ? Settings->RandomFloatClampMax : 1.0;
		TryGetFloatingPointMetaData(PropertyHandle, ClampMinMetaDataName, MinValue);
		TryGetFloatingPointMetaData(PropertyHandle, ClampMaxMetaDataName, MaxValue);

		if (MinValue > MaxValue)
		{
			MaxValue = MinValue;
		}

		OutMinValue = MinValue;
		OutMaxValue = MaxValue;
	}

	FString GenerateRandomIntegerValueText(const FNumericProperty& NumericProperty, double MinValue, double MaxValue)
	{
		int64 IntegerMinValue = static_cast<int64>(MinValue);
		int64 IntegerMaxValue = static_cast<int64>(MaxValue);

		if (!NumericProperty.CanHoldValue(static_cast<int64>(-1)))
		{
			IntegerMinValue = FMath::Max<int64>(IntegerMinValue, 0);
			IntegerMaxValue = FMath::Max<int64>(IntegerMaxValue, 0);
		}

		if (IntegerMinValue > IntegerMaxValue)
		{
			IntegerMaxValue = IntegerMinValue;
		}

		IntegerMinValue = FMath::Clamp<int64>(IntegerMinValue, MIN_int32, MAX_int32);
		IntegerMaxValue = FMath::Clamp<int64>(IntegerMaxValue, MIN_int32, MAX_int32);

		if (IntegerMinValue > IntegerMaxValue)
		{
			IntegerMaxValue = IntegerMinValue;
		}

		const int32 RandomValue = FMath::RandRange(static_cast<int32>(IntegerMinValue), static_cast<int32>(IntegerMaxValue));
		return FString::FromInt(RandomValue);
	}

	FString GenerateRandomFloatingPointValueText(double MinValue, double MaxValue)
	{
		if (MinValue > MaxValue)
		{
			MaxValue = MinValue;
		}

		const double RandomValue = FMath::Lerp(MinValue, MaxValue, static_cast<double>(FMath::FRand()));
		return FString::SanitizeFloat(RandomValue);
	}

	FString GenerateRandomColorValueText(const FStructProperty& StructProperty)
	{
		if (StructProperty.Struct == TBaseStructure<FColor>::Get())
		{
			return FColor(
				FMath::RandRange(0, 255),
				FMath::RandRange(0, 255),
				FMath::RandRange(0, 255),
				255).ToString();
		}

		if (StructProperty.Struct == TBaseStructure<FLinearColor>::Get())
		{
			return FLinearColor(FMath::FRand(), FMath::FRand(), FMath::FRand(), 1.0f).ToString();
		}

		return FString();
	}

	TSharedRef<SWidget> MakeMoveControls( const TSharedPtr<IPropertyHandle>& PropertyHandle, const TWeakPtr<IPropertyUtilities>& WeakPropertyUtilities)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				MakeMoveButton(
					PropertyHandle,
					WeakPropertyUtilities,
					-1,
					TEXT("Icons.ArrowUp"),
					LOCTEXT("MoveArrayElementUpTooltip", "Move array element up"))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				MakeMoveButton(
					PropertyHandle,
					WeakPropertyUtilities,
					1,
					TEXT("Icons.ArrowDown"),
					LOCTEXT("MoveArrayElementDownTooltip", "Move array element down"))
			];
	}

	void MoveElementToNeighbor(const TSharedPtr<IPropertyHandle>& PropertyHandle, const TWeakPtr<IPropertyUtilities> WeakPropertyUtilities, int32 Offset)
	{
		TSharedPtr<IPropertyHandle> ParentHandle;
		TSharedPtr<IPropertyHandleArray> ArrayHandle;
		int32 CurrentIndex = INDEX_NONE;
		int32 TargetIndex = INDEX_NONE;
		if (!GetArrayMoveData(PropertyHandle, Offset, ParentHandle, ArrayHandle, CurrentIndex, TargetIndex))
		{
			UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to resolve array element move context"));
			return;
		}

		FScopedTransaction Transaction(LOCTEXT("MoveArrayElementTransaction", "Move Array Element"));

		ParentHandle->NotifyPreChange();
		const FPropertyAccess::Result MoveResult = ArrayHandle->MoveElementTo(CurrentIndex, TargetIndex);
		if (MoveResult != FPropertyAccess::Success)
		{
			Transaction.Cancel();
			UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to move array element from %d to %d"), CurrentIndex, TargetIndex);
			return;
		}

		ParentHandle->NotifyPostChange(EPropertyChangeType::ArrayMove);
		const FPropertyChangedEvent MoveEvent(ParentHandle->GetProperty(), EPropertyChangeType::ArrayMove);

		if (const TSharedPtr<IPropertyUtilities> PropertyUtilities = WeakPropertyUtilities.Pin())
		{
			PropertyUtilities->NotifyFinishedChangingProperties(MoveEvent);
			PropertyUtilities->RequestForceRefresh();
		}
		else
		{
			ParentHandle->NotifyFinishedChangingProperties();
			ParentHandle->RequestRebuildChildren();
		}
	}

}

FDataAssetDetailsExtensionHandler::FRandomNumericRange& FDataAssetDetailsExtensionHandler::GetOrCreateRandomNumericRange(const TSharedPtr<IPropertyHandle>& PropertyHandle)
{
	const FString Key = PropertyHandle.IsValid() ? GetRandomRangeKey(*PropertyHandle) : FString();
	if (FRandomNumericRange* ExistingRange = RandomNumericRanges.Find(Key))
	{
		return *ExistingRange;
	}

	FRandomNumericRange NewRange;
	if (PropertyHandle.IsValid())
	{
		if (const FNumericProperty* NumericProperty = GetSupportedNumericProperty(PropertyHandle))
		{
			GetDefaultRandomNumericRange(*PropertyHandle, *NumericProperty, NewRange.Min, NewRange.Max);
		}
	}

	return RandomNumericRanges.Add(Key, NewRange);
}

void FDataAssetDetailsExtensionHandler::SetRandomNumericRangeMin(const TSharedPtr<IPropertyHandle>& PropertyHandle, double Value)
{
	FRandomNumericRange& Range = GetOrCreateRandomNumericRange(PropertyHandle);
	Range.Min = Value;
	if (Range.Min > Range.Max)
	{
		Range.Max = Range.Min;
	}
}

void FDataAssetDetailsExtensionHandler::SetRandomNumericRangeMax(const TSharedPtr<IPropertyHandle>& PropertyHandle, double Value)
{
	FRandomNumericRange& Range = GetOrCreateRandomNumericRange(PropertyHandle);
	Range.Max = Value;
	if (Range.Max < Range.Min)
	{
		Range.Min = Range.Max;
	}
}

bool FDataAssetDetailsExtensionHandler::IsRandomRangeExpanded(const TSharedPtr<IPropertyHandle>& PropertyHandle) const
{
	if (!PropertyHandle.IsValid())
	{
		return false;
	}

	return ExpandedRandomRangeKeys.Contains(GetRandomRangeKey(*PropertyHandle));
}

void FDataAssetDetailsExtensionHandler::ToggleRandomRangeExpanded(const TSharedPtr<IPropertyHandle>& PropertyHandle)
{
	if (!PropertyHandle.IsValid())
	{
		return;
	}

	const FString Key = GetRandomRangeKey(*PropertyHandle);
	if (ExpandedRandomRangeKeys.Contains(Key))
	{
		ExpandedRandomRangeKeys.Remove(Key);
	}
	else
	{
		ExpandedRandomRangeKeys.Add(Key);
	}
}

TSharedRef<SWidget> FDataAssetDetailsExtensionHandler::MakeRandomRangeToggleButton(const TSharedPtr<IPropertyHandle>& PropertyHandle)
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(1.0f))
		.ToolTipText(LOCTEXT("ToggleRandomRangeTooltip", "Show random range"))
		.OnClicked_Lambda([this, PropertyHandle]()
			{
				ToggleRandomRangeExpanded(PropertyHandle);
				return FReply::Handled();
			})
		[
			SNew(SBox)
				.WidthOverride(14.0f)
				.HeightOverride(14.0f)
				[
					SNew(SImage)
						.Image_Lambda([this, PropertyHandle]()
							{
								return IsRandomRangeExpanded(PropertyHandle)
									? FAppStyle::Get().GetBrush("TreeArrow_Expanded")
									: FAppStyle::Get().GetBrush("TreeArrow_Collapsed");
							})
				]
		];
}

TSharedRef<SWidget> FDataAssetDetailsExtensionHandler::MakeRandomRangeControls(const TSharedPtr<IPropertyHandle>& PropertyHandle)
{
	const FNumericProperty* NumericProperty = GetSupportedNumericProperty(PropertyHandle);
	const bool bIntegerProperty = NumericProperty && NumericProperty->IsInteger();
	const TOptional<int32> MinFractionalDigits = bIntegerProperty ? TOptional<int32>(0) : TOptional<int32>(1);
	const TOptional<int32> MaxFractionalDigits = bIntegerProperty ? TOptional<int32>(0) : TOptional<int32>(3);
	const double Delta = bIntegerProperty ? 1.0 : 0.1;

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
					.WidthOverride(30.0f)
					[
						SNumericEntryBox<double>::BuildLabel(
							LOCTEXT("RandomRangeMinLabel", "Min"),
							FLinearColor::White,
							SNumericEntryBox<double>::RedLabelBackgroundColor)
					]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(64.0f)
				[
					SNew(SNumericEntryBox<double>)
						.AllowSpin(true)
						.Delta(Delta)
						.MinDesiredValueWidth(54.0f)
						.MinFractionalDigits(MinFractionalDigits)
						.MaxFractionalDigits(MaxFractionalDigits)
						.Value_Lambda([this, PropertyHandle]()
							{
								return TOptional<double>(GetOrCreateRandomNumericRange(PropertyHandle).Min);
							})
						.OnValueChanged_Lambda([this, PropertyHandle](double Value)
							{
								SetRandomNumericRangeMin(PropertyHandle, Value);
							})
						.OnValueCommitted_Lambda([this, PropertyHandle](double Value, ETextCommit::Type)
							{
								SetRandomNumericRangeMin(PropertyHandle, Value);
							})
				]
			]
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
					.WidthOverride(30.0f)
					[
						SNumericEntryBox<double>::BuildLabel(
							LOCTEXT("RandomRangeMaxLabel", "Max"),
							FLinearColor::White,
							SNumericEntryBox<double>::GreenLabelBackgroundColor)
					]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(64.0f)
				[
					SNew(SNumericEntryBox<double>)
						.AllowSpin(true)
						.Delta(Delta)
						.MinDesiredValueWidth(54.0f)
						.MinFractionalDigits(MinFractionalDigits)
						.MaxFractionalDigits(MaxFractionalDigits)
						.Value_Lambda([this, PropertyHandle]()
							{
								return TOptional<double>(GetOrCreateRandomNumericRange(PropertyHandle).Max);
							})
						.OnValueChanged_Lambda([this, PropertyHandle](double Value)
							{
								SetRandomNumericRangeMax(PropertyHandle, Value);
							})
						.OnValueCommitted_Lambda([this, PropertyHandle](double Value, ETextCommit::Type)
							{
								SetRandomNumericRangeMax(PropertyHandle, Value);
							})
				]
			]
		];
}

TSharedRef<SWidget> FDataAssetDetailsExtensionHandler::MakeRandomizeButton(const TSharedPtr<IPropertyHandle>& PropertyHandle, const TWeakPtr<IPropertyUtilities>& WeakPropertyUtilities)
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(1.0f))
		.ToolTipText(LOCTEXT("RandomizePropertyTooltip", "Randomize value"))
		.IsEnabled_Lambda([PropertyHandle]()
			{
				return CanRandomizeProperty(PropertyHandle);
			})
		.OnClicked_Lambda([this, PropertyHandle, WeakPropertyUtilities]()
			{
				RandomizeProperty(PropertyHandle, WeakPropertyUtilities);
				return FReply::Handled();
			})
		[
			SNew(SBox)
				.WidthOverride(16.0f)
				.HeightOverride(16.0f)
				[
					SNew(SImage)
						.Image(FSlateIcon(FName("StateTreeEditorStyle"), "StateTreeEditor.TrySelectChildrenAtRandom").GetIcon())

				]
		];
}

void FDataAssetDetailsExtensionHandler::RandomizeProperty(const TSharedPtr<IPropertyHandle>& PropertyHandle, const TWeakPtr<IPropertyUtilities> WeakPropertyUtilities)
{
	if (!CanRandomizeProperty(PropertyHandle))
	{
		return;
	}

	FString RandomValueText;
	if (const FNumericProperty* NumericProperty = GetSupportedNumericProperty(PropertyHandle))
	{
		const FRandomNumericRange& Range = GetOrCreateRandomNumericRange(PropertyHandle);
		RandomValueText = NumericProperty->IsInteger()
			? GenerateRandomIntegerValueText(*NumericProperty, Range.Min, Range.Max)
			: GenerateRandomFloatingPointValueText(Range.Min, Range.Max);
	}
	else if (const FStructProperty* StructProperty = GetSupportedColorStructProperty(PropertyHandle))
	{
		RandomValueText = GenerateRandomColorValueText(*StructProperty);
	}

	if (RandomValueText.IsEmpty())
	{
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to generate random property value"));
		return;
	}

	FScopedTransaction Transaction(LOCTEXT("RandomizePropertyTransaction", "Randomize Property"));
	const FPropertyAccess::Result SetResult = PropertyHandle->SetValueFromFormattedString(RandomValueText);
	if (SetResult != FPropertyAccess::Success)
	{
		Transaction.Cancel();
		UE_LOG(SDataAssetManagerLog, Warning, TEXT("Failed to set random property value '%s'"), *RandomValueText);
		return;
	}

	if (const TSharedPtr<IPropertyUtilities> PropertyUtilities = WeakPropertyUtilities.Pin())
	{
		const FPropertyChangedEvent ChangeEvent(PropertyHandle->GetProperty(), EPropertyChangeType::ValueSet);
		PropertyUtilities->NotifyFinishedChangingProperties(ChangeEvent);
		PropertyUtilities->RequestForceRefresh();
	}
	else
	{
		PropertyHandle->NotifyFinishedChangingProperties();
		PropertyHandle->RequestRebuildChildren();
	}
}

bool FDataAssetDetailsExtensionHandler::IsPropertyExtendable(const UClass* InObjectClass, const IPropertyHandle& PropertyHandle) const
{
	if (!InObjectClass || !InObjectClass->IsChildOf(UDataAsset::StaticClass()))
	{
		return false;
	}

	return IsArrayElementMoveExtendable(PropertyHandle) || IsPropertyRandomizable(PropertyHandle);
}

void FDataAssetDetailsExtensionHandler::ExtendWidgetRow(FDetailWidgetRow& InWidgetRow, const IDetailLayoutBuilder& InDetailBuilder, const UClass* InObjectClass,TSharedPtr<IPropertyHandle> PropertyHandle)
{
	if (!PropertyHandle.IsValid())
	{
		return;
	}

	static_cast<void>(InObjectClass);

	const bool bShowMoveControls = IsArrayElementMoveExtendable(*PropertyHandle);
	const bool bShowRandomRangeControls = GetSupportedNumericProperty(*PropertyHandle) != nullptr;
	const bool bShowRandomizeButton = IsPropertyRandomizable(*PropertyHandle);
	if (!bShowMoveControls && !bShowRandomizeButton)
	{
		return;
	}

	const TWeakPtr<IPropertyUtilities> WeakPropertyUtilities = InDetailBuilder.GetPropertyUtilities();
	const TSharedRef<SWidget> ExistingValueWidget = InWidgetRow.ValueWidget.Widget;
	const EHorizontalAlignment ExistingValueHorizontalAlignment = InWidgetRow.ValueWidget.HorizontalAlignment;
	const EVerticalAlignment ExistingValueVerticalAlignment = InWidgetRow.ValueWidget.VerticalAlignment;
	const TOptional<float> ExistingMinWidth = InWidgetRow.ValueWidget.MinWidth;
	const TOptional<float> ExistingMaxWidth = InWidgetRow.ValueWidget.MaxWidth;

	float ExtraWidth = 0.0f;
	if (bShowMoveControls)
	{
		ExtraWidth += ArrayMoveControlsWidth;
	}
	if (bShowRandomizeButton)
	{
		if (bShowRandomRangeControls)
		{
			ExtraWidth += RandomRangeToggleControlWidth;
			ExtraWidth += RandomRangeControlsWidth;
		}
		ExtraWidth += RandomNumericControlWidth;
	}

	TOptional<float> ExtendedMinWidth = ExistingMinWidth;
	TOptional<float> ExtendedMaxWidth = ExistingMaxWidth;
	if (ExtendedMinWidth.IsSet())
	{
		ExtendedMinWidth = ExtendedMinWidth.GetValue() + ExtraWidth;
	}
	if (ExtendedMaxWidth.IsSet())
	{
		ExtendedMaxWidth = ExtendedMaxWidth.GetValue() + ExtraWidth;
	}

	const TSharedRef<SHorizontalBox> ValueContent = SNew(SHorizontalBox);
	ValueContent->AddSlot()
	.AutoWidth()
	.VAlign(VAlign_Center)
	[
		ExistingValueWidget
	];

	if (bShowMoveControls)
	{
		ValueContent->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(4.0f, 1.0f, 0.0f, 1.0f)
		[
			MakeMoveControls(PropertyHandle, WeakPropertyUtilities)
		];
	}

	if (bShowRandomizeButton)
	{
		if (bShowRandomRangeControls)
		{
			ValueContent->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(4.0f, 1.0f, 0.0f, 1.0f)
			[
				MakeRandomRangeToggleButton(PropertyHandle)
			];

			ValueContent->AddSlot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.Padding(2.0f, 1.0f, 0.0f, 1.0f)
			[
				SNew(SBox)
				.Visibility_Lambda([this, PropertyHandle]()
					{
						return IsRandomRangeExpanded(PropertyHandle) ? EVisibility::Visible : EVisibility::Collapsed;
					})
				[
					MakeRandomRangeControls(PropertyHandle)
				]
			];
		}

		ValueContent->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(4.0f, 1.0f, 0.0f, 1.0f)
		[
			MakeRandomizeButton(PropertyHandle, WeakPropertyUtilities)
		];
	}

	InWidgetRow.ValueContent()
	.HAlign(ExistingValueHorizontalAlignment)
	.VAlign(ExistingValueVerticalAlignment)
	.MinDesiredWidth(ExtendedMinWidth)
	.MaxDesiredWidth(ExtendedMaxWidth)
	[
		ValueContent
	];
}

#undef LOCTEXT_NAMESPACE
