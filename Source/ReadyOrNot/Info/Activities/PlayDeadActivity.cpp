// Void Interactive, 2020

#include "Info/Activities/PlayDeadActivity.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

TAutoConsoleVariable<int32> CVarPlayDeadActivityDebug(TEXT("PlayDeadActivity.DrawDebug"), 0, TEXT("0 = Dont draw debug info. 1 = Draw debug info"));

UPlayDeadActivity::UPlayDeadActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "PlayDead");
	bNoMoveActivity = true;
	bIsProgressActivity = false;

	MaxActivityTime = 90.0f; // Need to stop playing dead eventually
}

void UPlayDeadActivity::StartActivity(AAIController* Owner)
{
	Super::StartActivity(Owner);

	OwningController->bStopDecisionMaking = true;

	// Initialize states
	PlayDeadDuration = InitialPlayDeadDuration;
	WaitDelay = 0.0f;
	SWATAimTime = 0.0f;
	SWATNoLookTime = 0.0f;
	bBindedEvents = false;
	
	GetCharacter()->PlayDead(0.0f, !bSilentDeath);
	
	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	GetCharacter()->OnHeardOfficerYell.RemoveAll(this);
}

void UPlayDeadActivity::PerformActivity(const float DeltaTime)
{
	Super::PerformActivity(DeltaTime);

	// Wait until ragdoll has almost stopped falling
	WaitDelay += DeltaTime;
	if (WaitDelay < 2.0f)
	{
		bBindedEvents = false;
		return;
	}

	if (!bBindedEvents)
	{
		bBindedEvents = true;
		
		GetCharacter()->OnCharacterTakeDamage.AddDynamic(this, &UPlayDeadActivity::OnDamaged);
		GetCharacter()->OnStunnedEvent.AddDynamic(this, &UPlayDeadActivity::OnStunned);
		GetCharacter()->OnHeardOfficerYell.AddDynamic(this, &UPlayDeadActivity::OnHeardYell);
	}
	
	const bool bAnySWATLooking = IsAnySWATLookingAtUs();
	const bool bAnyPlayerAiming = IsAnyPlayerAimingAtUs();

	bAnyPlayerAiming ? SWATAimTime += DeltaTime : SWATAimTime = 0.0f;
	bAnySWATLooking ? SWATNoLookTime = 0.0f : SWATNoLookTime += DeltaTime;

	if (bAnySWATLooking || bAnyPlayerAiming)
	{
		PlayDeadDuration = SeenSWATPlayDeadDuration;
	}

	// Stop playing dead if we need to do so now or if we're not being looked at by swat or is being aimed at by swat for a while
	if (SWATNoLookTime > PlayDeadDuration)
	{
		StopPlayingDeadNow();
		return;
	}

	if ((SWATAimTime > PlayDeadDuration/2))
	{
		StopPlayingDeadNow(true);
		return;
	}
}

#if !UE_BUILD_SHIPPING
void UPlayDeadActivity::PerformActivity_Debug(const float DeltaTime)
{
	Super::PerformActivity_Debug(DeltaTime);

	if (CVarPlayDeadActivityDebug.GetValueOnAnyThread() > 0)
	{
		const FString DebugStringAI = FString::Printf(TEXT("Swat Aim Time: %.2f"), SWATAimTime) + LINE_TERMINATOR +
									FString::Printf(TEXT("Swat No Look At Time: %.2f"), SWATNoLookTime);
		
		DrawDebugString(GetWorld(), GetCharacter()->GetActorLocation(), DebugStringAI, nullptr, FColor::White, DeltaTime/* + 0.04f*/, true);
	}
}

void UPlayDeadActivity::GatherDebugString(FString& OutString)
{
	OutString += AddDebugString("Swat Aim Time", FString::Printf(TEXT("%.2f"), SWATAimTime));
	OutString += AddDebugString("Swat No Look At Time", FString::Printf(TEXT("%.2f"), SWATNoLookTime));
}
#endif

