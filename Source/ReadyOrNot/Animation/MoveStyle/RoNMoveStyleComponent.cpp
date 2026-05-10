// Copyright Void Interactive, 2022

#include "RoNMoveStyleComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

URoNMoveStyleComponent::URoNMoveStyleComponent()
{
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	DefaultMoveStyleName = "male01_shared_unarmed";
	DefaulGaitName = "walk";
}

void URoNMoveStyleComponent::InitializeComponent()
{
	Super::InitializeComponent();

	//UE_LOG(LogTemp, Warning, TEXT("RoNMoveStyleComponent: InitializeComponent"));

	// attempt to set default specified movestyle
	if (DefaultMoveStyleName != "")
		SetMovementStyleByName(DefaultMoveStyleName);

	if (DefaulGaitName != "")
		SetMovementGaitByName(DefaulGaitName);

	TargetSpeed = 160.0f;
}

void URoNMoveStyleComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(URoNMoveStyleComponent, bIsStrafing);
	DOREPLIFETIME(URoNMoveStyleComponent, Rep_MoveStyleName);
	DOREPLIFETIME(URoNMoveStyleComponent, ActiveGaitName);
	DOREPLIFETIME(URoNMoveStyleComponent, ActiveGaitIndex);
}

void URoNMoveStyleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (GetOwner()->GetLocalRole() >= ROLE_Authority)
	{
		bIsStrafing = bServerIsStrafing;
	}

	AReadyOrNotCharacter* Character = Cast<AReadyOrNotCharacter>(GetOwner());
	if (!Character)
		return;

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement)
		return;

	const bool bIsActiveForMovement = !Character->IsDeadOrUnconscious() && !Character->IsIncapacitated() && !Character->IsInRagdoll();
	if (!bIsActiveForMovement)
	{
		Movement->MaxWalkSpeed = 0.0f;
		Movement->MaxWalkSpeedCrouched = 0.0f;
		Movement->MaxAcceleration = 0.0f;
		return;
	}
	
	const float SmoothedSpeed = FMath::FInterpTo(Movement->MaxWalkSpeed, TargetSpeed, DeltaTime, TargetInterpSpeed);
	Movement->MaxWalkSpeed = SmoothedSpeed;
	Movement->MaxWalkSpeedCrouched = SmoothedSpeed;

	if (GaitTimeOut <= 0.0f)
	{
		if (PendingGaitName != NAME_None)
		{
			SetMovementGaitByName(PendingGaitName, true);
			PendingGaitName = NAME_None;
		}
	}
	else
	{
		GaitTimeOut -= DeltaTime;
	}
	
	//SetCharacterSpeed(SmoothedSpeed);
}

void URoNMoveStyleComponent::OnRep_MoveStyle()
{
	SetMovementStyleByName(Rep_MoveStyleName);
	SetMovementGaitByName(ActiveGaitName);
}

const FRoNMovementStyle* URoNMoveStyleComponent::GetMovementStyleByName(FName Name) const
{
	FRoNMovementStyle* FoundMovementStyle = nullptr;
	
	#if NEW_MOVESTYLE_LAYOUT
	MoveStyleDatabase->ForeachRow<FRoNMoveStyleTable>("GetMovementStyleByName", [&](const FName& RowName, const FRoNMoveStyleTable& RowData)
	{
		for (const FRoNMovementStyle& MoveStyle : RowData.MoveStyles)
		{
			const FString& MoveStyleName = RowName.ToString() + "_" + MoveStyle.Name.ToString();
			
			if (Name != FName(MoveStyleName))
				continue;

			FoundMovementStyle = const_cast<FRoNMovementStyle*>(&MoveStyle);
		}
	});
	#else
	FRoNMoveStyleTable* LookupRow = MoveStyleDatabase->FindRow<FRoNMoveStyleTable>(Name, "GetMovementStyleByName");

	if (LookupRow)
	{
		FoundMovementStyle = &LookupRow->MoveStyle;
	}
	#endif

	return FoundMovementStyle;
}

void URoNMoveStyleComponent::SetOverrideMoveStyleByName(FName Name)
{
	PreviousMoveStyleName = Rep_MoveStyleName;
	SetMovementStyleByName(Name);
	bIsOverriding = true;
}

void URoNMoveStyleComponent::ClearOverrideMoveStyle()
{
	bIsOverriding = false;
	SetMovementStyleByName(PreviousMoveStyleName);
}

