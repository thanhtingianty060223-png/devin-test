// Copyright Void Interactive, 2021

#include "FMODAudioPropagationComponent.h"

#include "FMODWorldSubsystem.h"
#include "Actors/Door.h"
#include "Material/Physical/CustomPhysicalMaterial.h"
#include "NavigationSystem/Public/NavigationSystem.h"

UFMODAudioPropagationComponent::UFMODAudioPropagationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.4f;
	
	bAutoActivate = true;
	bAutoRegister = true;
	bCanEverAffectNavigation = false;
	bNavigationRelevant = false;

	OcclusionAmount = 0.0f;
	silentDistance = 400.0f;
}

void UFMODAudioPropagationComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OcclusionAmount = 0.0f;
}

// void UFMODAudioPropagationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
// {
// 	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
// 	AActor* owner = GetOwner();
// 	AActor* localPlayer = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
// 	if (owner != localPlayer)
// 	{
// 		if (IsPlaying())
// 		{
// 			CheckOcclusion();
// 		}
// 		// if (OcclusionAmount > 0)
// 		// {
// 		// 	UpdateAudioPropagation();
// 		// }
// 	}
// }

/*
bool UFMODAudioPropagationComponent::UpdateAudioPropagation()
{
	return false;
	if (bSearchingPath)
		return false;

	APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	if (LocalPlayer)
	{
		//UE_LOG(LogTemp, Warning, TEXT("PLAYER GOT");
		UNavigationSystemV1* const NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FNavAgentProperties Agent;
			Agent.AgentHeight = 1.0f;
			Agent.AgentRadius = 1.0f;
			Agent.bCanFly = false;
			Agent.bCanWalk = true;

			float distanceFromSource = GetComponentLocation().Distance(this->GetComponentLocation(),
			                                                           LocalPlayer->GetActorLocation());
			if (distanceFromSource <= minDistance)
			{
				bPlayPropagation = false;
			}
			else
			{
				bPlayPropagation = true;
			}

			FPathFindingQuery PathFindingQuery;
			PathFindingQuery.StartLocation = GetComponentLocation();
			PathFindingQuery.EndLocation = LocalPlayer->GetActorLocation();
			const ANavigationData* NavData = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())->
				GetDefaultNavDataInstance();
			if (NavData)
			{
				PathFindingQuery.QueryFilter = UNavigationQueryFilter::GetQueryFilter<UNavigationQueryFilter>(*NavData);
				if (PathFindingQuery.NavData.Get())
				{
					PathFindingQuery.NavData = NavData;
					FNavPathQueryDelegate NavDelegate;
					NavDelegate.BindUObject(this, &UFMODAudioPropagationComponent::OnPathFound);
					NavSys->FindPathAsync(Agent, PathFindingQuery, NavDelegate);
					V_LOGM(LogReadyOrNot, "[%s] Requesting Path!", *FString(__FUNCTION__));
					bSearchingPath = true;
					return true;
				}
			}
		}
		return false;
	}
	return false;
}
*/

float UFMODAudioPropagationComponent::GetOcclusionAmount()
{
	CheckOcclusion();
	
	return OcclusionAmount;
}

