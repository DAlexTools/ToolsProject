// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DataAssetManager : ModuleRules
{
    public DataAssetManager(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",

            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Engine",
                "AssetRegistry",
                "ApplicationCore",
                "Projects",
                "InputCore",
                "EditorFramework",
                "EditorStyle",
                "UnrealEd",
                "ToolMenus",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "PropertyEditor",
                "ContentBrowser",
                "DeveloperSettings",
                "EditorScriptingUtilities",
                "AssetManagerEditor",
                "SourceControl",
                "ClassViewer",
                "ToolWidgets",
                "StatusBar",
                "BlueprintGraph",
                "Kismet",
                "SettingsEditor",
                "PropertyEditor",
                "EditorWidgets",
                "MessageLog",
                "OutputLog",
                "Json",
                "JsonUtilities"
            }   
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
