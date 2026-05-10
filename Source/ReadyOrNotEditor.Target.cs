// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ReadyOrNotEditorTarget : TargetRules
{
	public ReadyOrNotEditorTarget(TargetInfo Target) : base (Target)
	{
		Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
	        ExtraModuleNames.Add("ReadyOrNotEditorModule");
        }

        DisablePlugins.Add("SteamVR");
	}
}
