// Copyright 2025 DimAlek. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Models/TextureChannelPackerTypes.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

/** 
 * @class STextureChannelPackerWidget
 * @brief Main Slate widget for configuring and executing texture channel packing opeations.
 * 
 * Provides the user interface for creating texture packing requests, previewing
 * results, managing presets, and generating packed or unpacked textures.
 */
class TEXTURECHANNELPACKER_API STextureChannelPackerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STextureChannelPackerWidget) {}
	SLATE_END_ARGS()

	/**
	 * @brief Constructs the widget and initializes its UI.
	 *
	 * @param InArgs Slate construction arguments.
	 */
	void Construct(const FArguments& InArgs);

private:

	/** Initializes combo box options and default widget state. */
	void InitializeOptions();

	/** Applies the specified preset to the current widget state. */
	void ApplyPreset(const FTextureChannelPackerPreset& Preset);

	/** Refreshes the list of available presets. */
	void RefreshPresetOptions();

	/** Populates texture slots using the current Content Browser selection. */
	void ApplySelectedContentBrowserTextures();

	/** Builds the toolbar section. */
	TSharedRef<SWidget> BuildToolbar();

	/** Builds the action button panel. */
	TSharedRef<SWidget> BuildActionPanel();

	/** Builds the output settings section. */
	TSharedRef<SWidget> BuildOutputSettings();

	/** Builds the preview panel. */
	TSharedRef<SWidget> BuildPreviewPanel();

	/** Builds a preview mode selection button. */
	TSharedRef<SWidget> BuildPreviewModeButton(ETextureChannelPackerPreviewMode Mode, const FText& Label);

	/** Builds the channel mapping UI for pack/repack operations. */
	TSharedRef<SWidget> BuildPackMappings();

	/** Builds the configuration UI for unpack operations. */
	TSharedRef<SWidget> BuildUnpackSettings();

	/** Builds a texture picker widget. */
	TSharedRef<SWidget> BuildTexturePicker(TWeakObjectPtr<UTexture>& TextureSlot);

	/** Builds the texture picker for a specific channel row. */
	TSharedRef<SWidget> BuildChannelRowTexturePicker(int32 RowIndex);

	/** Builds the source channel selection widget for a mapping row. */
	TSharedRef<SWidget> BuildChannelCombo(int32 RowIndex);

	/** Builds a generic string-based combo box. */
	TSharedRef<SWidget> BuildStringCombo( TArray<TSharedPtr<FString>>& Options, TSharedPtr<FString>& CurrentOption);

	/** Rebuilds UI panels affected by the current operation mode. */
	void RefreshDynamicPanels();

	/** Executes the selected texture channel packing operation. */
	FReply OnGenerateClicked();

	/** Generates a preview of the current request. */
	FReply OnPreviewClicked();

	FReply OnPreviewModeClicked(const ETextureChannelPackerPreviewMode Mode);


	/** Saves the current configuration as a preset. */
	FReply OnSavePresetClicked();

	/** Builds a processing request from the current UI state. */
	FTextureChannelPackerRequest BuildRequest() const;

	/** Displays an editor notification. */
	void ShowNotification(const FText& Message, bool bSuccess) const;

	/** Automatically generates an output asset name. */
	void AutoGenerateOutputName();

	/** Returns the visibility of the packing UI. */
	EVisibility GetPackVisibility() const;

	/** Returns the visibility of the unpacking UI. */
	EVisibility GetUnpackVisibility() const;

	/** Returns the current operation name for display. */
	FText GetOperationText() const;

	/** Returns the currently selected preset name. */
	FText GetPresetText() const;

	/** Returns the current preview status text. */
	FText GetPreviewStatusText() const;

	FText GetOutputChannelText(int32 RowIndex) const;

	void OnUseConstantChanged(ECheckBoxState NewState, int32 RowIndex);

	ECheckBoxState GetUseConstantState(int32 RowIndex) const;

	/** Returns the visibility of the preview image. */
	EVisibility GetPreviewImageVisibility() const;

	/** Returns the visibility of the preview placeholder. */
	EVisibility GetPreviewPlaceholderVisibility() const;

	/** Returns the color of a preview mode button. */
	FSlateColor GetPreviewModeButtonColor(ETextureChannelPackerPreviewMode Mode) const;

	/** Regenerates the preview texture. */
	void RefreshPreview();

	/** Current processing operation. */
	ETextureChannelPackerOperation Operation = ETextureChannelPackerOperation::Pack;

	/** Output package path. */
	FString OutputPackagePath = TEXT("/Game");

	/** Output asset base name. */
	FString OutputBaseName = TEXT("T_Packed_Texture");

	/** Output texture settings. */
	FTextureChannelPackerOutputSettings OutputSettings;

	/** Indicates whether the output name was manually edited by the user. */
	bool bOutputNameManuallyEdited = false;

	/** Channel mapping rows displayed in the UI. */
	TArray<FChannelRowState> ChannelRows;

	/** Source texture used for unpack operations. */
	TWeakObjectPtr<UTexture> UnpackSourceTexture;

	/** Output definitions for unpack operations. */
	TArray<FTextureChannelPackerUnpackOutput> UnpackOutputs;

	/** Container for pack mapping widgets. */
	TSharedPtr<SBox> PackMappingsPanel;

	/** Container for unpack settings widgets. */
	TSharedPtr<SBox> UnpackSettingsPanel;

	/** Preview texture generated from the current request. */
	TStrongObjectPtr<UTexture2D> PreviewTexture;

	/** Brush used to display the preview texture. */
	FSlateBrush PreviewBrush;

	/** Current preview status message. */
	FText PreviewStatusText;

	/** Active preview visualization mode. */
	ETextureChannelPackerPreviewMode PreviewMode = ETextureChannelPackerPreviewMode::Composite;

	/** Available operation options. */
	TArray<TSharedPtr<FString>> OperationOptions;

	/** Currently selected operation option. */
	TSharedPtr<FString> CurrentOperationOption;

	/** Available channel options. */
	TArray<TSharedPtr<FString>> ChannelOptions;

	/** Available preset names. */
	TArray<TSharedPtr<FString>> PresetOptions;

	/** Currently selected preset. */
	TSharedPtr<FString> CurrentPresetOption;

	/** Available compression settings. */
	TArray<TSharedPtr<FString>> CompressionOptions;

	/** Currently selected compression setting. */
	TSharedPtr<FString> CurrentCompressionOption;

	/** Available mip generation settings. */
	TArray<TSharedPtr<FString>> MipOptions;

	/** Currently selected mip generation setting. */
	TSharedPtr<FString> CurrentMipOption;

	/** Available texture filter settings. */
	TArray<TSharedPtr<FString>> FilterOptions;

	/** Currently selected texture filter. */
	TSharedPtr<FString> CurrentFilterOption;

	/** Available LOD group settings. */
	TArray<TSharedPtr<FString>> LODGroupOptions;

	/** Currently selected LOD group. */
	TSharedPtr<FString> CurrentLODGroupOption;
};
