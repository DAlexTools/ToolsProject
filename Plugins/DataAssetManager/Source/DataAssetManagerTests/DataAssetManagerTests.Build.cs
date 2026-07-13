// Copyright (c) 2026 DimAlek. All Rights Reserved.

using UnrealBuildTool;

public class DataAssetManagerTests : ModuleRules
{
	public DataAssetManagerTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"Slate",
				"SlateCore",
				"AssetRegistry",
				"AssetTools",
				"EditorScriptingUtilities",
				"DeveloperSettings",
				"PropertyEditor",
				"SourceControl",
				"DataAssetManager"
			});
	}
}
