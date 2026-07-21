using UnrealBuildTool;
 
public class ToolsProjectEditor : ModuleRules
{
	public ToolsProjectEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "UnrealEd", "EditorScriptingUtilities", "AssetTools" });
 
		PublicIncludePaths.AddRange(new string[] {"ToolsProjectEditor/Public"});
		PrivateIncludePaths.AddRange(new string[] {"ToolsProjectEditor/Private"});


        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("EditorScriptingUtilities");
            PrivateDependencyModuleNames.Add("UnrealEd");
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "Slate",
                "SlateCore",
                "ToolMenus",
                "DataAssetManager",
                "OutlinerToolkit",
                "TextureChannelPacker"
            });
        }



    }
}
