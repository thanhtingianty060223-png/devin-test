// Copyright Void Interactive, 2022

#pragma once

#include "GameFramework/Actor.h"
#include "UObjectListener.generated.h"

UCLASS(NotBlueprintable)
class READYORNOT_API AUObjectListener final : public AActor, public FUObjectArray::FUObjectCreateListener, public FUObjectArray::FUObjectDeleteListener
{
	GENERATED_BODY()

public:
	AUObjectListener();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void NotifyUObjectCreated(const UObjectBase* Object, int32 Index) override;
	virtual void NotifyUObjectDeleted(const UObjectBase* Object, int32 Index) override;
	virtual void OnUObjectArrayShutdown() override;
};