float UFMODAudioPropagationComponent::GetDepthMaterialOcclusionAmount(UWorld* World, TArray<AActor*> ActorsToIgnore, FVector SoundLocation, FVector ListenerLocation, float DefaultOcclusionDepth, float OcclusionMultiplier)
{
	TArray<FHitResult> ForwardHits;
	TArray<FHitResult> ReverseHits;
	
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.bReturnPhysicalMaterial = true;
	CollisionQueryParams.bTraceComplex = true;

	for(int i = 0; i < ActorsToIgnore.Num(); i++)
	{
		if(!ActorsToIgnore[i])
		{
			continue;
		}
		CollisionQueryParams.AddIgnoredActor(ActorsToIgnore[i]);
	}
	
	// FCollisionObjectQueryParams CollisionObjectQueryParams;
	// CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	// CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	// CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);

	World->LineTraceMultiByChannel(ForwardHits, SoundLocation, ListenerLocation, ECC_OCCLUSION, CollisionQueryParams);
	World->LineTraceMultiByChannel(ReverseHits, ListenerLocation, SoundLocation, ECC_OCCLUSION, CollisionQueryParams);

	//DrawDebugLine(GetWorld(), SoundLocation, ListenerLocation, FColor::Green, false, 20);
	//DrawDebugLine(GetWorld(), ListenerLocation, SoundLocation, FColor::Red, false, 20);
	
	// Add hitresults to component map
	TMap<UActorComponent*, TArray<FHitResult>> CompToIndex;
	for(int i = 0; i < ForwardHits.Num(); i++)
	{
		FHitResult ForwardHitResult = ForwardHits[i];
		if(CompToIndex.Contains(ForwardHitResult.GetComponent()))
		{	
			CompToIndex.Find(ForwardHitResult.GetComponent())->Add(ForwardHitResult);
		}
		else
		{
			CompToIndex.Add(ForwardHitResult.GetComponent(), TArray<FHitResult>{ForwardHitResult});
		}
	}
	for(int i = 0; i < ReverseHits.Num(); i++)
	{
		FHitResult ReverseHitResult = ReverseHits[i];
		if(CompToIndex.Contains(ReverseHitResult.GetComponent()))
		{	
			CompToIndex.Find(ReverseHitResult.GetComponent())->Add(ReverseHitResult);
		}
		else
		{
			CompToIndex.Add(ReverseHitResult.GetComponent(), TArray<FHitResult>{ReverseHitResult});
		}
	}

	float NewOcclusionAmount = 0.0f;
	// For every pair of hits
	for (auto It = CompToIndex.CreateConstIterator(); It; ++It)
	{
		// Make sure we have two hits.
		if(It.Value().Num() == 2)
		{
			FHitResult ForwardHitResult = It.Value()[0];
			FHitResult ReverseHitResult = It.Value()[1];
	
			//DrawDebugSphere(GetWorld(), ForwardHitResult.Location, 10, 10, FColor::Green, false, 20);
			//DrawDebugSphere(GetWorld(), ReverseHitResult.Location, 10, 10, FColor::Red, false, 20);

			// Check for specific actor tags
			if(ForwardHitResult.GetActor())
			{
				// If the actor shouldn't occlude.
				if (ForwardHitResult.GetActor()->ActorHasTag("NoOcclusion"))
				{
					continue;
				}
	
				// If the actor should not let any gunshot sound through.
				if (ForwardHitResult.GetActor()->ActorHasTag("NoSound"))
				{
					return -1;
				}
			}
	
			// If we hit a door, we should occlude more.
			float NewDoorMultiplier = 1.0f;
			if(ForwardHitResult.GetActor())
			{
				if(ADoor* DoorActor = Cast<ADoor>(ForwardHitResult.GetActor()))
				{
					NewDoorMultiplier = DoorActor->OcclusionMultiplier;
				}
			}
	
			// Calculate object depth based on reverse and forward hits
			float ObjectDepth = FVector::Distance(ForwardHitResult.Location, ReverseHitResult.Location);
	
			// Get custom physmat if it exists
			UPhysicalMaterial* PhysicalMaterial = ForwardHitResult.PhysMaterial.Get();
			UCustomPhysicalMaterial* CustomPhysicalMaterial = Cast<UCustomPhysicalMaterial>(PhysicalMaterial);
	
			// If there is a custom physmat, get the occlusion params.
			if (CustomPhysicalMaterial)
			{
				NewOcclusionAmount += (ObjectDepth / CustomPhysicalMaterial->FullOcclusionDepth) * OcclusionMultiplier * NewDoorMultiplier;
			}
			// Default occlusion params.
			else
			{
				NewOcclusionAmount += (ObjectDepth / DefaultOcclusionDepth) * OcclusionMultiplier * NewDoorMultiplier;
			}
		}
		// If we only have one hit, we hit something one sided.
		else
		{
			NewOcclusionAmount += 0.30f * OcclusionMultiplier;
		}
	}
	NewOcclusionAmount = FMath::Clamp(NewOcclusionAmount, -1.0f, 1.0f);
	return NewOcclusionAmount;
}


void UFMODAudioPropagationComponent::OnPathFound(uint32 PathId, ENavigationQueryResult::Type ResultType,
                                                 FNavPathSharedPtr NavPath)
{
	// no nav data loaded
	if (!NavPath)
		return;
	if (ResultType != ENavigationQueryResult::Success)
		return;

	bSearchingPath = false;
	APlayerCharacter* LocalPlayer = UBpGameplayHelperLib::GetLocalPlayerCharacter(GetWorld());
	if (LocalPlayer)
	{
		if (NavPath->GetPathPoints().Num() > 2)
		{
			// Do trace from last path point towards second to last path point...
			FNavPathPoint LastPathPoint = NavPath->GetPathPoints()[NavPath->GetPathPoints().Num() - 1];
			FNavPathPoint SecondLastPathPoint = NavPath->GetPathPoints()[NavPath->GetPathPoints().Num() - 2];

			FVector StartTrace = LastPathPoint.Location;
			FVector EndTrace = (SecondLastPathPoint.Location - LastPathPoint.Location) * 200000.0f;
			FHitResult Hit;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(GetOwner());
			QueryParams.AddIgnoredActors(LocalPlayer->GetCollisionIgnoredActors());
			FCollisionObjectQueryParams CollisionObjectParams;
			CollisionObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
			GetWorld()->LineTraceSingleByObjectType(Hit, StartTrace, EndTrace, CollisionObjectParams, QueryParams);
			//DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor::Green, false, 1.0f, 1, 1);


			AudioPlayLocation = NavPath->GetPathPoints()[1].Location;
			float distanceToOrigin = FVector::Dist(NavPath->GetPathPoints()[0], AudioPlayLocation);
			float distanceToPlayer = FVector::Dist(SecondLastPathPoint.Location, LocalPlayer->GetActorLocation());
			if (distanceToPlayer < silentDistance || distanceToOrigin < silentDistance)
			{
				volumeToSet = 0.0f;
			}
			else
			{
				volumeToSet = 0.6f * FMath::Clamp(300.0f / distanceToOrigin, 0.0f, 1.0f);
			}
		}
	}
	//GetWorld()->GetTimerManager().SetTimer(UpdateAudioPropagation_Handle, this, &UFMODAudioPropagationComponent::UpdateAudioPropagation, 1.0f, true);
}

