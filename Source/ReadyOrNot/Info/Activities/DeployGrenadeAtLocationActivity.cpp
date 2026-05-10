// Void Interactive, 2020

#include "DeployGrenadeAtLocationActivity.h"

#include "Actors/BaseGrenade.h"

#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

#include "ReadyOrNotAIConfig.h"

#include "lib/ReadyOrNotMathLibrary.h"

UDeployGrenadeAtLocationActivity::UDeployGrenadeAtLocationActivity()
{
	ActivityName = FText::FromStringTable("SwatCommandTable", "DeployGrenade");
}

void UDeployGrenadeAtLocationActivity::EnterMoveToStage()
{
	ProgressState = FText::FromStringTable("SwatCommandTable", "InProgress");
	SetLocation(DeployLocation);
}

void UDeployGrenadeAtLocationActivity::TickMoveToStage(const float DeltaTime, const float Uptime)
{
	Super::TickMoveToStage(DeltaTime, Uptime);
}

void UDeployGrenadeAtLocationActivity::EnterDeployStage()
{
	Super::EnterDeployStage();

	if (CheckEdgeCases())
	{
		const TArray<EItemCategory> ItemCategories = DeployItemClass->GetDefaultObject<ABaseItem>()->ItemCategories;

		if (ItemCategories.Contains(EItemCategory::IC_CSGas))
		{
			GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_CS_GAS);
		}
		else if (ItemCategories.Contains(EItemCategory::IC_Stingball))
		{
			GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_STINGER);
		}
		else if (ItemCategories.Contains(EItemCategory::IC_Flashbang))
		{
			GetCharacter()->PlayRawVO(VO_SWAT_GENERAL::RESPONSE_DEPLOY_FLASHBANG);
		}

		EquipItemOfClass(DeployItemClass);

		AbortMove();
	}
}

void UDeployGrenadeAtLocationActivity::TickDeployStage(const float DeltaTime, const float Uptime)
{
	Super::TickDeployStage(DeltaTime, Uptime);
	// Look at destination
	GetCharacter()->SetActorRotation(FMath::RInterpTo(GetCharacter()->GetActorRotation(), UKismetMathLibrary::FindLookAtRotation(GetCharacter()->GetActorLocation(), DeployLocation), DeltaTime, 0.5f));

	if (ABaseGrenade* Grenade = GetCharacter()->GetEquippedItem<ABaseGrenade>())
	{
		TryThrowGrenade(Grenade, DeltaTime, Uptime);

		if (Grenade->bStartedDetonating || Grenade->bThrowStarted)
		{
			OwningController->FinishActivity(this, true, true);
		}
	}
}

bool UDeployGrenadeAtLocationActivity::CanDeploy() const
{
	FHitResult HitResult;
	const FVector Start = GetCharacter()->GetMesh() ? GetCharacter()->GetMesh()->GetSocketLocation(SOCKET_EYES_VIEW_POINT) : GetCharacter()->GetActorLocation();
	const FVector End = DeployLocation + FVector::UpVector * 20.0f;
	FVector Direction = (Start - End).GetSafeNormal();
	FVector StartLocation1 = Start + Direction.RightVector * 0.5f;
	FVector StartLocation2 = Start + Direction.LeftVector * 0.5f;
	const bool bCanSee = !GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation1, End, ECC_Visibility, GetCharacter()->GetCollisionQueryParameters()) &&
		!GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation2, End, ECC_Visibility, GetCharacter()->GetCollisionQueryParameters());

	DrawDebugLine(GetWorld(), Start, End, bCanSee ? FColor::Green : FColor::Red, false, 1.0f, 0, 0.5f);

	return HasReachedLocation() || bCanSee;
}

FString UDeployGrenadeAtLocationActivity::GetDeployAnimation() const
{
	return "";
}

void UDeployGrenadeAtLocationActivity::TryThrowGrenade(ABaseGrenade* Grenade, const float DeltaTime, const float Uptime)
{
	if (!Grenade)
		return;

	// TODO: fix this activity
	if (!Grenade->bStartedDetonating && GetCharacter()->bPrimed && Uptime > 1.0f)
	{
		const FVector Start = GetCharacter()->GetMesh() ? GetCharacter()->GetMesh()->GetSocketLocation(SOCKET_EYES_VIEW_POINT) : GetCharacter()->GetActorLocation();
		const FVector End = DeployLocation + FVector::UpVector * 100.0f;
		const FVector ThrowDirection = (End - Start).GetSafeNormal() * 1.1f;
		Grenade->ThrowDistance = FVector::Distance(Start, DeployLocation + ThrowDirection);

		Grenade->bAIThrowComplete = true; // Must be set through anim notify for throw to be accurate

		Grenade->bForceNoSimulateGrenadePathOnThrow = false;
		Grenade->bCanThrowGrenade = true;
		Grenade->bThrowStarted = false;
		Grenade->Throw(false, true, ThrowDirection);
		Grenade->OnItemPrimaryUseEnd();
		Grenade->StartDetonationTimer();
	}

	if (!GetCharacter()->bPrimed)
	{
		GetCharacter()->ConsumeMovementInputVector();
		Grenade->bForceNoSimulateGrenadePathOnThrow = false;
		Grenade->OnItemPrimaryUse();
	}
}
