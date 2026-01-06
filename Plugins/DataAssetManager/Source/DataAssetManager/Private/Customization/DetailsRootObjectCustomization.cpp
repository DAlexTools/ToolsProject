// Fill out your copyright notice in the Description page of Project Settings.


#include "Customization/DetailsRootObjectCustomization.h"
#include "FunctionLibrary/DataAssetManagerFunctionLibrary.h"
#include "Styling/SlateIconFinder.h"
#include "DetailLayoutBuilder.h"
#include "Engine/DataAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(FDetailsRootObjectCustomizationLog, Log, All);

/* clang-format off */
TSharedPtr<SWidget> FDetailsRootObjectCustomization::CustomizeObjectHeader(const FDetailsObjectSet& InRootObjectSet, const TSharedPtr<ITableRow>& InTableRow)
{
    CachedRootObjectSet = InRootObjectSet;
    MainObject = InRootObjectSet.RootObjects.Num() > 0 ? InRootObjectSet.RootObjects[0] : nullptr;

    TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);
    HorizontalBox->AddSlot().AutoWidth()[ CreateExpandCollapseButton(InTableRow) ];
    HorizontalBox->AddSlot().AutoWidth().Padding(4.0f, 0.0f).VAlign(VAlign_Center)[ CreateHeaderDataAssetIcon() ];
    HorizontalBox->AddSlot().AutoWidth().VAlign(VAlign_Center)[ CreateMainObjectText() ];
    HorizontalBox->AddSlot().AutoWidth().Padding(4.0f, 0.0f).VAlign(VAlign_Bottom)[ CreateHeaderComboButton() ];
    HorizontalBox->AddSlot().AutoWidth().Padding(4.0f, 0.0f).VAlign(VAlign_Center)[ CreateSyncBrowserButton() ];

    TSharedRef<SBorder> Border = SNew(SBorder)
        .Padding(4)
        .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
        .BorderBackgroundColor(FLinearColor(0.2f, 0.4f, 0.8f, 1.f))
        .VAlign(VAlign_Center)
        [
            HorizontalBox
        ];

    return Border;
}

[[nodiscard]] TSharedRef<SWidget> FDetailsRootObjectCustomization::CreateExpandCollapseButton(TSharedPtr<ITableRow> InTableRow)
{
    return SNew(SButton)
        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
        .ContentPadding(FMargin(4.0f, 1.0f))
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        .OnClicked_Lambda([this, InTableRow] { return OnExpandCollapseClicked(InTableRow); })
        .Cursor(EMouseCursor::Hand)
        .ToolTipText(FText::FromString("Expand / Collapse"))
        [
            SNew(SImage)
                .Image_Lambda([this, InTableRow] { return GetExpandCollapseIcon(InTableRow); })
        ];
}

[[nodiscard]] TSharedRef<SWidget> FDetailsRootObjectCustomization::CreateSyncBrowserButton()
{
    return SNew(SButton)
        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
        .ContentPadding(FMargin(4.0f, 2.0f))
        .ToolTipText(FText::FromString("Find in Content Browser"))
        .OnClicked(this, &FDetailsRootObjectCustomization::SyncBrowserObject_OnClicked)
        [
            SNew(SImage).Image(FAppStyle::GetBrush("Icons.Search"))
        ];
}

[[nodiscard]] TSharedRef<SWidget> FDetailsRootObjectCustomization::CreateHeaderComboButton()
{
    return SNew(SComboButton)
        .ButtonStyle(FAppStyle::Get(), "SimpleButton")
        .HasDownArrow(true)
        .ContentPadding(FMargin(4.0f, 2.0f))
        .ButtonContent()
        [
            SNew(STextBlock)
            .Text(FText::FromString(""))
        ]
        .MenuContent()
        [
            SNew(SBox)
            .MinDesiredWidth(200.0f)
            [
                BuildHeaderMenu()
            ]
        ];
}

[[nodiscard]] TSharedRef<SWidget> FDetailsRootObjectCustomization::CreateHeaderDataAssetIcon()
{
    return SNew(SImage).Image(FAppStyle::GetBrush("ClassIcon.DataAsset"));
}

[[nodiscard]] TSharedRef<SWidget> FDetailsRootObjectCustomization::CreateMainObjectText()
{
    FSlateFontInfo BoldFont{ IDetailLayoutBuilder::GetDetailFontBold() };
    BoldFont.Size = 16;

    return SNew(STextBlock)
        .Text(IsValid(MainObject)
            ? FText::FromString(MainObject->GetName())
            : FText::FromString(TEXT("Invalid Object")))
        .Font(BoldFont);
}
/* clang-format on */

