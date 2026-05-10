// Copyright Void Interactive, 2022

#include "UObjectListener.h"

AUObjectListener::AUObjectListener()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AUObjectListener::BeginPlay()
{
	Super::BeginPlay();

	GUObjectArray.AddUObjectCreateListener(this);
	GUObjectArray.AddUObjectDeleteListener(this);
}

void AUObjectListener::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GUObjectArray.RemoveUObjectCreateListener(this);
	GUObjectArray.RemoveUObjectDeleteListener(this);
}

void AUObjectListener::NotifyUObjectCreated(const UObjectBase* Object, int32 Index)
{
	ULog::Info("Created: " + Object->GetFName().ToString());
}

void AUObjectListener::NotifyUObjectDeleted(const UObjectBase* Object, int32 Index)
{
	if (Object->GetClass())
		ULog::Info("Deleted: " + GetNameSafe(Object->GetClass()));
	
	// Always outputs "None" for some reason
	//ULog::Info("Deleted: " + Object->GetFName().ToString());
}

void AUObjectListener::OnUObjectArrayShutdown()
{
}
