using UnrealBuildTool;

public class ContentBrowserToolkit : ModuleRules
{
	public ContentBrowserToolkit(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"ContentBrowser",
				"ContentBrowserData",
				"Core",
				"CoreUObject",
				"ToolWidgets"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetRegistry",
				"DeveloperSettings",
				"Engine",
				"SlateCore"
			});
	}
}
