// Copyright Void Interactive, 2021

#include "SkinComponent.h"
#include "ReadyOrNot.h"
#include "lib/GameFeatureLibrary.h"

USkinComponent::USkinComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void USkinComponent::ResetSkin()
{
	for (auto& s : PreAppliedSkeletalMeshMap)
	{
		USkeletalMeshComponent* scomp = Cast<USkeletalMeshComponent>(s.Key);
		if (scomp)
		{
			scomp->SetSkeletalMesh(s.Value, false);
		}
	}
	
	PreAppliedSkeletalMeshMap.Empty();

	for (auto& s : PreAppliedStaticMeshMap)
	{
		UStaticMeshComponent* scomp = Cast<UStaticMeshComponent>(s.Key);
		if (scomp)
		{
			scomp->SetStaticMesh(s.Value);
		}
	}

	PreAppliedStaticMeshMap.Empty();
}

void USkinComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void USkinComponent::DestroyComponent(const bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);
	
	ResetSkin();
}

void USkinComponent::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<USkinComponent*> SkinComps;
	GetOwner()->GetComponents<USkinComponent>(SkinComps);
	
	for (USkinComponent* s : SkinComps)
	{
		if (s && s != this)
		{
			s->ResetSkin();
			s->DestroyComponent();
		}
	}
}

void USkinComponent::ApplySkin()
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;
	
	APlayerCharacter* Player = Cast<APlayerCharacter>(Owner);
	if (Player && (Player->IsLocalPlayer() || !Player->GetPlayerState()))
	{
		if (!HasDLCUnlocked())
			return;
	}
	
	//bool bPVPMode = false;
	
	//AReadyOrNotGameState* gs = Cast<AReadyOrNotGameState>(GetWorld()->GetGameState());
	//if (gs)
	//{
	//	bPVPMode = gs->bPvPMode;
	//}

	if (LockedToBlueprint.Num() > 0)
	{
		bool bMatchesAnyBlueprint = false;
		LockedToBlueprint.Remove(nullptr);
		for (int32 i = 0; i < LockedToBlueprint.Num(); i++)
		{
			if (Owner->IsA(LockedToBlueprint[i]))
			{
				bMatchesAnyBlueprint = true;
			}
		}
		
		if (!bMatchesAnyBlueprint)
		{
			DestroyComponent();
			return;
		}
	}
	
	// loop every skeletal mesh up the chain
	// try apply it
	while (Owner)
	{
		TArray<USkeletalMeshComponent*> SkeletalMeshComps;
		Owner->GetComponents<USkeletalMeshComponent>(SkeletalMeshComps);
		
		for (USkeletalMeshComponent* s : SkeletalMeshComps)
		{
			if (SkeletalMeshSkinMap.Find(s->SkeletalMesh) && !PreAppliedSkeletalMeshMap.Contains(s))
			{
				PreAppliedSkeletalMeshMap.Add(s, s->SkeletalMesh);
				
				s->SetSkeletalMesh(*SkeletalMeshSkinMap.Find(s->SkeletalMesh), false);
				s->EmptyOverrideMaterials();
			}
		}

		TArray<UStaticMeshComponent*> StaticMeshComps;
		Owner->GetComponents<UStaticMeshComponent>(StaticMeshComps);
		
		for (UStaticMeshComponent* s : StaticMeshComps)
		{
			if (StaticMeshSkinMap.Find(s->GetStaticMesh()) && !PreAppliedStaticMeshMap.Contains(s))
			{
				PreAppliedStaticMeshMap.Add(s, s->GetStaticMesh());
				
				s->SetStaticMesh(*StaticMeshSkinMap.Find(s->GetStaticMesh()));
				s->EmptyOverrideMaterials();
			}
		}
		
		Owner = Owner->GetOwner();
	}
}

bool USkinComponent::HasDLCUnlocked()
{
	if (bRequiresDLC && UGameFeatureLibrary::IsGameVersionEnabled(DLC))
		return false;
	
	return true;
}

UTexture2D* USkinComponent::GetClassDefaultIcon(TSubclassOf<USkinComponent> SkinComponent)
{
	if (!SkinComponent)
		return nullptr;

	return SkinComponent->GetDefaultObject<USkinComponent>()->Icon.LoadSynchronous();
}

bool USkinComponent::IsFactorySkin()
{
	return bResetsToFactorySkin;
}

// Called every frame
void USkinComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ApplySkin();
}
