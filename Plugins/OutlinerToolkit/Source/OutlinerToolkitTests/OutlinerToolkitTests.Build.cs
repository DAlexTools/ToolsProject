// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OutlinerToolkitTests : ModuleRules
{
	public OutlinerToolkitTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AutomationTest",
				"Core",
				"CoreUObject",
				"Engine",
				"OutlinerToolkit",
				"UnrealEd"
			}
		);
	}
}
