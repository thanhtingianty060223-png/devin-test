// Copyright 1998-2014 Epic ReadyOrNots, Inc. All Rights Reserved.
 
using UnrealBuildTool;
 
public class ReadyOrNotServerTarget : TargetRules
{
    public ReadyOrNotServerTarget(TargetInfo Target) : base (Target)
    {
        Type = TargetType.Server;
        ExtraModuleNames.Add("ReadyOrNot");
        bUseLoggingInShipping = true;
        DefaultBuildSettings = BuildSettingsVersion.V2;
    }
}

