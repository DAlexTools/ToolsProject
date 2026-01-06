// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailRootObjectCustomization.h"

/**
 * @brief Customization class for the root object header in the Details panel.
 */
class DATAASSETMANAGER_API FDetailsRootObjectCustomization : public IDetailRootObjectCustomization
{
public:
    /**
     * @brief Builds a custom header widget for the specified root object set.
     *
     * @param InRootObjectSet Set of root objects displayed in the Details panel.
     * @param InTableRow Table row associated with this header, if applicable.
     * @return Customized header widget.
     */
    virtual TSharedPtr<SWidget> CustomizeObjectHeader(const FDetailsObjectSet& InRootObjectSet, const TSharedPtr<ITableRow>& InTableRow) override;

    /**
     * @brief Returns custom arrow usage for expansion state handling.
     */
    virtual EExpansionArrowUsage GetExpansionArrowUsage() const
    {
        return EExpansionArrowUsage::Custom;
    }

private:
    /**
     * @brief Creates the expand/collapse button for a table row.
     *
     * Displays a plus or minus icon depending on the row’s expansion state.
     * Invokes OnExpandCollapseClicked on press to toggle the row state.
     *
     * @param InTableRow Table row for which the button is created.
     * @return Button widget.
     */
    [[nodiscard]] TSharedRef<SWidget> CreateExpandCollapseButton(TSharedPtr<ITableRow> InTableRow);

    /**
     * @brief Creates the button used to sync the object with the Content Browser.
     *
     * The button displays a search icon and triggers SyncBrowserObject_OnClicked
     * when pressed.
     *
     * @return Sync button widget.
     */
    [[nodiscard]] TSharedRef<SWidget> CreateSyncBrowserButton();

    /**
     * @brief Creates a ComboButton for the header menu.
     *
     * Used to display additional object-related actions.
     *
     * @return Header ComboButton widget.
     */
    [[nodiscard]] TSharedRef<SWidget> CreateHeaderComboButton();

    /** 
     * @brief Creates a DataAsset Icon for the header menu.
     * 
     * @return DataAsset Icon widget
     */
    [[nodiscard]] TSharedRef<SWidget> CreateHeaderDataAssetIcon();

    /**
     * @brief Creates a text block showing the main object's name.
     *
     * Displays the name of MainObject if valid, otherwise shows "Invalid Object".
     *
     * @return Text widget.
     */
    [[nodiscard]] TSharedRef<SWidget> CreateMainObjectText();

    /**
     * @brief Builds a context menu for actions on the root object in the header area.
     *
     * Includes operations such as:
     *  - Reset all properties to CDO defaults;
     *  - Export DataAsset properties to JSON;
     *  - Import properties from JSON.
     *  - !!!TODO More in the future add opearation
     * @return Menu widget.
     */
    [[nodiscard]] TSharedRef<SWidget> BuildHeaderMenu();

    /**
     * @brief Handles the sync button click, synchronizing the selected object
     *        with the Content Browser or other editor panels.
     *
     * @return Click reply.
     */
    [[nodiscard]] FReply SyncBrowserObject_OnClicked();

    /**
     * @brief Handles expand/collapse button clicks.
     *
     * Toggles the expansion state of the given table row.
     *
     * @param InTableRow Row to expand or collapse.
     * @return Handled reply.
     */
    [[nodiscard]] FReply OnExpandCollapseClicked(TSharedPtr<ITableRow> InTableRow);

    /**
     * @brief Returns the appropriate icon for the expand/collapse button.
     *
     * Chooses between expanded or collapsed icons based on row state.
     * If the row pointer is invalid, returns the default “plus” icon.
     *
     * @param InTableRow Row requiring an icon.
     * @return Slate brush icon pointer.
     */
    [[nodiscard]] const FSlateBrush* GetExpandCollapseIcon(TSharedPtr<ITableRow> InTableRow) const;

    /**
     * @brief Exports the first root DataAsset to a JSON file.
     *
     * Checks for valid root objects, generates the file path,
     * and calls SaveDataAssetToJsonFile(). Logs a message on success.
     */
    void ExportToJson();

    /**
     * @brief Imports DataAsset properties from a JSON file into the first root object.
     *
     * Retrieves the first root object, removes const if required,
     * casts to UDataAsset, and loads values from JSON.
     * Logs success when finished.
     */
    void ImportFromJson();

    /**
     * @brief Resets all root DataAssets to match their Class Default Object (CDO).
     *
     * Copies all properties from each object's CDO, then calls
     * PostEditChange() and MarkPackageDirty() to update the editor and mark changes.
     */
    void ResetToCDO();

private:
    /**
     * @brief Main object associated with this customization instance.
     *
     * Stored for accessing its properties and handling related logic.
     */
    const UObject* MainObject;

    /**
     * @brief Cached set of root objects.
     *
     * Used for building UI elements and managing their state.
     */
    FDetailsObjectSet CachedRootObjectSet;
};