void UPlayDeadActivity::FinishedActivity(const bool bSuccess)
{
	Super::FinishedActivity(bSuccess);
	
	OwningController->bStopDecisionMaking = false;
	
	GetCharacter()->OnCharacterTakeDamage.RemoveAll(this);
	GetCharacter()->OnStunnedEvent.RemoveAll(this);
	GetCharacter()->OnHeardOfficerYell.RemoveAll(this);
}

bool UPlayDeadActivity::CanOverrideActivity() const
{
	return false;
}

bool UPlayDeadActivity::CanFinishActivity() const
{
	// Only force finished
	return false;
}

bool UPlayDeadActivity::IsAnySWATLookingAtUs() const
{
	for (TActorIterator<AReadyOrNotCharacter> It(GetWorld()); It; ++It)
	{
		AReadyOrNotCharacter* Character = *It;

		if (Character->IsOnSWATTeam())
		{
			FHitResult Hit;
			FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Character, GetCharacter());

			FVector Start = Character->GetMesh()->GetSocketLocation("head_end");
			FVector End = GetCharacter()->GetMesh()->GetSocketLocation("head_end");

			bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionQueryParams);
			
			if (bHasLOS)
			{
				const FVector DirectionToUs = (GetCharacter()->GetMesh()->GetSocketLocation("spine_3") - Start).GetSafeNormal2D();
				const float DotProduct = FVector::DotProduct(Character->GetActorForwardVector(), DirectionToUs);

				if (DotProduct > 0.5f)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool UPlayDeadActivity::IsAnyPlayerAimingAtUs() const
{
	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		APlayerCharacter* Character = *It;

		if (Character->IsOnSWATTeam())
		{
			FHitResult Hit;
			FCollisionQueryParams CollisionQueryParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Character, GetCharacter());

			FVector Start = Character->GetMesh()->GetSocketLocation("head_end");
			FVector End = GetCharacter()->GetMesh()->GetSocketLocation("head_end");

			const float Distance = FVector::Distance(Start, End);

			if (Distance > 500.0f)
				continue;
			
			bool bHasLOS = !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionQueryParams);
			
			if (bHasLOS)
			{
				if (!Character->bAiming)
					continue;
				
				const FVector DirectionToUs = (End - Start).GetSafeNormal2D();
				const float DotProduct = FVector::DotProduct(Character->GetActorForwardVector(), DirectionToUs);

				if (DotProduct > 0.9f)
				{
					return true;
				}
			}
		}
	}

	return false;
}

void UPlayDeadActivity::StopPlayingDeadNow(const bool bSurrender)
{
	GetCharacter()->bSurrendered = bSurrender;
	GetCharacter()->bSurrenderComplete = bSurrender;
	GetCharacter()->StopPlayingDead();
	OwningController->FinishActivity(this, true, true);
}

void UPlayDeadActivity::OnStunned(AReadyOrNotCharacter* StunnedCharacter, float Duration, EStunType StunType, AActor* DamageCauser)
{
	if (WaitDelay < 2.0f)
		return;

	StopPlayingDeadNow(true);
}

void UPlayDeadActivity::OnHeardYell(AReadyOrNotCharacter* Shouter, const bool bLOS)
{
	if (WaitDelay < 2.0f)
		return;

	const float DistanceToEnemy = FVector::Distance(GetCharacter()->GetActorLocation(), Shouter->GetActorLocation());
	const FVector DirectionToEnemy = (GetCharacter()->GetActorLocation() - Shouter->GetActorLocation()).GetSafeNormal2D();
	const float ForwardDotProduct = FVector::DotProduct(DirectionToEnemy, Shouter->GetActorForwardVector());

	// Is the shouter yelling at us?
	if (DistanceToEnemy < 500.0f && ForwardDotProduct > 0.95f)
	{
		const bool bSuccess = FMath::FRand() < 0.3f; // 30% exit chance
		
		if (bSuccess)
		{
			StopPlayingDeadNow(true);
		}
	}
}

void UPlayDeadActivity::OnDamaged(AReadyOrNotCharacter* InstigatorCharacter, AReadyOrNotCharacter* DamagedCharacter, AActor* DamageCauser, float Damage, float HealthRemaining)
{
	if (WaitDelay < 2.0f)
		return;

	StopPlayingDeadNow(true);
}
