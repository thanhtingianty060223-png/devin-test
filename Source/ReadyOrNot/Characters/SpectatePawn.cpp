// Copyright Void Interactive, 2023

#include "SpectatePawn.h"
#include "PlayerCharacter.h"
#include "ReadyOrNotPlayerController.h"
#include "Actors/Gameplay/ReadyOrNotPlayerState.h"
#include "AI/TrailerSWATCharacter.h"

ASpectatePawn::ASpectatePawn()
{
	PrimaryActorTick.TickInterval = 0.0f;

	PawnCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PawnCamera"));
	PawnCamera->bUsePawnControlRotation = false;
	PawnCamera->SetupAttachment(RootComponent);
}

void ASpectatePawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpectatePawn, Killer);
	DOREPLIFETIME(ASpectatePawn, KilledCharacter);
}

void ASpectatePawn::BeginPlay()
{
	Super::BeginPlay();

	HeadcamMaterialInstance = UMaterialInstanceDynamic::Create(HeadcamMaterial, this);
	
	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->SpectatePawns.AddUnique(this);
	}
}

void ASpectatePawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (AReadyOrNotGameState* GameState = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GameState->SpectatePawns.Remove(this);
	}
}

void ASpectatePawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetLocalRole() == ROLE_AutonomousProxy && GetGameTimeSinceCreation() < 10.0f)
	{
		CleanUpOldPlayer();
	}

	if (GetController() == UBpGameplayHelperLib::GetLocalPlayerController(GetWorld()))
	{
		const AReadyOrNotCharacter* SpecateCharacter = Cast<AReadyOrNotCharacter>(CurrentViewTarget);

		if (KilledCharacter)
		{
			KilledCharacter->SetPlayerState(GetPlayerState());
		}
		
		if (SpecateCharacter && GetController())
		{
			AttachToComponent(SpecateCharacter->GetTeamViewTarget(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "HeadCamSocket");
			
			PawnCamera->SetWorldRotation(SpecateCharacter->GetTeamViewTarget()->GetSocketRotation("HeadCamSocket"));
			
			PawnCamera->PostProcessSettings = {};
			PawnCamera->PostProcessSettings.AddBlendable(HeadcamMaterialInstance, 1.0f);
			PawnCamera->SetFieldOfView(120.0f);
			PawnCamera->SetActive(true);
			
			if (!UBpGameplayHelperLib::HasWidgetInViewport("PreMissionPlanning"))
			{
				Cast<APlayerController>(GetController())->SetViewTarget(this);
			}
			
			if (KilledCharacter)
			{
				KilledCharacter->GetFaceMesh()->SetVisibility(true);
				KilledCharacter->GetTeamViewTarget()->SetVisibility(true);
				if (KilledCharacter->GetInventoryComponent()->GetSpawnedGear().Helmet)
				{
					KilledCharacter->GetInventoryComponent()->GetSpawnedGear().Helmet->ItemMesh->SetVisibility(true);
				}
			}
		}
		else
		{
			if (KilledCharacter)
			{
				PawnCamera->PostProcessSettings = KilledCharacter->GetFirstPersonCameraComponent()->PostProcessSettings;
				AttachToComponent(KilledCharacter->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "DeadCameraView");
				GetController()->SetControlRotation(KilledCharacter->GetMesh()->GetSocketRotation("DeadCameraView"));
				
				KilledCharacter->GetFaceMesh()->SetVisibility(false);
				KilledCharacter->GetTeamViewTarget()->SetVisibility(false);
				if (KilledCharacter->GetInventoryComponent()->GetSpawnedGear().Helmet)
				{
					KilledCharacter->GetInventoryComponent()->GetSpawnedGear().Helmet->ItemMesh->SetVisibility(false);
				}
			}
		}
	}
}

void ASpectatePawn::PossessedBy(class AController* NewController)
{
	Super::PossessedBy(NewController);
	
	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(NewController);
	if (pc)
	{
		if (pc->bWantsHudClearOnPossess)
		{
			pc->HideHUD();
			//pc->Client_ClearHUDWidgets(); // takes 9ms... yikes
		}

		if (!bHideWidgets)
			pc->Client_CreateWidget("SpectateHUD");

		pc->FlushPressedKeys();
	}
}

void ASpectatePawn::UnPossessed()
{
	Super::UnPossessed();
	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* pc = *It;
		pc->GetFaceMesh()->SetVisibility(true);
		pc->GetTeamViewTarget()->SetVisibility(true);
		if (pc->GetInventoryComponent()->GetSpawnedGear().Helmet)
		{
			pc->GetInventoryComponent()->GetSpawnedGear().Helmet->ItemMesh->SetVisibility(true);
		}
	}
}

void ASpectatePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	if (!bDeadSpectatePawn)
	{
		Super::SetupPlayerInputComponent(PlayerInputComponent);
	}

	PlayerInputComponent->BindAction("SpectateNextPlayer", IE_Pressed, this, &ASpectatePawn::SpectateNextPlayer);
	PlayerInputComponent->BindAction("SpectatePreviousPlayer", IE_Pressed, this, &ASpectatePawn::SpectatePreviousPlayer);
	PlayerInputComponent->BindAction("Chat", IE_Pressed, this, &ASpectatePawn::OnChatPressed);
	PlayerInputComponent->BindAction("TeamChat", IE_Pressed, this, &ASpectatePawn::OnTeamChatPressed);
	PlayerInputComponent->BindAction("EscapeMenu", IE_Pressed, this, &ASpectatePawn::EscapeMenu);
}

void ASpectatePawn::OnChatPressed_Implementation()
{
	if (SpectatorHUD)
	{
		SpectatorHUD->ChatPressed();
	}
}

void ASpectatePawn::OnTeamChatPressed_Implementation()
{
	if (SpectatorHUD)
	{
		SpectatorHUD->TeamChatPressed();
	}
}

void ASpectatePawn::CenterPrint_Implementation(FName Type, float Duration, APlayerCharacter* Other)
{
	if (SpectatorHUD)
	{
		SpectatorHUD->CenterPrint(Type, Duration, Other);
	}
	else
	{
		bPendingCenterprint = true;
		PendingCenterprintDuration = Duration;
		PendingCenterprintType = Type;
		PendingCenterprintOther = Other;
	}
}

void ASpectatePawn::EscapeMenu()
{
	AReadyOrNotPlayerController* pc = Cast<AReadyOrNotPlayerController>(GetController());
	if (pc)
	{
		pc->EscapeMenu();
	}
}

void ASpectatePawn::CleanUpOldPlayer_Implementation()
{
	for (TActorIterator<ABaseItem> It(GetWorld()); It; ++It)
	{
		ABaseItem* bi = *It;
		if (bi)
		{
			bi->DisableWeaponFovShader();
			bi->AttachStatic();
		}
		
	}

	for (TActorIterator<APlayerCharacter>It(GetWorld()); It; ++It)
	{
		It->GetInventoryComponent()->ReattachAllGear();
	}
}

void ASpectatePawn::SpectateNextPlayer()
{
	TArray<AReadyOrNotCharacter*> compatibleCharacters = GetCompatibleViewTargets();

	int32 index = compatibleCharacters.Find(Cast<AReadyOrNotCharacter>(CurrentViewTarget));
	if (compatibleCharacters.IsValidIndex(index + 1))
		SetViewTarget(compatibleCharacters[index + 1]);
	else if (compatibleCharacters.IsValidIndex(0))
		SetViewTarget(compatibleCharacters[0]);
	else
		SetViewTarget(nullptr);
}

void ASpectatePawn::SpectatePreviousPlayer()
{
	TArray<AReadyOrNotCharacter*> compatibleCharacters = GetCompatibleViewTargets();

	int32 index = compatibleCharacters.Find(Cast<AReadyOrNotCharacter>(CurrentViewTarget));
	if (compatibleCharacters.IsValidIndex(index - 1))
		SetViewTarget(compatibleCharacters[index - 1]);
	else if (compatibleCharacters.IsValidIndex(compatibleCharacters.Num() - 1))
		SetViewTarget(compatibleCharacters[compatibleCharacters.Num() - 1]);
}

TArray<AReadyOrNotCharacter*> ASpectatePawn::GetCompatibleViewTargets() const
{
	AReadyOrNotPlayerState* ReadyOrNotPlayerState = Cast<AReadyOrNotPlayerState>(GetPlayerState());
	if (!ReadyOrNotPlayerState)
		return {};
	
	TArray<AReadyOrNotCharacter*> CompatibleCharacters;
	CompatibleCharacters.Reserve(8);
	
	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		// Include player characters
		APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(*It);
		if (IsValid(PlayerCharacter) && !PlayerCharacter->IsDeadOrUnconscious())
		{
			CompatibleCharacters.Add(PlayerCharacter);
			continue;
		}

		// Include AI characters
		AReadyOrNotCharacter* Character = *It;
		if (Character->IsActive() && !Cast<ATrailerSWATCharacter>(Character))
		{
			const bool bFriendly = AReadyOrNotCharacter::IsOnSameTeam(Character, ReadyOrNotPlayerState->GetPawn<AReadyOrNotCharacter>());
			if (bFriendly && !Character->IsDeadOrUnconscious())
			{
				CompatibleCharacters.Add(Character);
			}
		}
	}
	
	return CompatibleCharacters;
}

ETeamType ASpectatePawn::GetTeam() const
{
	AReadyOrNotPlayerState* ps = Cast<AReadyOrNotPlayerState>(GetPlayerState());
	if (ps)
	{
		return ps->GetTeam();
	}
	return ETeamType::TT_NONE;
}

void ASpectatePawn::SetViewTarget(AReadyOrNotCharacter* inCharacter)
{
	if (!GetController() || !inCharacter)
		return;

	CurrentViewTarget = inCharacter;

	if (SpectatorHUD)
	{
		SpectatorHUD->OnNewCharacterViewed(inCharacter);
	}

	AttachToComponent(inCharacter->GetTeamViewTarget(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, "HeadCamSocket");
}
