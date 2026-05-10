// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
using System;
using UnrealBuildTool;
using System.IO;

public class ReadyOrNot : ModuleRules
{
	public ReadyOrNot(ReadOnlyTargetRules Target) : base (Target)
    {
        UnrealTargetPlatform parsedPlatform;
        var targetingXSX = UnrealTargetPlatform.TryParse("XSX", out parsedPlatform) && Target.Platform == parsedPlatform;
        var targetingXboxOneGDK = UnrealTargetPlatform.TryParse("XboxOneGDK", out parsedPlatform) && Target.Platform == parsedPlatform;
        var targetingXbox = targetingXboxOneGDK || targetingXSX;
        var targetingPS4 = UnrealTargetPlatform.TryParse("PS4", out parsedPlatform) && Target.Platform == parsedPlatform;
        var targetingPS5 = UnrealTargetPlatform.TryParse("PS5", out parsedPlatform) && Target.Platform == parsedPlatform;
        var targetingPlayStation = targetingPS4 || targetingPS5;
        var targetingConsole = targetingXbox || targetingPlayStation;

        bAllowConfidentialPlatformDefines = true;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "ReadyOrNot.h";
        PublicIncludePaths.Add("ReadyOrNot");
        PrivateIncludePaths.Add("ReadyOrNot");
        PublicIncludePaths.Add("../Plugins/InputRemapping/Source/InputRemapping/Private");

        // Public modules.

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "CableComponent",
            "RHI",
            "HTTP",
            "HttpNetworkReplayStreaming",
            "Json",
            "JsonUtilities",
            "OnlineSubsystemNull",
            "OnlineSubsystemUtils",
            "AIModule",
            //"ApexDestruction",
            "AdvancedSessions",
            //"AdvancedSteamSessions",
            "CustomAnimNode",
            "NavigationSystem",
            "FMODStudio",
            "GameplayTasks",
            "OnlineSubsystem",
            "RHI",
            "RenderCore",
            "UMG",
            "LevelSequence",
            "MovieScene",
            "Debug",
            //"Zeuzsdk", 
            //"ZeuzModule",
            "ProceduralMeshComponent",
            "DynamicCoverSystem",
            "GameplayCameras",
            "PhysicsCore",
            //"DLSSBlueprint",
            //"StreamlineBlueprint",
            "AudioMixer",
            "PakFile",
            "SkinnedDecalComponent",
            "CinematicCamera",
            // "PhysX",
            "ObjectPooler",
            "CommonUI",
            "CommonInput",
            "MeshDescription",
            "StaticMeshDescription",
            "Navmesh",
            "DeveloperSettings",
            "InputRemapping",
            "AMRagdoll",
            "GameplayTags",
            "Projects",
            "ApplicationCore",
            "AimAssistSystem",
            "NetCore",
        });

        if (!targetingConsole)
        {
            PublicDependencyModuleNames.AddRange(new[]
            {
                "OnlineSubsystemSteam",
                "Steamworks",
                "StatsIntegration",
                //"VivoxCore",
                "Reflex",
                //"VICODynamicsPlugin",
                //"WinDualShock",
                //"WinDualSense"
            });

            PublicDefinitions.AddRange(new[]
            {
                // "WITH_STEAM",
                // "WITH_MODIO",
                // "WITH_VIVOX",
                "WITH_REFLEX",
                //"WITH_VICODYNAMICS",
                "_CRT_SECURE_NO_WARNINGS"
            });
        }

        if (targetingXbox)
        {
            PublicDependencyModuleNames.AddRange(new[]
            {
                "OnlineSubsystemGDK",
                "OnlineSubsystemEOS",
                "OnlineSubsystemEOSPlus",
                "HairStrandsCore", // Prevent runtime error on XBoxOneGDK
                // "VivoxCore",
                //"FSR"
            });

            PublicDefinitions.AddRange(new[]
            {
                "TARGET_XBOX",
                "TARGET_CONSOLE",
				//"WITH_VIVOX"
			});
        }

        if (targetingXboxOneGDK)
        {
            PublicDefinitions.AddRange(new[]
            {
                "TARGET_XB1"
            });
        }

        if (targetingXSX)
        {
            PublicDefinitions.AddRange(new[]
            {
                "TARGET_XSX"
            });
        }

        if (targetingPS4)
        {
            PublicDefinitions.AddRange(new[]
            {
                "TARGET_PS4"
            });            
        }

        if (targetingPS5)
        {
            PublicDefinitions.AddRange(new[]
            {
                "TARGET_PS5"
            });            
        }

        if (targetingPlayStation)
        {
            if (targetingPS4)
            {
                PublicDependencyModuleNames.AddRange(new[]
                {
                    "OnlineSubsystemPS4", // probably not needed
                    "OnlineSubsystemSony",
                    "OnlineSubsystemEOS",
                    "OnlineSubsystemEOSPlus",
                    // "VivoxCore",
                    //"FSR"
                });
            }
            else
            {
                PublicDependencyModuleNames.AddRange(new[]
                {
                    "OnlineSubsystemPS5", // probably not needed
                    "OnlineSubsystemSony",
                    "OnlineSubsystemEOS",
                    "OnlineSubsystemEOSPlus",
                    // "VivoxCore",
                    //"FSR"
                });
            }

            PublicDefinitions.AddRange(new[]
            {
                "TARGET_SONY",
                "TARGET_CONSOLE",
                //"WITH_VIVOX"
            });
        }

        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PublicDependencyModuleNames.AddRange(new[]
            {
                "GameplayDebugger",
                "DevMenu",
                "AutomationController"
            });
        }

        // Private modules.
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Slate",
            "SlateCore",
            "OnlineSubsystem",
            "OnlineSubsystemNull",
            "OnlineSubsystemUtils",
            "Gauntlet",
            "AdvancedSessions",
            "AdvancedSteamSessions", 
            "CommonUI", 
            "Navmesh",
            "AimAssistSystem", 
            "InteractiveToolsFramework", "VivoxCore"
        });

        if (!targetingConsole)
        {
            PrivateDependencyModuleNames.AddRange(new[]
            {
                "OnlineSubsystemSteam",
                // enable later
                //"OnlineSubsystemEOS",
                //"OnlineSubsystemEOSPlus",
                "Steamworks",
                //"Modio",
                //"ModioUI",
                //"ModioUICore",
                //"ModioUIEditor",
                // "MSDFSupport",
                //"MSDFSupportEditor"
            });

            PrivateDefinitions.AddRange(new[]
            {
                "WITH_STEAM",
                //"WITH_MODIO",
                "_CRT_SECURE_NO_WARNINGS"
            });
        }

        if (targetingXbox)
        {
            PrivateDependencyModuleNames.AddRange(new[]
            {
                "OnlineSubsystemGDK",
                "OnlineSubsystemEOS",
                "OnlineSubsystemEOSPlus"
            });

            PrivateDefinitions.AddRange(new[]
            {
                "TARGET_XBOX",
				"TARGET_CONSOLE"
			});
        }

        if (targetingPlayStation)
        {
            if (targetingPS4)
            {
                PrivateDependencyModuleNames.AddRange(new[]
                {
                    "OnlineSubsystemPS4", // probably not needed
                    "OnlineSubsystemSony",
                    "OnlineSubsystemEOS",
                    "OnlineSubsystemEOSPlus"
                });
            }
            else
            {
                PrivateDependencyModuleNames.AddRange(new[]
                {
                    "OnlineSubsystemPS5", // probably not needed    
                    "OnlineSubsystemSony",
                    "OnlineSubsystemEOS",
                    "OnlineSubsystemEOSPlus"
                });
            }

            PrivateDefinitions.AddRange(new[]
            {
                "WITH_SONY",
                "TARGET_CONSOLE",
                "WITH_VIVOX"
            });
        }

        if (Target.Type == TargetType.Editor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
            PrivateDependencyModuleNames.Add("EditorScriptingUtilities");
        }
    }

    public string ProjectRoot
    {
        get
        {
            return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
        }
    }
}
