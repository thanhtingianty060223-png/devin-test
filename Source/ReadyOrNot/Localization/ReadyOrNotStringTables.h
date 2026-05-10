#pragma once
#include "Internationalization/StringTableRegistry.h"

//TODO (Max): Remove formatting tags from here (<red> etc) and format ftext in place where required
inline void CreateActionPromptTable()
{
	LOCTABLE_NEW("ActionPromptTable", "ActionPromptTable");

	// Prompt Input types
	LOCTABLE_SETSTRING("ActionPromptTable", "PressPrompt", "Press {0} to <{1}>{2}</>");
	LOCTABLE_SETMETA("ActionPromptTable", "PressPrompt", "Context", "Used when interacting with objects, E.g. 'Press F to Open Door'. {0} is an input key, {1} is a color code that wraps the action {2}");
	LOCTABLE_SETSTRING("ActionPromptTable", "ReleasePrompt", "Release {0} to <{1}>{2}</>");
	LOCTABLE_SETMETA("ActionPromptTable", "ReleasePrompt", "Context", "Used when interacting with objects, E.g. 'Release F to Open Door'. {0} is an input key, {1} is a color code that wraps the action {2}");
	LOCTABLE_SETSTRING("ActionPromptTable", "HoldPrompt", "Hold {0} to <{1}>{2}</>");
	LOCTABLE_SETMETA("ActionPromptTable", "HoldPrompt", "Context", "Used when interacting with objects, E.g. 'Hold F to Open Door'. {0} is an input key, {1} is a color code that wraps the action {2}");
	LOCTABLE_SETSTRING("ActionPromptTable", "DoubleTapPrompt", "Double Tap {0} to <{1}>{2}</>");
	LOCTABLE_SETMETA("ActionPromptTable", "DoubleTapPrompt", "Context", "Used when interacting with objects, E.g. 'Double Tap F to Open Door'. {0} is an input key, {1} is a color code that wraps the action {2}");
	LOCTABLE_SETSTRING("ActionPromptTable", "MovePrompt", "Move {0} to <{1}>{2}</>");
	LOCTABLE_SETMETA("ActionPromptTable", "MovePrompt", "Context", "Used when interacting with objects, E.g. 'Move Left Stick to Open Door'. {0} is an input key, {1} is a color code that wraps the action {2}");
	LOCTABLE_SETSTRING("ActionPromptTable", "InvalidPrompt", "Invalid Prompt");

	// Misc
	LOCTABLE_SETSTRING("ActionPromptTable", "SelectMission", "Select Mission");
	LOCTABLE_SETSTRING("ActionPromptTable", "EquipMultitool", "Equip Multitool");
	LOCTABLE_SETSTRING("ActionPromptTable", "EquipWireCutters", "Equip Wire Cutters");
	LOCTABLE_SETSTRING("ActionPromptTable", "DisarmTrap", "Disarm Trap");
	LOCTABLE_SETSTRING("ActionPromptTable", "Disarming", "<red>Disarming...</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "OrderCivilian", "Order Civilian");
	LOCTABLE_SETSTRING("ActionPromptTable", "OrderSuspect", "Order Suspect");
	LOCTABLE_SETSTRING("ActionPromptTable", "Restrain", "Restrain");
	LOCTABLE_SETSTRING("ActionPromptTable", "Restraining", "<Red>Restraining</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "BeingArrestedBy", "<Red>Being Arrested by {0}</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "CarryArrested", "Carry Arrested");
	LOCTABLE_SETSTRING("ActionPromptTable", "CarryingArrested", "<red>Carrying Arrested</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "BeingCarriedBy", "<red>Being Carried by {0}</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "SecureEvidence", "Secure Evidence");
	LOCTABLE_SETSTRING("ActionPromptTable", "SecuringEvidence", "<red>Securing Evidence...</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "SecureEvidenceName", "Secure Evidence ({0})");
	LOCTABLE_SETSTRING("ActionPromptTable", "SecureName", "Secure {0}");
	LOCTABLE_SETSTRING("ActionPromptTable", "ClearEvidence", "Clear Evidence");
	LOCTABLE_SETSTRING("ActionPromptTable", "ClearEvidenceWithName", "Clear Evidence ({0})");
	LOCTABLE_SETSTRING("ActionPromptTable", "PickupWithName", "Pickup {0}");
	LOCTABLE_SETSTRING("ActionPromptTable", "PickupItem", "Pickup Item");
	LOCTABLE_SETSTRING("ActionPromptTable", "InsertCoins", "Insert Coins");
	LOCTABLE_SETSTRING("ActionPromptTable", "InsertCoin", "Insert Coin");
	LOCTABLE_SETSTRING("ActionPromptTable", "PlayAsRightPaddle", "Play as Right Paddle");
	LOCTABLE_SETSTRING("ActionPromptTable", "PlayAsLeftPaddle", "Play as Left Paddle");
	LOCTABLE_SETSTRING("ActionPromptTable", "DefuseBomb", "Defuse Bomb");
	LOCTABLE_SETSTRING("ActionPromptTable", "DefusingBomb", "<red>Defusing...</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "ReportArrest", "Report Arrest");
	LOCTABLE_SETSTRING("ActionPromptTable", "ReportDead", "Report Dead");
	LOCTABLE_SETSTRING("ActionPromptTable", "ReportDeadBodies", "Report Dead Bodies");
	LOCTABLE_SETSTRING("ActionPromptTable", "ReportDOA", "Report DOA");
	LOCTABLE_SETSTRING("ActionPromptTable", "ReportIncapacitated", "Report Incapacitated");
	LOCTABLE_SETSTRING("ActionPromptTable", "ReportIncapacitatedBodies", "Report Incapacitated Bodies");

	LOCTABLE_SETSTRING("ActionPromptTable", "TurnOn", "Turn On");
	LOCTABLE_SETSTRING("ActionPromptTable", "TurnOff", "Turn Off");
	LOCTABLE_SETSTRING("ActionPromptTable", "RefillAmmo", "Refill Ammo");
	LOCTABLE_SETSTRING("ActionPromptTable", "PickupMagazine", "Pickup Magazine");
	
	// Door specific
	LOCTABLE_SETSTRING("ActionPromptTable", "OpenDoor", "Open Door");
	LOCTABLE_SETMETA("ActionPromptTable", "OpenDoor", "Context", "Used as an action, E.g. 'Press F to Open Door");
	LOCTABLE_SETSTRING("ActionPromptTable", "CloseDoor", "Close Door");
	LOCTABLE_SETMETA("ActionPromptTable", "CloseDoor", "Context", "Used as an action, E.g. 'Press F to Close Door");
	LOCTABLE_SETSTRING("ActionPromptTable", "PeekDoor", "Peek Door");
	LOCTABLE_SETMETA("ActionPromptTable", "PeekDoor", "Context", "Used as an action, E.g. 'Press F to Peek Door");
	LOCTABLE_SETSTRING("ActionPromptTable", "KickDoor", "Kick Door");
	LOCTABLE_SETMETA("ActionPromptTable", "KickDoor", "Context", "Used as an action, E.g. 'Press F to Kick Door");
	LOCTABLE_SETSTRING("ActionPromptTable", "OpenBothDoors", "Open Both Doors");
	LOCTABLE_SETMETA("ActionPromptTable", "OpenBothDoors", "Context", "Used as an action, E.g. 'Press F to Open Both Doors");
	LOCTABLE_SETSTRING("ActionPromptTable", "CloseBothDoors", "Close Both Doors");
	LOCTABLE_SETMETA("ActionPromptTable", "CloseBothDoors", "Context", "Used as an action, E.g. 'Press F to Close Both Doors");
	LOCTABLE_SETSTRING("ActionPromptTable", "PeekBothDoors", "Peek Both Doors");
	LOCTABLE_SETMETA("ActionPromptTable", "PeekBothDoors", "Context", "Used as an action, E.g. 'Press F to Peek Both Doors");
	LOCTABLE_SETSTRING("ActionPromptTable", "KickBothDoors", "Kick Both Doors");
	LOCTABLE_SETMETA("ActionPromptTable", "KickBothDoors", "Context", "Used as an action, E.g. 'Press F to Kick Both Doors");
	LOCTABLE_SETSTRING("ActionPromptTable", "DoorOpen", "Door <Red>Open</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "DoorClosed", "Door <Red>Closed</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "DoorBroken", "Door <Red>Broken</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "DoorJammed", "Door <Red>Jammed</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "DoorLocked", "Door <Red>Locked</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "DoorUnlocked", "Door <Red>Unlocked</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "CannotKickWith", "Cannot <red>Kick</> with <red>{0}</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "CannotKickWithBrokenLegs", "Cannot <red>Kick</> with <red>Broken Legs</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "EquipLockpickGun", "Equip Lockpick Gun");
	LOCTABLE_SETSTRING("ActionPromptTable", "EquipLockpick", "Equip Lockpick");
	LOCTABLE_SETSTRING("ActionPromptTable", "CheckDoorLock", "Check Door Lock");
	LOCTABLE_SETSTRING("ActionPromptTable", "PickLock", "Pick Lock");
	LOCTABLE_SETSTRING("ActionPromptTable", "Unlocking", "<red>Unlocking...</>");

	LOCTABLE_SETSTRING("ActionPromptTable", "EquipOptiwand", "Equip Optiwand");
	LOCTABLE_SETSTRING("ActionPromptTable", "MirrorUnderDoor", "Mirror Under Door");
	LOCTABLE_SETSTRING("ActionPromptTable", "MirrorBlocked", "<red>Mirror Blocked</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "EquipWedge", "Equip Wedge");
	LOCTABLE_SETSTRING("ActionPromptTable", "DeployWedge", "Deploy Wedge");
	LOCTABLE_SETSTRING("ActionPromptTable", "RemoveWedge", "Remove Wedge");
	LOCTABLE_SETSTRING("ActionPromptTable", "RemovingWedge", "<red>Removing Wedge...</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "EquipC2", "Equip C2");
	LOCTABLE_SETSTRING("ActionPromptTable", "PlantC2", "Plant C2 Explosive");
	LOCTABLE_SETSTRING("ActionPromptTable", "RemoveC2", "Remove C2");
	LOCTABLE_SETSTRING("ActionPromptTable", "RemovingC2", "<Red>Removing C2...</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "EquipBreachingShotgun", "Equip Breaching Shotgun");
	LOCTABLE_SETSTRING("ActionPromptTable", "EquipBatteringRam", "Equip Battering Ram");
	
	// Exfil Specific
	LOCTABLE_SETSTRING("ActionPromptTable", "ExfilWithNoMembers", "Exfiltrate Mission With 0 Squad Members");
	LOCTABLE_SETSTRING("ActionPromptTable", "ExfilNotSquadLeader", "<red>Cannot Exfiltrate - Not Squad Leader</>");
	LOCTABLE_SETSTRING("ActionPromptTable", "ExfilWithXMembers", "Exfiltrate Mission With {0} Squad Members");
	LOCTABLE_SETSTRING("ActionPromptTable", "EndMission", "End Mission");
}

inline void CreateTooltipStringTable()
{
	LOCTABLE_NEW("TooltipTable", "TooltipTable");

	// Station
	LOCTABLE_SETSTRING("TooltipTable", "MissionSelect", "Mission Select");
	LOCTABLE_SETSTRING("TooltipTable", "MissionSelectDescription", "Approach the Briefing desk to choose your next assignment");
	
	LOCTABLE_SETSTRING("TooltipTable", "ModifyLoadout", "Modify Loadout");
	LOCTABLE_SETSTRING("TooltipTable", "ModifyLoadoutSWATUnitRoom", "Modify your loadout and equipment here in the SWAT Unit room as well as in the Armory");
	LOCTABLE_SETSTRING("TooltipTable", "ModifyLoadoutArmory", "Modify your loadout and equipment here in the Armory, to test in the Shooting Range");
	
	LOCTABLE_SETSTRING("TooltipTable", "Customisation", "Customisation");
	LOCTABLE_SETSTRING("TooltipTable", "CustomisationDescription", "Customise your character's appearance in the SWAT Unit Lockers");

	LOCTABLE_SETSTRING("TooltipTable", "EvidenceLocker", "Evidence Locker");
	LOCTABLE_SETSTRING("TooltipTable", "EvidenceLockerDescription", "View a collection of case evidence, gathered from your previous missions.");	
}

inline void CreateScoringStringTable()
{
	LOCTABLE_NEW("ScoringTable", "ScoringTable");
	
	LOCTABLE_SETSTRING("ScoringTable", "EvidenceSecured", "Evidence Secured");
	LOCTABLE_SETSTRING("ScoringTable", "EvidenceReported", "Evidence Reported");
	LOCTABLE_SETSTRING("ScoringTable", "EvidenceSecuredWithName", "Evidence Secured ({0})");
	LOCTABLE_SETSTRING("ScoringTable", "SuspectReported", "Suspect Reported");
	LOCTABLE_SETSTRING("ScoringTable", "SuspectsSecured", "Suspects Secured");
	LOCTABLE_SETSTRING("ScoringTable", "CivilianReported", "Civilian Reported");
	LOCTABLE_SETSTRING("ScoringTable", "CiviliansSecured", "Civilians Secured");
	LOCTABLE_SETSTRING("ScoringTable", "CivilianKilled", "Civilian Killed");
	LOCTABLE_SETSTRING("ScoringTable", "DownedOfficerReported", "Downed Officer Reported");
	LOCTABLE_SETSTRING("ScoringTable", "DownedOfficersReported", "Downed Officers Reported");
	LOCTABLE_SETSTRING("ScoringTable", "IncapacitatedBodyReported", "Incapacitated Body Reported");
	LOCTABLE_SETSTRING("ScoringTable", "IncapacitatedBodiesReported", "Incapacitated Bodies Reported");
	LOCTABLE_SETSTRING("ScoringTable", "DeadBodyReported", "Dead Body Reported");
	LOCTABLE_SETSTRING("ScoringTable", "DeadBodiesReported", "Dead Bodies Reported");
	LOCTABLE_SETSTRING("ScoringTable", "NoOfficersDead", "No Officers Dead");
	LOCTABLE_SETSTRING("ScoringTable", "TrapsDisarmed", "Traps Disarmed");
	LOCTABLE_SETSTRING("ScoringTable", "ExfiltratedMission", "ExfiltratedMission");
	LOCTABLE_SETSTRING("ScoringTable", "ObjectiveComplete", "Objective Complete");
	LOCTABLE_SETSTRING("ScoringTable", "ObjectiveCompleteWithName", "Objective Complete ({0})");
	LOCTABLE_SETSTRING("ScoringTable", "ObjectiveUnlocked", "{0} Objective Unlocked");
	LOCTABLE_SETSTRING("ScoringTable", "ObjectiveFailed", "Objective Failed");
	LOCTABLE_SETSTRING("ScoringTable", "ObjectiveFailedWithName", "Objective Failed ({0})");
	LOCTABLE_SETSTRING("ScoringTable", "MissionObjectives", "Mission Objectives");
	LOCTABLE_SETSTRING("ScoringTable", "SoftObjectives", "Soft Objectives");
	
	LOCTABLE_SETSTRING("ScoringTable", "TrapDisarmed", "Trap Disarmed");
	LOCTABLE_SETSTRING("ScoringTable", "TrapTriggered", "Trap Triggered");
	LOCTABLE_SETSTRING("ScoringTable", "TrapTriggeredWithName", "Trap Triggered [{0}]");
	
	LOCTABLE_SETSTRING("ScoringTable", "UnauthorizedUseofForce", "Unauthorized Use of Force");
	LOCTABLE_SETSTRING("ScoringTable", "UnauthorizedUseofDeadlyForce", "Unauthorized Use of Deadly Force");
	LOCTABLE_SETSTRING("ScoringTable", "FriendlyFire", "Friendly Fire");
	LOCTABLE_SETSTRING("ScoringTable", "FriendlyTeamKill", "Friendly Team Kill");
	LOCTABLE_SETSTRING("ScoringTable", "FailedToReportADownedOfficer", "Failed to Report a Downed Officer");
	LOCTABLE_SETSTRING("ScoringTable", "KilledAnIncapacitatedHuman", "Killed an Incapacitated Human");

	LOCTABLE_SETSTRING("ScoringTable", "Alive", "Alive");
	LOCTABLE_SETSTRING("ScoringTable", "Arrest", "Arrest");
	LOCTABLE_SETSTRING("ScoringTable", "NoInjury", "No Injury");
}

inline void CreateSwatCommandStringTable()
{
	LOCTABLE_NEW("SwatCommandTable", "SwatCommandTable");
	LOCTABLE_SETSTRING("SwatCommandTable", "None", "None");
	LOCTABLE_SETSTRING("SwatCommandTable", "Roger", "Roger");
	LOCTABLE_SETMETA("SwatCommandTable", "Roger", "Context", "Used as message received, copy that, etc");
	LOCTABLE_SETSTRING("SwatCommandTable", "Negative", "Negative");
	LOCTABLE_SETMETA("SwatCommandTable", "Negative", "Context", "Used as a response. E.g 'Can you do that?' 'Negative'");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveTo", "Move To");
	LOCTABLE_SETSTRING("SwatCommandTable", "FallIn", "Fall In");
	LOCTABLE_SETSTRING("SwatCommandTable", "Cover", "Cover");
	LOCTABLE_SETSTRING("SwatCommandTable", "Hold", "Hold");
	LOCTABLE_SETMETA("SwatCommandTable", "Hold", "Context", "Maintain position");
	LOCTABLE_SETSTRING("SwatCommandTable", "Resume", "Resume");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployFlashbang", "Deploy Flashbang");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployStinger", "Deploy Stinger");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployCSGas", "Deploy CS Gas");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployChemlight", "Deploy Chemlight");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployGrenade", "Deploy Grenade");
	LOCTABLE_SETSTRING("SwatCommandTable", "SecureEvidence", "Secure Evidence");
	LOCTABLE_SETSTRING("SwatCommandTable", "Restrain", "Restrain");
	LOCTABLE_SETSTRING("SwatCommandTable", "Report", "Report");
	LOCTABLE_SETSTRING("SwatCommandTable", "DisarmTrap", "Disarm Trap");
	LOCTABLE_SETSTRING("SwatCommandTable", "KillMe", "Kill Me");
	LOCTABLE_SETSTRING("SwatCommandTable", "StackUp", "Stack Up");
	LOCTABLE_SETSTRING("SwatCommandTable", "StackUpLeft", "Stack Up Left");
	LOCTABLE_SETSTRING("SwatCommandTable", "StackUpRight", "Stack Up Right");
	LOCTABLE_SETSTRING("SwatCommandTable", "StackUpSplit", "Stack Up Split");
	LOCTABLE_SETSTRING("SwatCommandTable", "Split", "Split");
	LOCTABLE_SETSTRING("SwatCommandTable", "Left", "Left");
	LOCTABLE_SETSTRING("SwatCommandTable", "Right", "Right");
	LOCTABLE_SETSTRING("SwatCommandTable", "Auto", "Auto");
	LOCTABLE_SETSTRING("SwatCommandTable", "PickLock", "Pick Lock");
	LOCTABLE_SETSTRING("SwatCommandTable", "RemoveWedge", "Remove Wedge");
	LOCTABLE_SETSTRING("SwatCommandTable", "MirrorUnderDoor", "Mirror Under Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "WedgeDoor", "Wedge Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "MirrorForTraps", "Mirror For Traps");
	LOCTABLE_SETSTRING("SwatCommandTable", "OpenDoor", "Open Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "CloseDoor", "Close Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveAndClear", "Move and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveFlashbangAndClear", "Move, Flashbang and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveStingAndClear", "Move, Sting and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveGasAndClear", "Move, Gas and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveLauncherAndClear", "Move, Launcher and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveLeaderAndClear", "Move, Leader and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "OpenAndClear", "Open and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "OpenFlashbangAndClear", "Open, Flashbang and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "OpenStingAndClear", "Open, Sting and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "OpenGasAndClear", "Open, Gas and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "OpenLauncherAndClear", "Open, Launcher and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "OpenLeaderAndClear", "Open, Leader and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "KickAndClear", "Kick and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "KickFlashbangAndClear", "Kick, Flashbang and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "KickStingAndClear", "Kick, Sting and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "KickGasAndClear", "Kick, Gas and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "KickLauncherAndClear", "Kick, Launcher and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "KickLeaderAndClear", "Kick, Leader and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "ShotgunAndClear", "Shotgun and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "ShotgunFlashbangAndClear", "Shotgun, Flashbang and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "ShotgunStingAndClear", "Shotgun, Sting and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "ShotgunGasAndClear", "Shotgun, Gas and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "ShotgunLauncherAndClear", "Shotgun, Launcher and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "ShotgunLeaderAndClear", "Shotgun, Leader and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2AndClear", "C2 and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2FlashbangAndClear", "C2, Flashbang and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2StingAndClear", "C2, Sting and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2GasAndClear", "C2, Gas and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2LauncherAndClear", "C2, Launcher and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2LeaderAndClear", "C2, Leader and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "LeaderAndClear", "Leader and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "LeaderFlashbangAndClear", "Leader, Flashbang and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "LeaderStingAndClear", "Leader, Sting and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "LeaderGasAndClear", "Leader, Gas and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "LeaderLauncherAndClear", "Leader, Launcher and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "LeaderLeaderAndClear", "Leader, Leader and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "BreachTypeLeaderAndClear", "{0}, Leader and Clear");
	LOCTABLE_SETMETA("SwatCommandTable", "BreachTypeLeaderAndClear", "Context", "{0} is a type of breach, such as kick. E.g 'Kick, Leader and Clear'");
	LOCTABLE_SETSTRING("SwatCommandTable", "BreachTypeItemNameAndClear", "{0}, {1} and Clear");
	LOCTABLE_SETMETA("SwatCommandTable", "BreachTypeItemNameAndClear", "Context", "{0} is a type of breach, such as kick. {2} is an item to use, such as gas, or it could be leader, meaning the leader will throw an item. E.g 'Kick, Gas and Clear'");
	LOCTABLE_SETSTRING("SwatCommandTable", "BreachTypeAndClear", "{0} and Clear");
	LOCTABLE_SETMETA("SwatCommandTable", "BreachTypeLeaderAndClear", "Context", "{0} is a type of breach, such as Move. E.g 'Move and Clear'");
	LOCTABLE_SETSTRING("SwatCommandTable", "Execute", "Execute");
	LOCTABLE_SETMETA("SwatCommandTable", "Execute", "Context", "Perform Action, 'Execute the commands'");
	LOCTABLE_SETSTRING("SwatCommandTable", "Cancel", "Cancel");
	LOCTABLE_SETSTRING("SwatCommandTable", "Fall In Single File", "Fall In Single File");
	LOCTABLE_SETSTRING("SwatCommandTable", "Fall In Double File", "Fall In Double File");
	LOCTABLE_SETSTRING("SwatCommandTable", "Fall In Diamond", "Fall In Diamond");
	LOCTABLE_SETSTRING("SwatCommandTable", "Fall In Wedge", "Fall In Wedge");
	LOCTABLE_SETSTRING("SwatCommandTable", "Slide", "Slide");
	LOCTABLE_SETSTRING("SwatCommandTable", "PIE", "PIE");
	LOCTABLE_SETMETA("SwatCommandTable", "PIE", "Context", "Method for clearing room in QCB");
	LOCTABLE_SETSTRING("SwatCommandTable", "SlideBy", "Slide By");
	LOCTABLE_SETSTRING("SwatCommandTable", "Peek", "Peek");
	LOCTABLE_SETSTRING("SwatCommandTable", "CenterCheck", "Center Check");
	LOCTABLE_SETSTRING("SwatCommandTable", "Scan", "Scan");
	LOCTABLE_SETSTRING("SwatCommandTable", "Scan...", "Scan...");
	LOCTABLE_SETSTRING("SwatCommandTable", "SearchAndSecure", "Search And Secure");
	LOCTABLE_SETSTRING("SwatCommandTable", "Search Room", "Search Room");
	LOCTABLE_SETSTRING("SwatCommandTable", "RamAndClear", "Ram and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "RamFlashbangAndClear", "Ram, Flashbang and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "RamStingAndClear", "Ram, Sting and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "RamGasAndClear", "Ram, Gas and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "RamLauncherAndClear", "Ram, Launcher and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "RamLeaderAndClear", "Ram, Leader and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "Alpha", "Alpha");
	LOCTABLE_SETMETA("SwatCommandTable", "Alpha", "Context", "Phonetic alphabet, used as a team member codename");
	LOCTABLE_SETSTRING("SwatCommandTable", "Bravo", "Bravo");
	LOCTABLE_SETMETA("SwatCommandTable", "Bravo", "Context", "Phonetic alphabet, used as a team member codename");
	LOCTABLE_SETSTRING("SwatCommandTable", "Charlie", "Charlie");
	LOCTABLE_SETMETA("SwatCommandTable", "Charlie", "Context", "Phonetic alphabet, used as a team member codename");
	LOCTABLE_SETSTRING("SwatCommandTable", "Delta", "Delta");
	LOCTABLE_SETMETA("SwatCommandTable", "Delta", "Context", "Phonetic alphabet, used as a team member codename");
	LOCTABLE_SETSTRING("SwatCommandTable", "SwapWithAlpha", "Swap With Alpha");
	LOCTABLE_SETSTRING("SwatCommandTable", "SwapWithBravo", "Swap With Bravo");
	LOCTABLE_SETSTRING("SwatCommandTable", "SwapWithCharlie", "Swap With Charlie");
	LOCTABLE_SETSTRING("SwatCommandTable", "SwapWithDelta", "Swap With Delta");
	LOCTABLE_SETSTRING("SwatCommandTable", "SwapWithName", "Swap With {0}");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToAlpha", "Move To Alpha");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToAlpha", "Move To Alpha");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToCharlie", "Move To Charlie");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToDelta", "Move To Delta");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToAlphaOtherSide", "Alpha  (Other Side)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToBravoOtherSide", "Bravo  (Other Side)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToCharlieOtherSide", "Charlie  (Other Side)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToDeltaOtherSide", "Delta  (Other Side)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToAlphaLeft", "Alpha  (Left)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToBravoLeft", "Bravo  (Left)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToCharlieLeft", "Charlie  (Left)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToDeltaLeft", "Delta  (Left)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToAlphaRight", "Alpha  (Right)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToBravoRight", "Bravo  (Right)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToCharlieRight", "Charlie  (Right)");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToDeltaRight", "Delta  (Right)");
	LOCTABLE_SETSTRING("SwatCommandTable", "SwapWith...", "Swap With...");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveHere", "Move Here");
	LOCTABLE_SETSTRING("SwatCommandTable", "MyPosition", "My Position");
	LOCTABLE_SETSTRING("SwatCommandTable", "Stop", "Stop");
	LOCTABLE_SETSTRING("SwatCommandTable", "MoveToExit", "Move To Exit");
	LOCTABLE_SETSTRING("SwatCommandTable", "TurnAround", "Turn Around");
	LOCTABLE_SETSTRING("SwatCommandTable", "Here", "Here");
	LOCTABLE_SETSTRING("SwatCommandTable", "HereThenBack", "Here Then Back");
	LOCTABLE_SETSTRING("SwatCommandTable", "FocusHere", "Focus Here");
	LOCTABLE_SETSTRING("SwatCommandTable", "Focus...", "Focus...");
	LOCTABLE_SETSTRING("SwatCommandTable", "FocusDoor", "Focus Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "FocusTarget", "Focus Target");
	LOCTABLE_SETSTRING("SwatCommandTable", "Target", "Target");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployShield", "Deploy Shield");
	LOCTABLE_SETSTRING("SwatCommandTable", "HolsterShield", "Holster Shield");
	LOCTABLE_SETSTRING("SwatCommandTable", "Deploy", "Deploy");
	LOCTABLE_SETSTRING("SwatCommandTable", "Deploy...", "Deploy...");
	LOCTABLE_SETSTRING("SwatCommandTable", "ConfirmOrderRequest", "Confirm Order Request");
	LOCTABLE_SETSTRING("SwatCommandTable", "Open", "Open");
	LOCTABLE_SETSTRING("SwatCommandTable", "Move", "Move");
	LOCTABLE_SETSTRING("SwatCommandTable", "Kick", "Kick");
	LOCTABLE_SETSTRING("SwatCommandTable", "Shotgun", "Shotgun");
	LOCTABLE_SETSTRING("SwatCommandTable", "Ram", "Ram");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2", "C2");
	LOCTABLE_SETSTRING("SwatCommandTable", "Leader", "Leader");
	LOCTABLE_SETSTRING("SwatCommandTable", "Breach", "Breach");
	LOCTABLE_SETSTRING("SwatCommandTable", "Open...", "Open...");
	LOCTABLE_SETSTRING("SwatCommandTable", "Move...", "Move...");
	LOCTABLE_SETSTRING("SwatCommandTable", "Kick...", "Kick...");
	LOCTABLE_SETSTRING("SwatCommandTable", "Shotgun...", "Shotgun...");
	LOCTABLE_SETSTRING("SwatCommandTable", "Ram...", "Ram...");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2...", "C2...");
	LOCTABLE_SETSTRING("SwatCommandTable", "Leader...", "Leader...");
	LOCTABLE_SETSTRING("SwatCommandTable", "Breach...", "Breach...");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployTaser", "Deploy Taser");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployPepperspray", "Deploy Pepperspray");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployPepperball", "Deploy Pepperball");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployBeanbag", "Deploy Beanbag");
	LOCTABLE_SETSTRING("SwatCommandTable", "MeleeTarget", "Melee Target");
	LOCTABLE_SETSTRING("SwatCommandTable", "SearchRoom", "Search Room");
	LOCTABLE_SETSTRING("SwatCommandTable", "SearchArea", "Search Area");
	LOCTABLE_SETSTRING("SwatCommandTable", "FocusMyPosition", "Focus My Position");
	LOCTABLE_SETSTRING("SwatCommandTable", "MyPosition", "My Position");
	LOCTABLE_SETSTRING("SwatCommandTable", "Unfocus", "Unfocus");
	LOCTABLE_SETSTRING("SwatCommandTable", "Activity", "Activity");
	LOCTABLE_SETSTRING("SwatCommandTable", "Reposition", "Reposition");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployItem", "Deploy Item");
	LOCTABLE_SETSTRING("SwatCommandTable", "DeployItemName", "Deploy {0}");
	LOCTABLE_SETSTRING("SwatCommandTable", "BreachingDoor", "Breaching Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "KickingDoor", "Kicking Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "C2Door", "C2 Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "RammingDoor", "Ramming Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "Launch40mmThroughDoor", "Launch 40mm Through Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "ThrowItemThroughDoor", "Throw Item Through Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "ThrowGrenade", "Throw Grenade");
	LOCTABLE_SETSTRING("SwatCommandTable", "CustomDoorBreach", "Custom Door Breach");
	LOCTABLE_SETSTRING("SwatCommandTable", "DoorInteraction", "Door Interaction");
	LOCTABLE_SETSTRING("SwatCommandTable", "ReportTarget", "Report Target");
	LOCTABLE_SETSTRING("SwatCommandTable", "BreachAndClear", "Breach and Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "CommitSuicide", "Commit Suicide");
	LOCTABLE_SETSTRING("SwatCommandTable", "EngageTarget", "Engage Target");
	LOCTABLE_SETSTRING("SwatCommandTable", "EngageWithName", "Engage With {0}");
	LOCTABLE_SETSTRING("SwatCommandTable", "PickUpTarget", "PickUpTarget");
	LOCTABLE_SETSTRING("SwatCommandTable", "PickupItem", "Pickup Item");
	LOCTABLE_SETSTRING("SwatCommandTable", "PlayDead", "Play Dead");
	LOCTABLE_SETSTRING("SwatCommandTable", "Reloading", "Reloading");
	LOCTABLE_SETSTRING("SwatCommandTable", "SearchLandmark", "Search Landmark");
	LOCTABLE_SETSTRING("SwatCommandTable", "SearchingName", "Searching {0}");
	LOCTABLE_SETSTRING("SwatCommandTable", "TakeCover", "Take Cover");
	LOCTABLE_SETSTRING("SwatCommandTable", "TakeCoverAtLandmark", "Take Cover At Landmark");
	LOCTABLE_SETSTRING("SwatCommandTable", "ToggleDoor", "Toggle Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "Clear", "Clear");
	LOCTABLE_SETSTRING("SwatCommandTable", "ClearWithFlashbang", "Clear With Flashbang");
	LOCTABLE_SETSTRING("SwatCommandTable", "ClearWithStinger", "Clear With Stinger");
	LOCTABLE_SETSTRING("SwatCommandTable", "ClearWithCSGas", "Clear With CS Gas");
	LOCTABLE_SETSTRING("SwatCommandTable", "ClearWithLauncher", "Clear With Launcher");
	LOCTABLE_SETSTRING("SwatCommandTable", "ClearWithLeader", "Clear With Leader");

	LOCTABLE_SETSTRING("SwatCommandTable", "PreparingForRestrain", "Preparing For Restrain");
	LOCTABLE_SETSTRING("SwatCommandTable", "RestrainingSuspect", "Restraining Suspect");
	LOCTABLE_SETSTRING("SwatCommandTable", "RestrainingCivilian", "Restraining Civilian");
	LOCTABLE_SETSTRING("SwatCommandTable", "Collecting", "Collecting");
	LOCTABLE_SETSTRING("SwatCommandTable", "Deploying", "Deploying");
	LOCTABLE_SETSTRING("SwatCommandTable", "RemovingWedge", "Removing Wedge");
	LOCTABLE_SETSTRING("SwatCommandTable", "WedgingDoor", "Wedging Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "WedgeRemoved", "Wedge Removed");
	LOCTABLE_SETSTRING("SwatCommandTable", "WedgeDeployed", "Wedge Deployed");
	LOCTABLE_SETSTRING("SwatCommandTable", "PreparingToRemoveWedge", "Preparing To Remove Wedge");
	LOCTABLE_SETSTRING("SwatCommandTable", "PreparingToWedge", "Preparing To Wedge");
	LOCTABLE_SETSTRING("SwatCommandTable", "CuttingWire", "Cutting Wire");
	LOCTABLE_SETSTRING("SwatCommandTable", "PreparingForDisarm", "Preparing For Disarm");
	LOCTABLE_SETSTRING("SwatCommandTable", "PreparingForKick", "Preparing For Kick");
	LOCTABLE_SETSTRING("SwatCommandTable", "PerformingKick", "Performing Kick");
	LOCTABLE_SETSTRING("SwatCommandTable", "RiggingC2", "Rigging C2");
	LOCTABLE_SETSTRING("SwatCommandTable", "DetonatingC2", "Detonating C2");
	LOCTABLE_SETSTRING("SwatCommandTable", "Ramming", "Ramming");
	LOCTABLE_SETSTRING("SwatCommandTable", "Throwing", "Throwing");
	LOCTABLE_SETSTRING("SwatCommandTable", "Ready", "Ready");
	LOCTABLE_SETSTRING("SwatCommandTable", "Returning", "Returning");
	LOCTABLE_SETSTRING("SwatCommandTable", "MovingToPosition", "Moving To Position");
	LOCTABLE_SETSTRING("SwatCommandTable", "GettingInPosition", "Getting In Position");
	LOCTABLE_SETSTRING("SwatCommandTable", "PreparingForLockPicking", "Preparing For Lock Picking");
	LOCTABLE_SETSTRING("SwatCommandTable", "LockPickingDoor", "Lock Picking Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "NoTargetsVisible", "No Targets Visible");
	LOCTABLE_SETSTRING("SwatCommandTable", "OneSuspectAndOneCivilianSpotted", "One Suspect & One Civilian Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "OneSuspectSpotted", "One Suspect Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "OneCivilianSpotted", "One Civilian Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "MultipleSuspectsSpotted", "Multiple Suspects Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "MultipleCiviliansSpotted", "Multiple Civilians Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "MultipleSuspectsAndCiviliansSpotted", "Multiple Suspects & Civilians Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "TrapSpotted", "Trap Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "TrapNameTrapSpotted", "{0} Trap Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "UnknownTrapSpotted", "Unknown Trap Spotted");
	LOCTABLE_SETSTRING("SwatCommandTable", "NoTrapVisible", "No Trap Visible");
	LOCTABLE_SETSTRING("SwatCommandTable", "Mirroring", "Mirroring");
	LOCTABLE_SETSTRING("SwatCommandTable", "MirrorViewBlocked", "Mirror View Blocked");
	LOCTABLE_SETSTRING("SwatCommandTable", "PreparingForMirror", "Preparing For Mirror");
	LOCTABLE_SETSTRING("SwatCommandTable", "SecuringEvidenceName", "Securing {0}");
	LOCTABLE_SETMETA("SwatCommandTable", "SecuringEvidenceName", "Context", "Evidence name will be inserted in {0}, E.g. 'Securing Contraband'");
	LOCTABLE_SETSTRING("SwatCommandTable", "SecuringEvidence", "Securing Evidence");
	LOCTABLE_SETSTRING("SwatCommandTable", "SecuringSuspect", "Securing Suspect");
	LOCTABLE_SETSTRING("SwatCommandTable", "SecuringCivilian", "Securing Civilian");
	LOCTABLE_SETSTRING("SwatCommandTable", "Checking", "Checking");
	LOCTABLE_SETSTRING("SwatCommandTable", "DoorCheckResult", "Door {0}");
	LOCTABLE_SETMETA("SwatCommandTable", "DoorCheckResult", "Context", "Door state will be inserted in {0}, E.g. 'Door Locked', 'Door Jammed'");
	LOCTABLE_SETSTRING("SwatCommandTable", "Breaching", "Breaching");
	LOCTABLE_SETSTRING("SwatCommandTable", "Clearing", "Clearing");
	LOCTABLE_SETSTRING("SwatCommandTable", "Stacking", "Stacking");
	LOCTABLE_SETSTRING("SwatCommandTable", "SingleFile", "Single File");
	LOCTABLE_SETSTRING("SwatCommandTable", "DoubleFile", "Double File");
	LOCTABLE_SETSTRING("SwatCommandTable", "Diamond", "Diamond");
	LOCTABLE_SETMETA("SwatCommandTable", "Diamond", "Context", "Squad formation");
	LOCTABLE_SETSTRING("SwatCommandTable", "Wedge", "Wedge");
	LOCTABLE_SETMETA("SwatCommandTable", "Wedge", "Context", "Squad formation");
	LOCTABLE_SETSTRING("SwatCommandTable", "ForcingSingleFile", "Forcing Single File");
	LOCTABLE_SETSTRING("SwatCommandTable", "DoorLocked", "Door Locked");
	LOCTABLE_SETSTRING("SwatCommandTable", "DoorUnlocked", "Door Unlocked");
	LOCTABLE_SETSTRING("SwatCommandTable", "DoorJammed", "Door Jammed");
	LOCTABLE_SETSTRING("SwatCommandTable", "DoorBlocked", "Door Blocked");
	LOCTABLE_SETSTRING("SwatCommandTable", "DoorNone", "Door None");
	LOCTABLE_SETSTRING("SwatCommandTable", "Scanning", "Scanning");
	LOCTABLE_SETSTRING("SwatCommandTable", "OpeningDoor", "Opening Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "ClosingDoor", "Closing Door");
	
	LOCTABLE_SETSTRING("SwatCommandTable", "Closet", "Closet");
	LOCTABLE_SETSTRING("SwatCommandTable", "Bed", "Bed");
	LOCTABLE_SETSTRING("SwatCommandTable", "Table", "Table");
	LOCTABLE_SETSTRING("SwatCommandTable", "Corner", "Corner");
	LOCTABLE_SETSTRING("SwatCommandTable", "Door", "Door");
	LOCTABLE_SETSTRING("SwatCommandTable", "Doorway", "Doorway");
	LOCTABLE_SETSTRING("SwatCommandTable", "Door...", "Door...");
	LOCTABLE_SETSTRING("SwatCommandTable", "Doorway...", "Doorway...");
	LOCTABLE_SETSTRING("SwatCommandTable", "OtherDoor...", "Other Door...");
	LOCTABLE_SETSTRING("SwatCommandTable", "OtherDoorway...", "Other Doorway...");
	
	LOCTABLE_SETSTRING("SwatCommandTable", "AwaitingOrder", "Awaiting Order");
	LOCTABLE_SETSTRING("SwatCommandTable", "InProgress", "In Progress");
	LOCTABLE_SETSTRING("SwatCommandTable", "QueuedCommand", "[Queued] {0}");
	LOCTABLE_SETSTRING("SwatCommandTable", "Queuing...", "Queuing...");
	LOCTABLE_SETSTRING("SwatCommandTable", "QueueCommand", "Queue Command");
	LOCTABLE_SETSTRING("SwatCommandTable", "Acknowledge...", "Acknowledge...");
	LOCTABLE_SETSTRING("SwatCommandTable", "StatusInProgress", "{0} | In Progress");
	LOCTABLE_SETMETA("SwatCommandTable", "StatusInProgress", "Context", "Command inserted at {0}, E.g. 'Move and Clear | In Progress'");
	LOCTABLE_SETSTRING("SwatCommandTable", "StatusComplete", "{0} | Complete");
	LOCTABLE_SETMETA("SwatCommandTable", "StatusComplete", "Context", "Command inserted at {0}, E.g. 'Move and Clear | Complete'");

	LOCTABLE_SETSTRING("SwatCommandTable", "SuspectCombat", "Suspect Combat");
	LOCTABLE_SETSTRING("SwatCommandTable", "SwatCombat", "Swat Combat");
	LOCTABLE_SETSTRING("SwatCommandTable", "TargetNextCivilian", "Target Next Civilian");
	LOCTABLE_SETSTRING("SwatCommandTable", "TraverseHole", "Traverse Hole");

	LOCTABLE_SETSTRING("SwatCommandTable", "Healthy", "Healthy");
	LOCTABLE_SETSTRING("SwatCommandTable", "Injured", "Injured");
	LOCTABLE_SETSTRING("SwatCommandTable", "Downed", "Downed");
	LOCTABLE_SETSTRING("SwatCommandTable", "Incapacitated", "Incapacitated");
	LOCTABLE_SETSTRING("SwatCommandTable", "Dead", "Dead");
	LOCTABLE_SETSTRING("SwatCommandTable", "Arrested", "Arrested");
	LOCTABLE_SETSTRING("SwatCommandTable", "Unavailable", "Unavailable");

	LOCTABLE_SETSTRING("SwatCommandTable", "Judge", "Judge");
	LOCTABLE_SETMETA("SwatCommandTable", "Judge", "Context", "Name of the player in commander mode");
	LOCTABLE_SETSTRING("SwatCommandTable", "Unknown", "Unknown");

	LOCTABLE_SETSTRING("SwatCommandTable", "CommandWheelPressToQueue",  "queue command");
	LOCTABLE_SETSTRING("SwatCommandTable", "CommandWheelPressToStopQueueing", "stop queueing");
}

inline void CreateLoadoutInfoStringTable()
{
	LOCTABLE_NEW("LoadoutInfoTable", "LoadoutEffectsTable");
	
	// Positive
	LOCTABLE_SETSTRING("LoadoutInfoTable", "ReducedVerticalRecoil", "Reduced vertical recoil");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "ReducedHorizontalRecoil", "Reduced horizontal recoil");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "IncreasedMuzzleVelocity", "Increased muzzle velocity");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "IncreasedADSSpeed", "Increased ADS Speed");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "IncreasedAccuracy", "Increased Accuracy");

	LOCTABLE_SETSTRING("LoadoutInfoTable", "Suppressed", "Suppressed");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "HidesMuzzleFlash", "Hides muzzle flash");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "ReducesMuzzleFlash", "Reduces muzzle flash");
	
	// Negative
	LOCTABLE_SETSTRING("LoadoutInfoTable", "IncreasedVerticalRecoil", "Increased vertical recoil");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "IncreasedHorizontalRecoil", "Increased horizontal recoil");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "ReducedMuzzleVelocity", "Reduced muzzle velocity");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "ReducedADSSpeed", "Reduced ADS Speed");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "ReducedAccuracy", "Reduced Accuracy");

	//Attachments
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Null", "Null");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Optics", "Optics");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Muzzle", "Muzzle");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Underbarrel", "Underbarrel");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Overbarrel", "Overbarrel");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Stock", "Stock");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Grip", "Grip");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Illuminators", "Illuminators");
	LOCTABLE_SETSTRING("LoadoutInfoTable", "Ammunition", "Ammunition");
	
}

inline void CreateTextModifierTable()
{
	LOCTABLE_NEW("TextModifierTable", "TextModifierTable");
	LOCTABLE_SETSTRING("TextModifierTable", "ColorText", "<{0}>{1}</>");
	LOCTABLE_SETMETA("TextModifierTable", "ColorText", "Comment", "Not required for localization");
	
}

inline void LoadReadyOrNotStringTables()
{
	if (!FStringTableRegistry::Get().FindStringTable("ActionPromptTable"))
	{
		CreateActionPromptTable();
	}
	if (!FStringTableRegistry::Get().FindStringTable("TooltipTable"))
	{
		CreateTooltipStringTable();
	}
	if (!FStringTableRegistry::Get().FindStringTable("ScoringTable"))
	{
		CreateScoringStringTable();
	}
	if (!FStringTableRegistry::Get().FindStringTable("TextModifierTable"))
	{
		CreateTextModifierTable();
	}
	if (!FStringTableRegistry::Get().FindStringTable("SwatCommandTable"))
	{
		CreateSwatCommandStringTable();
	}
	if (!FStringTableRegistry::Get().FindStringTable("LoadoutInfoTable"))
	{
		CreateLoadoutInfoStringTable();
	}
}