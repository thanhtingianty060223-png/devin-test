// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "ActivityTriggerVolume.h"
#include "CheckpointActivityTriggerVolume.generated.h"

UCLASS()
class READYORNOT_API ACheckpointActivityTriggerVolume : public AActivityTriggerVolume
{
	GENERATED_BODY()

public:
	ACheckpointActivityTriggerVolume();

	virtual void Activate() override;

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	class UArrowComponent* ArrowComponent;

public:
	/** Returns ArrowComponent subobject **/
	class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
#endif
};
