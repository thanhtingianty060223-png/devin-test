// Void Interactive, 2020


#include "AnimNotifyState_SpawnLight.h"

void UAnimNotifyState_SpawnLight::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration)
{
	// ##UE5UPGRADE##
	const FAnimNotifyEventReference EventReference;
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	PointLight = MeshComp->GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass());
	PointLight->GetLightComponent()->SetLightColor(LightColor);
	PointLight->GetLightComponent()->SetIntensity(StartIntensity);
	PointLight->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	PointLight->GetLightComponent()->SetCastShadows(false);
	MaxDuration = TotalDuration;
	CurrentDuration = 0.0f;
}

void UAnimNotifyState_SpawnLight::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime)
{
	CurrentDuration += FrameDeltaTime;
	// ##UE5UPGRADE##
	const FAnimNotifyEventReference EventReference;
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (PointLight)
	{
		float DesiredIntensity = MiddleIntensity;
		if (CurrentDuration >= MaxDuration * 0.5f)
		{
			DesiredIntensity = EndIntensity;
		}
		PointLight->GetLightComponent()->SetIntensity(FMath::FInterpTo(PointLight->GetLightComponent()->Intensity, DesiredIntensity, FrameDeltaTime, InterpSpeed));
	}
}

void UAnimNotifyState_SpawnLight::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	// ##UE5UPGRADE##
	const FAnimNotifyEventReference EventReference;
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (PointLight)
	{
		PointLight->Destroy();
		PointLight = nullptr;
	}
}
