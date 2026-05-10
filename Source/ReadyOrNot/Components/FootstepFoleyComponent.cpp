#include "FootstepFoleyComponent.h"
#include "ReadyOrNot.h"
#include "Actors/Environment/FootstepFoleyVolume.h"

UFootstepFoleyComponent::UFootstepFoleyComponent()
{
	SetGenerateOverlapEvents(true);

	bMultiBodyOverlap = false;
	bTraceComplexOnMove = false;
}

void UFootstepFoleyComponent::BeginPlay()
{
	Super::BeginPlay();

	OnComponentBeginOverlap.AddDynamic(this, &UFootstepFoleyComponent::StartedOverlapping);
	OnComponentEndOverlap.AddDynamic(this, &UFootstepFoleyComponent::StoppedOverlapping);
}

void UFootstepFoleyComponent::StartedOverlapping(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(OtherActor);
	if (!Character)
		return;
	
	Character->StartFoley(bPlayEveryStep, SetEventTo, SetEventToRemote);
}

void UFootstepFoleyComponent::StoppedOverlapping(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* Character = Cast<APlayerCharacter>(OtherActor);
	if (!Character)
		return;

	Character->StopFoley();
}