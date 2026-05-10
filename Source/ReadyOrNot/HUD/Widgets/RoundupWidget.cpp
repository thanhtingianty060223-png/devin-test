// Copyright Void Interactive, 2023

#include "RoundupWidget.h"
#include "Actors/Collectable.h"
#include "Actors/Environment/MissionPortal.h"
#include "Commander/CampaignData.h"
#include "Commander/CommanderProfile.h"
#include "Commander/MetaGameProfile.h"
#include "Commander/RosterManager.h"
#include "Data/CustomizationData.h"
#include "Engine/AssetManager.h"
#include "GameModes/LobbyGM.h"
// ##UE5UPGRADE## CommonUI
#include "CommonInputSubsystem.h"

#define LOCTEXT_NAMESPACE "RoundupWidget"

void URoundupWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	MetaGameProfile = UMetaGameProfile::GetProfile(GetWorld());
	if (!ensureAlways(MetaGameProfile))
		return;

	if (const ALobbyGM* LobbyGameMode = GetWorld()->GetAuthGameMode<ALobbyGM>())
	{
		CommanderProfile = LobbyGameMode->CommanderProfile;
		RosterManager = LobbyGameMode->RosterManager;
	}

	if (IsNewCommanderModeSave())
	{
		SetupNewCommanderModeSaveNotification();
		return;
	}

	SetupUnlockNotifications();
	SetupActionNotifications();
}

void URoundupWidget::NativeConstruct()
{
	BindInputMethodChangedDelegate();

	Super::NativeConstruct();
}

void URoundupWidget::NativeDestruct()
{
	Super::NativeDestruct();

	UnbindInputMethodChangedDelegate();
}

void URoundupWidget::BindInputMethodChangedDelegate()
{
	if (UCommonInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		OnInputMethodChanged(InputSubsystem->GetCurrentInputType());
		InputSubsystem->OnInputMethodChangedNative.AddUObject(this, &URoundupWidget::OnInputMethodChanged);
	}
}

void URoundupWidget::UnbindInputMethodChangedDelegate()
{
	if (UCommonInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		InputSubsystem->OnInputMethodChangedNative.RemoveAll(this);
	}
}

void URoundupWidget::OnInputMethodChanged(const ECommonInputType InputMethod)
{
	bUsingGamepad = InputMethod == ECommonInputType::Gamepad;

	RefreshWidget();
}

void URoundupWidget::AddUnlock(const FText& Unlock)
{
	Unlocks.Add(Unlock);

	AddRoundupUnlock(Unlock);
}

void URoundupWidget::AddAction(const FText& Action)
{
	Actions.Add(Action);

	AddRoundupAction(Action);
}

bool URoundupWidget::IsNewCommanderModeSave() const
{
	if (CommanderProfile)
	{
		if (CommanderProfile->TotalPlaytime == 0)
		{
			return true;
		}
	}

	return false;
}

void URoundupWidget::SetupNewCommanderModeSaveNotification()
{
	AddUnlock(LOCTEXT("WelcomeToRoN", "Welcome to <emphasisyellow>Ready or Not</>!\r\n\r\nBefore you start your first mission, there are a few preparations that need to be made."));

	FKey InputActionKey = UReadyOrNotFunctionLibrary::GetKeyFromInputActionName("OpenTablet", bUsingGamepad);
	FText ActionText = InputActionKey.GetDisplayName().ToUpper();
	
	FText TabletText = LOCTEXT("OpenTablet", "Open the tablet by pressing and holding <emphasisyellow>{0}</> to <emphasisyellow>view your team</>.\r\nUsing the tablet, you can <emphasisyellow>view your team's traits</>, as well as <emphasisyellow>hire new officers</> as you lose personnel.");
	FText FormattedTabletText = FText::Format(TabletText, ActionText);
	
	// Reverse order of importance as the widget will display them in reverse order
	AddAction(LOCTEXT("StartMission", "Once you are satisfied with your setup, head into the briefing room and interact with the <emphasisyellow>briefing desk</> to select your first mission."));
	AddAction(LOCTEXT("ChangeLoadout", "In order to <emphasisyellow>customize your own and your team’s gear</>, head straight through the briefing room towards your <emphasisyellow>SWAT Unit room.</>"));
	AddAction(FormattedTabletText);
}

