// Copyright Void Interactive, 2021

#include "SlowDownVolume.h"
#include "ReadyOrNot.h"

ASlowDownVolume::ASlowDownVolume()
{

}

void ASlowDownVolume::BeginPlay()
{
	Super::BeginPlay();
	OnActorBeginOverlap.RemoveDynamic(this, &ASlowDownVolume::OnOverlapBegin);
	OnActorBeginOverlap.AddDynamic(this, &ASlowDownVolume::OnOverlapBegin);
	OnActorEndOverlap.RemoveDynamic(this, &ASlowDownVolume::OnOverlapEnd);
	OnActorEndOverlap.AddDynamic(this, &ASlowDownVolume::OnOverlapEnd);
	GetCollisionComponent()->OnComponentBeginOverlap.RemoveDynamic(this, &ASlowDownVolume::OnOverlapBeginComponent);
	GetCollisionComponent()->OnComponentBeginOverlap.AddDynamic(this, &ASlowDownVolume::OnOverlapBeginComponent);
	GetCollisionComponent()->OnComponentEndOverlap.RemoveDynamic(this, &ASlowDownVolume::OnOverlapEndComponent);
	GetCollisionComponent()->OnComponentEndOverlap.AddDynamic(this, &ASlowDownVolume::OnOverlapEndComponent);

}

void ASlowDownVolume::OnOverlapBegin(class AActor* OverlappedActor, class AActor* OtherActor)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (pc)
	{
		pc->SetSlowDownSpeed(SlowDownMultiplier);
	}
}



void ASlowDownVolume::OnOverlapEnd(class AActor* OverlappedActor, class AActor* OtherActor)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (pc)
	{
		pc->EndSlowDownSpeed();
	}
}

void ASlowDownVolume::OnOverlapBeginComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (pc)
	{
		pc->SetSlowDownSpeed(SlowDownMultiplier);
	}
}

void ASlowDownVolume::OnOverlapEndComponent(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerCharacter* pc = Cast<APlayerCharacter>(OtherActor);
	if (pc)
	{
		pc->EndSlowDownSpeed();
	}
}
