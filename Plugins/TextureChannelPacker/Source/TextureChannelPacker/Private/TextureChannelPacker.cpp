// Copyright 2025 kurorekish. All Rights Reserved.
#include "TextureChannelPacker.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "UI/STextureChannelPackerWidget.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "FTextureChannelPackerModule"

void FTextureChannelPackerModule::StartupModule()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenus* ToolMenus = UToolMenus::Get();
	UToolMenu* ToolsMenu = ToolMenus->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& Section = ToolsMenu->AddSection("TextureChannelPacker", LOCTEXT("TextureChannelPackerSection", "Texture Packing"));

	Section.AddMenuEntry(
		"PackTextures",
		LOCTEXT("PackTexturesMenuEntry", "Texture Channel Packer"),
		LOCTEXT("PackTexturesMenuEntryTooltip", "Open the Texture Channel Packer tool."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Layout"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FTextureChannelPackerModule::OpenPluginWindow)
		)
	);
}

void FTextureChannelPackerModule::ShutdownModule()
{
	UToolMenus::UnregisterOwner(this);

	if (TSharedPtr<SWindow> ExistingWindow = ToolWindow.Pin())
	{
		ExistingWindow->RequestDestroyWindow();
		ToolWindow.Reset();
	}
}

void FTextureChannelPackerModule::OpenPluginWindow()
{
	if (TSharedPtr<SWindow> ExistingWindow = ToolWindow.Pin())
	{
		ExistingWindow->BringToFront();
		return;
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("TextureChannelPackerWindowTitle", "Texture Channel Packer"))
		.ClientSize(FVector2D(960.0f, 580.0f))
		.SizingRule(ESizingRule::FixedSize)
		.SupportsMaximize(false)
		.SupportsMinimize(true)
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		[
			SNew(STextureChannelPackerWidget)
		];

	ToolWindow = Window;
	FSlateApplication::Get().AddWindow(Window);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTextureChannelPackerModule, TextureChannelPacker)
