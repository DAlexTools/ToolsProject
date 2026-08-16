using UnrealBuildTool;

public class PerformanceInspectorEditor : ModuleRules
{
	public PerformanceInspectorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"DesktopPlatform",
				"EditorStyle",
				"Engine",
				"InputCore",
				"Json",
				"JsonUtilities",
				"LevelEditor",
				"PerformanceInspectorRuntime",
				"Projects",
				"RenderCore",
				"RHI",
				"Settings",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UMG",
				"UnrealEd",
				"WorkspaceMenuStructure"
			});
	}
}
