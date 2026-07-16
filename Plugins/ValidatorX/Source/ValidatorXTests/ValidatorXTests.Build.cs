// Copyright 2025 DimAlek. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ValidatorXTests : ModuleRules
{
	public ValidatorXTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(ModuleDirectory, "..", "ValidatorX", "Private")
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AutomationTest",
				"Core",
				"CoreUObject",
				"DataValidation",
				"Engine",
				"Kismet",
				"BlueprintGraph",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"ValidatorX"
			}
		);
	}
}
