#include "InteractionActor.h"
#include "ReadyOrNot.h"

AInteractionActor::AInteractionActor()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	UseIconRadius = CreateDefaultSubobject<USphereComponent>(TEXT("IconRadius"));
	UseIconRadius->SetupAttachment(SceneRoot);
	UseIconRadius->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	UseIconRadius->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel6);

	Mesh_Static = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh_Static"));
	Mesh_Static->SetupAttachment(SceneRoot);
	Mesh_Static->SetCollisionProfileName("Item");

	Mesh_Skeletal = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh_Skeletal"));
	Mesh_Skeletal->SetupAttachment(SceneRoot);
	Mesh_Skeletal->SetCollisionProfileName("Item");
}

void AInteractionActor::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInteractionActor, bHoldButtonPrompt);
	DOREPLIFETIME(AInteractionActor, bCanUseNow);
	DOREPLIFETIME(AInteractionActor, bAvailableForUse);
	DOREPLIFETIME(AInteractionActor, bCompleteIcon);
	DOREPLIFETIME(AInteractionActor, bOverrideButtonPrompt);
	DOREPLIFETIME(AInteractionActor, OverrideButtonPromptText);
}

void AInteractionActor::Server_TryUse_Implementation(AActor* User)
{
	if (!bCanUseNow || !bAvailableForUse)
	{
		return;
	}

	OnActorUsed(User);
}

void AInteractionActor::Server_EndUse_Implementation(AActor* User)
{
	OnActorUsedEnd(User);
}

bool AInteractionActor::CanUse_Implementation(class APlayerCharacter* User)
{
	return bCanUseNow;
}

bool AInteractionActor::IsAvailableForUse_Implementation()
{
	return bAvailableForUse;
}

bool AInteractionActor::StartUse_Implementation(class APlayerCharacter* User)
{
	Server_TryUse(User);
	Server_TryUse_Implementation(User);

	if (bButtonPushAnimation && User && User->GetEquippedItem() && User->GetEquippedItem()->HasButtonPushAnimation())
	{
		User->GetEquippedItem()->PlayButtonPushAnimation();
	}
	return true;
}

void AInteractionActor::EndUse_Implementation(class APlayerCharacter* User)
{
	Server_EndUse(User);
	Server_EndUse_Implementation(User);
}

bool AInteractionActor::OverridesUseButtonPromptText_Implementation()
{
	return bOverrideButtonPrompt;
}

FText AInteractionActor::GetUseButtonPromptText_Implementation()
{
	return OverrideButtonPromptText;
}

bool AInteractionActor::PlaysUseIconComplete_Implementation()
{
	return bCompleteIcon;
}

USceneComponent* AInteractionActor::GetUseIconBoltComponent_Implementation()
{
	return SceneRoot;
}

TArray<USceneComponent*> AInteractionActor::GetUseViewComponents_Implementation()
{
	if (CachedUseComponents.Num() <= 0)
	{
		CachedUseComponents.Add(Mesh_Static);
		CachedUseComponents.Add(Mesh_Skeletal);
	}

	return CachedUseComponents;
}