[[nodiscard]] TSharedRef<SWidget> FDetailsRootObjectCustomization::BuildHeaderMenu()
{
    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.BeginSection("Actions", FText::FromString("Actions"));

    MenuBuilder.AddMenuEntry(FText::FromString("Reset To Default (CDO)"),
        FText::FromString("Reset all property values to their default state"),
        FSlateIcon(FSlateIconFinder::FindIcon("IKRig.Reset.Small")),
        FUIAction(FExecuteAction::CreateSP(this, &FDetailsRootObjectCustomization::ResetToCDO)));

    MenuBuilder.AddMenuEntry(FText::FromString("Export to JSON"),
        FText::FromString("Save current DataAsset properties to JSON file"),
        FSlateIcon(FSlateIconFinder::FindIcon("ControlRig.ReExportAnimSequence.Small")),
        FUIAction(FExecuteAction::CreateSP(this, &FDetailsRootObjectCustomization::ExportToJson)));

    MenuBuilder.AddMenuEntry(FText::FromString("Import from JSON"),
        FText::FromString("Load DataAsset properties from JSON file"),
        FSlateIcon(FSlateIconFinder::FindIcon("ControlRig.ReImportFromRigSequence.Small")),
        FUIAction(FExecuteAction::CreateSP(this, &FDetailsRootObjectCustomization::ImportFromJson)));

    // TODO More Entry in the future... 

    MenuBuilder.EndSection();

    return MenuBuilder.MakeWidget();
}

void FDetailsRootObjectCustomization::ResetToCDO()
{
    for (const UObject* ConstObject : CachedRootObjectSet.RootObjects)
    {
        UObject* const Object{ const_cast<UObject*>(ConstObject) };
        if (!IsValid(Object))
        {
            continue;
        }
        UObject* const CDO{ Object->GetClass()->GetDefaultObject() };
        if (!IsValid(CDO))
        {
            continue;
        }

        // Копирование всех свойств из CDO в объект
        for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
        {
            FProperty* const Property{ *PropIt };
            if (!Property)
            {
                continue;
            }
            void* const DestPtr{ Property->ContainerPtrToValuePtr<void>(Object) };
            const void* const SrcPtr{ Property->ContainerPtrToValuePtr<void>(CDO) };
            Property->CopyCompleteValue(DestPtr, SrcPtr);
        }

        Object->PostEditChange();
        Object->MarkPackageDirty();
    }
}

void FDetailsRootObjectCustomization::ExportToJson()
{
    if (CachedRootObjectSet.RootObjects.Num() == 0)
    {
        return;
    }

    const FString FilePath{ FPaths::ProjectSavedDir() / TEXT("ExportedDataAsset.json") };
    if (DataAssetManager::SaveDataAssetToJsonFile(Cast<UDataAsset>(CachedRootObjectSet.RootObjects[0]), FilePath))
    {
        UE_LOG(FDetailsRootObjectCustomizationLog, Log, TEXT("Export successful: %s"), *FilePath);
    }
}


void FDetailsRootObjectCustomization::ImportFromJson()
{
    if (CachedRootObjectSet.RootObjects.Num() == 0)
    {
        return;
    }

    const FString FilePath{ FPaths::ProjectSavedDir() / TEXT("ExportedDataAsset.json") };
    const UObject* const ConstObject{ CachedRootObjectSet.RootObjects[0] };
    UObject* const MutableObject{ const_cast<UObject*>(ConstObject) };
    UDataAsset* const DataAsset{ Cast<UDataAsset>(MutableObject) };

    if (!DataAsset)
    {
        UE_LOG(FDetailsRootObjectCustomizationLog, Warning, TEXT("Root object is not a UDataAsset"));
        return;
    }

    if (DataAssetManager::LoadDataAssetFromJsonFile(DataAsset, FilePath))
    {
        UE_LOG(FDetailsRootObjectCustomizationLog, Log, TEXT("Import successful: %s"), *FilePath);
    }
}

[[nodiscard]] FReply FDetailsRootObjectCustomization::SyncBrowserObject_OnClicked()
{
    if (MainObject && GEditor)
    {
        TArray<UObject*> Objects;
        Objects.Add(const_cast<UObject*>(MainObject));
        GEditor->SyncBrowserToObjects(Objects);
    }
    return FReply::Handled();
}

[[nodiscard]] FReply FDetailsRootObjectCustomization::OnExpandCollapseClicked(TSharedPtr<ITableRow> InTableRow)
{
    if (InTableRow.IsValid())
    {
        InTableRow->ToggleExpansion();
    }
    return FReply::Handled();
}

[[nodiscard]] const FSlateBrush* FDetailsRootObjectCustomization::GetExpandCollapseIcon(TSharedPtr<ITableRow> InTableRow) const
{
    if (!InTableRow.IsValid())
    {
        return FSlateIconFinder::FindIcon("Icons.Plus").GetIcon();
    }

    return InTableRow->IsItemExpanded() ? FSlateIconFinder::FindIcon("Icons.Minus").GetIcon() : FSlateIconFinder::FindIcon("Icons.Plus").GetIcon();
}