FFMODEventInstance UFMODAudioPropagationComponent::PlayEvent(UFMODEvent* EventToPlay, FVector Origin, TArray<FMODParam> Params)
{
	FFMODEventInstance EventInst = UFMODBlueprintStatics::PlayEventAtLocation(this, EventToPlay, FTransform(Origin), false);
	if (UFMODBlueprintStatics::EventInstanceIsValid(EventInst))
	{
		// Ensure we aren't already passing in an occlusion parameter.
		bool bHasOcclusionParam = false;
		for(int i = 0; i < Params.Num(); i++)
		{
			if(Params[i].paramName == "Occlusion")
			{
				bHasOcclusionParam = true;
				break;
			}
		}
		if(!bHasOcclusionParam)
		{
			UFMODBlueprintStatics::EventInstanceSetParameter(EventInst, "Occlusion", OcclusionAmount);
		}
		if (Params.Num() > 0)
		{
			for (int i = 0; i < Params.Num(); i++)
			{
				UFMODBlueprintStatics::EventInstanceSetParameter(EventInst, Params[i].paramName, Params[i].paramVal);
			}
		}
		UFMODBlueprintStatics::EventInstancePlay(EventInst);
	}

	return EventInst;
}

void UFMODAudioPropagationComponent::PlayEventAttached(UFMODEvent* EventToPlay, USceneComponent* CompToAttach, FName AttachPoint, TArray<FMODParam> Params)
{
	PlayEventAttached(EventToPlay, CompToAttach, AttachPoint, Params, true);
}

void UFMODAudioPropagationComponent::PlayEventAttached(UFMODEvent* EventToPlay, USceneComponent* CompToAttach, FName AttachPoint, TArray<FMODParam> Params, bool bCheckOcclusion)
{
	UFMODAudioComponent* AudioComp = nullptr;
	
	if (UFMODWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<UFMODWorldSubsystem>())
	{
		if (UFMODAudioComponent* AudioComponent = Subsystem->RetrieveAudioComponent_World())
		{
			AudioComponent->Rename(nullptr, CompToAttach->GetOwner());
			
			AudioComponent->Event = EventToPlay;
			
			if (!AudioComponent->IsRegistered())
				AudioComponent->RegisterComponentWithWorld(GetWorld());

			AudioComponent->AttachToComponent(CompToAttach, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
			AudioComponent->SetRelativeLocation(FVector::ZeroVector);

			AudioComp = AudioComponent;
		}
		// failsafe
		else
		{
			AudioComp = UFMODBlueprintStatics::PlayEventAttached(EventToPlay, CompToAttach, AttachPoint, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, false, true);
		}
	}
	
	PlayEventAttached(AudioComp, Params, bCheckOcclusion);
}

void UFMODAudioPropagationComponent::PlayEventAttached(UFMODAudioComponent* AudioComp, const TArray<FMODParam>& Params, bool bCheckOcclusion)
{
	if (AudioComp)
	{
		if (bCheckOcclusion)
		{
			CheckOcclusion();
			AudioComp->SetParameter("Occlusion", OcclusionAmount);
		}
		
		if (Params.Num() > 0)
		{
			for (int i = 0; i < Params.Num(); i++)
			{
				AudioComp->SetParameter(Params[i].paramName, Params[i].paramVal);
			}
		}
		
		AudioComp->Play();
	}
}

bool UFMODAudioPropagationComponent::CheckOcclusion()
{
	LOCAL_PLAYER;
	
	if (!LocalPlayer)
		return false;

	AActor* Owner = GetOwner();

	if (!Owner || Owner == LocalPlayer)
	{
		OcclusionAmount = 0.0f;
		return false;
	}

	FCollisionQueryParams CollisionParams = UReadyOrNotFunctionLibrary::CreateCollisionQueryParams(Cast<AReadyOrNotCharacter>(Owner), LocalPlayer);

	FCollisionObjectQueryParams CollisionObjectQueryParams;
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	CollisionObjectQueryParams.AddObjectTypesToQuery(ECC_DOOR);
	
	TArray<FHitResult> Hits;
	GetWorld()->LineTraceMultiByObjectType(Hits, GetComponentLocation(), LocalPlayer->GetFirstPersonCameraComponent()->GetComponentLocation(), CollisionObjectQueryParams);

	const float HitsFloat = Hits.Num();
	
	if (Hits.Num() > 3 || (GetComponentLocation() - LocalPlayer->GetFirstPersonCameraComponent()->GetComponentLocation()).Size() > 3000.0f)
	{
		OcclusionAmount = (HitsFloat - 2) * 0.20f;
	}
	else
	{
		OcclusionAmount = 0.0f;
	}

	return true;
}

void FMODParam::SetValues(FName name, float val)
{
	if (name.IsValid())
	{
		paramName = name;
		paramVal = val;
	}
}
