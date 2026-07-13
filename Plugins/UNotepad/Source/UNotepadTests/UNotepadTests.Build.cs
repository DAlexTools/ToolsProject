// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UNotepadTests : ModuleRules
{
	public UNotepadTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AutomationTest",
				"ApplicationCore",
				"Core",
				"CoreUObject",
				"DeveloperSettings",
				"Engine",
				"Json",
				"Slate",
				"SlateCore",
				"UNotepad",
				"UnrealEd"
			}
		);
	}
}
