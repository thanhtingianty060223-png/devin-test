using UnrealBuildTool;

public class ReadyOrNotEditorModule : ModuleRules
{
    public ReadyOrNotEditorModule(ReadOnlyTargetRules Target) : base (Target)
    {
        PrivatePCHHeaderFile = "Public/ReadyOrNotEditorModule.h";
        PublicIncludePaths.AddRange(new[] { "ReadyOrNotEditorModule/Public" });

        PrivateIncludePaths.AddRange(new[] { "ReadyOrNotEditorModule/Private" });

        PublicDependencyModuleNames.AddRange(new[] 
        { 
            "Core", 
            "CoreUObject", 
            "Engine", 
            "AudioEditor", 
            "EditorScriptingUtilities", 
            "UnrealEd", 
            "Blutility", 
            "UMG", 
            "StructViewer", 
            "Slate", 
            "SlateCore",
            "Core",
            "ApplicationCore"
        });
        
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDependencyModuleNames.Add("ReadyOrNot");
            PrivateDependencyModuleNames.Add("ReadyOrNot");
        }

        if ((Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
        {
            PrivateDependencyModuleNames.Add("GameplayDebugger");
            PrivateDependencyModuleNames.Add("ReadyOrNot");
            PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
        }
        else
        {
            PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
        }
    }
}
