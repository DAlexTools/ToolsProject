// Copyright 2025 DimAlek. All Rights Reserved.

using UnrealBuildTool;

public class CppTemplateGenerator : ModuleRules
{
    public CppTemplateGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "GameProjectGeneration",
            "Settings",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "UnrealEd"
        });
    }
}
