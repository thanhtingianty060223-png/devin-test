// Copyright Void Interactive, 2023

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "FMODEvent.h"
#include "SoundSourceAnimNotify.generated.h"

UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Anim Notify Sound Source"))
class READYORNOT_API USoundSourceAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	USoundSourceAnimNotify();

	virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *AnimSeq) override;
	virtual FString GetNotifyName_Implementation() const override;

	// If this sound should follow its owner
	UPROPERTY(EditAnywhere, Category = "FMOD Anim Notify")
	uint32 bFollow : 1;

	// Socket or bone name to attach sound to
	UPROPERTY(EditAnywhere, Category = "FMOD Anim Notify", meta = (EditCondition = "bFollow"))
	FString AttachName;

	// Sound to Play
	UPROPERTY(EditAnywhere, Category = "FMOD Anim Notify", BlueprintReadWrite)
	UFMODEvent* Event;
};