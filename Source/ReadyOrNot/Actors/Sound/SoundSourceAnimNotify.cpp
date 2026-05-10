// Copyright Void Interactive, 2023

#include "SoundSourceAnimNotify.h"

USoundSourceAnimNotify::USoundSourceAnimNotify()
    : Super()
{
    bFollow = false;
    Event = nullptr;

    #if WITH_EDITORONLY_DATA
    NotifyColor = FColor(196, 142, 255, 255);
    #endif
}

void USoundSourceAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSeq)
{
    if (Event)
    {
        if (bFollow)
        {
            #if WITH_EDITOR
            if (MeshComp->GetWorld()->WorldType == EWorldType::Editor ||
                MeshComp->GetWorld()->WorldType == EWorldType::EditorPreview)
            {
                UFMODBlueprintStatics::PlayEventAttached(Event, MeshComp, *AttachName, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, false, true, true);
                return;
            }
            #endif

            APlayerCharacter* Owner = Cast<APlayerCharacter>(MeshComp->GetOwner());
            if (Owner && Owner->IsLocalPlayer())
            {
                USoundSource* SoundSource = USoundSource::CreateFirstPersonSound(MeshComp->GetWorld(), Event, FTransform(MeshComp->GetSocketLocation(*AttachName)), {}, false);
                if(SoundSource)
                {
                    SoundSource->Attach(MeshComp, *AttachName);
                    SoundSource->Play();
                }
            }
            else
            {
                USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(MeshComp->GetWorld(), Event, FTransform(MeshComp->GetSocketLocation(*AttachName)), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
                if(SoundSource)
                {
                    SoundSource->Attach(MeshComp, *AttachName);
                    SoundSource->Play();
                }
            }
        }
        else
        {

            #if WITH_EDITOR
            if (MeshComp->GetWorld()->WorldType == EWorldType::Editor || MeshComp->GetWorld()->WorldType == EWorldType::EditorPreview)
            {
                UFMODBlueprintStatics::PlayEventAtLocation(MeshComp, Event, MeshComp->GetComponentTransform(), true);
                return;
            }
            #endif


            APlayerCharacter* Owner = Cast<APlayerCharacter>(MeshComp->GetOwner());
            if (Owner && Owner->IsLocalPlayer())
            {
                USoundSource* SoundSource = USoundSource::CreateFirstPersonSound(MeshComp->GetWorld(), Event, FTransform(MeshComp->GetSocketLocation(*AttachName)), {}, false);
                if(SoundSource)
                {
                    SoundSource->Play();
                }
            }
            else
            {
                USoundSource* SoundSource = USoundSource::CreateThirdPersonSound(MeshComp->GetWorld(), Event, FTransform(MeshComp->GetSocketLocation(*AttachName)), {}, EOcclusionType::OT_Angular, EPropagationType::PT_Portal, false);
                if(SoundSource)
                {
                    SoundSource->Play();
                }
            }
        }
    }
}

FString USoundSourceAnimNotify::GetNotifyName_Implementation() const
{
    if (Event)
    {
        return Event->GetName();
    }
    
    return Super::GetNotifyName_Implementation();
}