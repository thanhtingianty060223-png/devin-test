// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ReadyOrNotTarget : TargetRules
{
	public ReadyOrNotTarget(TargetInfo Target) : base (Target)
	{
		Type = TargetType.Game;
        ExtraModuleNames.Add("ReadyOrNot");
        bUseLoggingInShipping = false;
        DefaultBuildSettings = BuildSettingsVersion.V2;
	}
}
