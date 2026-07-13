// Copyright 2025 DimAlek. All Rights Reserved.

using UnrealBuildTool;

public class CppTemplateGeneratorTests : ModuleRules
{
	public CppTemplateGeneratorTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AutomationTest",
				"Core",
				"CoreUObject",
				"CppTemplateGenerator",
				"DeveloperSettings",
				"Engine",
				"UnrealEd"
			}
		);
	}
}