void URoNMoveStyleComponent::SetMovementStyleByName(FName Name)
{
	// Must call clear override to allow setting of other movestyles
	if (bIsOverriding)
		return;
	
	if (Name.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("RoNMoveStyleComponent: %s tried to lookup NONE name"), *GetName())
		return;
	}

	// Already set
	if (ActiveMoveStyle.Name == Name)
		return;

	if (MoveStyleDatabase)
	{
		#if NEW_MOVESTYLE_LAYOUT
		MoveStyleDatabase->ForeachRow<FRoNMoveStyleTable>("SetMovementStyleByName", [&](const FName& RowName, const FRoNMoveStyleTable& RowData)
		{
			for (const FRoNMovementStyle& MoveStyle : RowData.MoveStyles)
			{
				const FString& MoveStyleName = RowName.ToString() + "_" + MoveStyle.Name.ToString();
				
				if (Name != FName(MoveStyleName))
					continue;

				MoveStyleCharacterName = RowName;
				ActiveMoveStyle = MoveStyle;
				
				if (GetOwnerRole() == ROLE_Authority)
					Rep_MoveStyleName = Name;
				
				break;
			}
		});

		SetIsStrafing(ActiveMoveStyle.bIsStrafeMovement);

		// allow setting the gait instantly here its safe
		// fail safe when switching move styles, does the current gait still exist? If not revert to default
		if (!SetMovementGaitByName(ActiveGaitName, true))
		{
			SetMovementGaitByName(DefaulGaitName, true);
		}
		
		#else
		FRoNMoveStyleTable* LookupRow = MoveStyleDatabase->FindRow<FRoNMoveStyleTable>(Name, "MoveData Lookup");

		if (LookupRow)
		{
			if (ActiveMoveStyle.Name == LookupRow->MoveStyle.Name)
			{
				return;
			}

			ActiveMoveStyle = LookupRow->MoveStyle;

			// see if this is strafe movement
			if (ActiveMoveStyle.bIsStrafeMovement)
			{
				SetIsStrafing(true);
			}
			else
			{
				SetIsStrafing(false);
			}

			// fail safe when switching move styles, does the current gait still exist? If not revert to default
			if (!SetMovementGaitByName(ActiveGaitName, true))
			{
				SetMovementGaitByName(DefaulGaitName, true);
			}

			//UE_LOG(LogTemp, Warning, TEXT("RoNMoveStyleComponent: Movement Style set to: %s"), *Name)
		}
		#endif
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RoNMoveStyleComponent: No valid MoveStyleDatabase please verify assignment!."));
	}
}

// set GAIT by name searching
bool URoNMoveStyleComponent::SetMovementGaitByName(FName Name, bool bForce)
{
	if (GaitTimeOut > 0.0f && !bForce)
	{
		PendingGaitName = Name;
		return true;
	}

	GaitTimeOut = 1.0f;
	
	if (ActiveMoveStyle.GaitEntries.Num() == 1)
	{
		Name = ActiveMoveStyle.GaitEntries[0].Name;
	}
	
	for (int32 i = 0; i < ActiveMoveStyle.GaitEntries.Num(); i++)
	{
		if (ActiveMoveStyle.GaitEntries[i].Name == Name)
		{
			TargetSpeed = ActiveMoveStyle.GaitEntries[i].Speed; 
			// this should modify the speed to whatever is set inside the gait entries
			SetCharacterSpeed(ActiveMoveStyle.GaitEntries[i].Speed);
			SetCharacterAcceleration(ActiveMoveStyle.GaitEntries[i].Acceleration);

			ActiveGaitName = ActiveMoveStyle.GaitEntries[i].Name;
			ActiveGaitIndex = i;

			return true;
		}
	}
	
	return false;
}

void URoNMoveStyleComponent::SetCharacterSpeed(float Speed)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
		return;

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement)
		return;

	Speed *= CurrentMoveSpeedMultiplier;

	TargetSpeed = Speed;
	
//	Movement->MaxWalkSpeed = Speed;
//	Movement->MaxWalkSpeedCrouched = Speed;
	LastSetSpeed = Speed;
}

void URoNMoveStyleComponent::SetCharacterSpeedMultiplier(float Multiplier)
{
	CurrentMoveSpeedMultiplier = Multiplier;
	SetCharacterSpeed(LastSetSpeed);
}

void URoNMoveStyleComponent::SetCharacterAcceleration(float Acceleration)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
		return;

	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement)
		return;

	Acceleration *= CurrentAccelerationMultiplier;

	Movement->MaxAcceleration = Acceleration;
	LastSetAcceleration = Acceleration;
}

void URoNMoveStyleComponent::SetCharacterAccelerationMultiplier(float Multiplier)
{
	CurrentAccelerationMultiplier = Multiplier;
	SetCharacterAcceleration(LastSetAcceleration);
}

void URoNMoveStyleComponent::SetIsStrafing(bool bNewStrafing)
{
	if (GetOwner()->GetLocalRole() < ROLE_Authority)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, "Client trying to set strafing.... ignoring");
		return;
	}
	
	bServerIsStrafing = bNewStrafing;
}

void URoNMoveStyleComponent::OnRep_IsStrafing()
{
	FString strafstr = (bIsStrafing ? "true" : "false");
}