void URoundupWidget::SetupUnlockNotifications()
{
	// Reverse order of importance as the widget will display them in reverse order
	CheckIfMissionUnlocked();

	CheckIfRosterSlotUnlocked();

	CheckIfCustomizationUnlocked();

	CheckIfEvidenceUnlocked();

	if (Unlocks.Num() == 0)
	{
		AddUnlock(FText::FromStringTable(DebriefStringTableId, "entry.Unlock.NoUnlocks"));
		UReadyOrNotFunctionLibrary::StartTimerForCallback(this, &URoundupWidget::CollapseContent, 5.0f);
	}
}

void URoundupWidget::CheckIfEvidenceUnlocked()
{
	int32 NumUnlocked = 0;
	TArray<ACollectable*> Collectables = ACollectable::GetAllCollectables();

	for (const ACollectable* Collectable : Collectables)
	{
		if (!Collectable)
			continue;

		if (!IsNewUnlock(Collectable->RequiredTags))
			continue;

		NumUnlocked++;
	}

	if (NumUnlocked == 1)
		AddUnlock(FText::FromStringTable(DebriefStringTableId, "entry.Unlock.Evidence"));
	else if (NumUnlocked > 1)
		AddUnlock(FText::FromStringTable(DebriefStringTableId, "entry.Unlock.EvidencePlural"));
}

void URoundupWidget::CheckIfCustomizationUnlocked()
{
	const UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FAssetData> DataAssets;
	AssetManager.GetPrimaryAssetDataList("CustomizationAsset", DataAssets);

	for (const FAssetData& AssetData : DataAssets)
	{
		const UCustomizationDataBase* DataAsset = Cast<UCustomizationDataBase>(AssetData.GetAsset());
		if (!ensure(DataAsset))
			continue;

		// Ignore assets set to not appear in loadout
		if (!DataAsset->bShowInLoadout)
			continue;

		// Ignore assets that have a parent (variants)
		if (IsValid(DataAsset->Parent))
			continue;

		if (!IsNewUnlock(DataAsset->RequiredTags))
			continue;

		const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Unlock.Customization");
		AddUnlock(FText::Format(Format, DataAsset->Name));
	}
}

void URoundupWidget::CheckIfMissionUnlocked()
{
	const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Unlock.Mission");

	UCampaignData* CampaignData = UBpGameplayHelperLib::GetCampaignData();
	if (!CampaignData)
		return;

	for (FString Map : CampaignData->Levels)
	{
		bool bIsUnlocked = false;
		float ScoreRequired;
		FString LockedURL;
		AMissionPortal::IsLevelUnlocked(Map, bIsUnlocked, ScoreRequired, LockedURL);

		const bool bIsCompleted = MetaGameProfile->GetCompletedLevels().Contains(Map);

		if (bIsUnlocked && !bIsCompleted)
		{
			const FLevelDataLookupTable LevelData = UBpGameplayHelperLib::GetMapDetailsFromName(Map);
			AddUnlock(FText::Format(Format, LevelData.FriendlyLevelName));

			// TODO Kobe: remove this return so we can show multiple missions unlocked
			return;
		}
	}
}

void URoundupWidget::CheckIfRosterSlotUnlocked()
{
	// Can only setup officer-related notifications in commander mode
	if (!CommanderProfile || !RosterManager)
		return;

	if (RosterManager->IsRosterSlotUnlocked())
	{
		AddUnlock(FText::FromStringTable(DebriefStringTableId, "entry.Unlock.OfficerSlot"));
	}
}

bool URoundupWidget::IsNewUnlock(const TArray<FName>& ItemRequiredTags) const
{
	const TSet<FName> NewProgressionTags = MetaGameProfile->GetTemporaryData().NewProgressionTags;

	// If no new progression tags, can't have unlocked anything
	if (NewProgressionTags.Num() == 0)
		return false;

	// If item has no tags required, assume it's unlocked by default and not new
	if (ItemRequiredTags.Num() == 0)
		return false;

	TSet<FName> OldProgressionTags = MetaGameProfile->GetProgressionTags();
	for (const FName NewTag : NewProgressionTags)
		OldProgressionTags.Remove(NewTag);

	// If all the required tags are already unlocked (minus the new tags), item not unlocked
	if (OldProgressionTags.Includes(TSet<FName>(ItemRequiredTags)))
		return false;

	for (const FName RequiredTag : ItemRequiredTags)
	{
		// If item requires a tag that we already have, skip it
		if (OldProgressionTags.Contains(RequiredTag))
			continue;

		// If item requires a tag that we haven't already unlocked and isn't new, item not unlocked
		if (!NewProgressionTags.Contains(RequiredTag))
			return false;
	}

	return true;
}

void URoundupWidget::SetupActionNotifications()
{
	// Can only setup officer notifications if we have a commander profile and roster manager (commander mode only)
	if (!CommanderProfile || !RosterManager)
	{
		HideRoundupActions();
		return;
	}

	// Reverse order of importance as the widget will display them in reverse order
	CheckIfCurrentOfficersStatusChanged();

	CheckIfPreviousOfficersStatusChanged();

	CheckIfExfiltratedFromMission();

	CheckIfEmotionalStressIncreased();

	if (Actions.Num() == 0)
	{
		HideRoundupActions();
	}
}

void URoundupWidget::CheckIfEmotionalStressIncreased()
{
	// TODO KobeT: Store state of character on exfil/mission end
	// Player died

	// Player injured

	// Player killed civilians
}

void URoundupWidget::CheckIfPreviousOfficersStatusChanged()
{
	for (const URosterCharacter* Character : RosterManager->GetPreviousCharacters())
	{
		if (!Character || Character->State == Character->PreviousState)
			continue;

		const bool bOfficerDied = Character->RemovalReason == ERosterRemovalReason::Deceased; 
		if (bOfficerDied)
		{
			const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Action.OfficerDied");
			AddAction(FText::Format(Format, Character->LastName));
		}
		
		const bool bOfficerResigned = Character->RemovalReason == ERosterRemovalReason::Overstressed; 
		if (bOfficerResigned)
		{
			const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Action.OfficerResigned");
			AddAction(FText::Format(Format, Character->LastName));
		}
	}
}

void URoundupWidget::CheckIfCurrentOfficersStatusChanged()
{
	const FExfiltrationData ExfilData = MetaGameProfile->GetTemporaryData().ExfilData;

	for (const URosterCharacter* Character : RosterManager->GetAllCharacters())
	{
		if (!Character)
			continue;

		const bool bStateChanged = Character->State != Character->PreviousState;

		const bool bTherapistIntervened = Character->bTherapistIntervened;
		if (bTherapistIntervened)
		{
			const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Action.TherapistIntervention");
			AddAction(FText::Format(Format, Character->LastName));
		}
		
		const bool bOfficerStressed = Character->StressLevel > Character->PreviousStressLevel;
		if (!bTherapistIntervened && bOfficerStressed && Character->StressLevel > 0.75f)
		{
			const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Action.OfficerStressed");
			AddAction(FText::Format(Format, Character->LastName));
		}

		const bool bOfficerReturned = Character->State == ERosterCharacterState::Available;
		if (bStateChanged && bOfficerReturned)
		{
			const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Action.OfficerAvailable");
			AddAction(FText::Format(Format, Character->LastName));
		}

		const bool bOfficerUnlockedTrait = Character->bJustUnlockedTrait;
		if (bOfficerUnlockedTrait && Character->Trait)
		{
			const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Action.OfficerUnlockedTrait");
			AddAction(FText::Format(Format, Character->LastName, Character->Trait->Name));
		}
		
		const bool bOfficerSurvivedBomb = ExfilData.bActiveBombThreat && Character->State == ERosterCharacterState::Available;
		if (bOfficerSurvivedBomb)
		{
			const FTextFormat Format = FText::FromStringTable(DebriefStringTableId, "entry.Action.OfficerSurvivedBomb");
			AddAction(FText::Format(Format, Character->LastName));
		}
	}
}

void URoundupWidget::CheckIfExfiltratedFromMission()
{
	ALobbyGM* LobbyGM = GetWorld()->GetAuthGameMode<ALobbyGM>();
	if (!LobbyGM || !MetaGameProfile)
		return;
	
	const FExfiltrationData ExfilData = MetaGameProfile->GetTemporaryData().ExfilData;

	if (!ExfilData.bExfiltratedMission)
		return;

	bool bExfilMessageDisplayed = false;

	FString DebriefKey = "entry.Action.ExfiltratedMission";
	if (ExfilData.bActiveBombThreat)
	{
		const FText Text = FText::FromStringTable(DebriefStringTableId, DebriefKey + "BombThreat");
		AddAction(Text);
		bExfilMessageDisplayed = true;
	}

	if (ExfilData.bActiveShooter)
	{
		const FText Text = FText::FromStringTable(DebriefStringTableId, DebriefKey + "ActiveShooter");
		AddAction(Text);
		bExfilMessageDisplayed = true;
	}
	
	if (!bExfilMessageDisplayed)
	{
		const FText Text = FText::FromStringTable(DebriefStringTableId, DebriefKey);
		AddAction(Text);
	}
}

#undef LOCTEXT_NAMESPACE
