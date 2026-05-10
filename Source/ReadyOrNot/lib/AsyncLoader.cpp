// Copyright Void Interactive, 2017

#include "AsyncLoader.h"
#include "ReadyOrNot.h"
#include "ReadyOrNotGameMode.h"




UStaticMesh* UAsyncLoader::GetLazyLoadedMesh(TSoftObjectPtr<UStaticMesh> Mesh)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Mesh.LoadSynchronous());
	}
	return Mesh.LoadSynchronous();
}

USkeletalMesh* UAsyncLoader::GetLazyLoadedSkeletalMesh(TSoftObjectPtr<USkeletalMesh> Mesh)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Mesh.LoadSynchronous());
	}
	return Mesh.LoadSynchronous();
}

USoundCue* UAsyncLoader::GetLazyLoadedSoundCue(TSoftObjectPtr<USoundCue> Cue)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Cue.LoadSynchronous());
	}
	return Cue.LoadSynchronous();
}

UObject* UAsyncLoader::GetLazyLoadedObject(TSoftObjectPtr<UObject> Object)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Object.LoadSynchronous());
	}
	return Object.LoadSynchronous();
}

UClass* UAsyncLoader::GetLazyLoadedClass(TSoftClassPtr<UClass> Class)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Class.LoadSynchronous());
	}
	return Class.LoadSynchronous();
}

UMaterialInstance* UAsyncLoader::GetLazyLoadedMaterialInstance(TSoftObjectPtr<UMaterialInstance> Material)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Material.LoadSynchronous());
	}
	return Material.LoadSynchronous();
}

UParticleSystem* UAsyncLoader::GetLazyLoadedParticleSystem(TSoftObjectPtr<UParticleSystem> Particle)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Particle.LoadSynchronous());
	}
	return Particle.LoadSynchronous();
}

UAnimMontage* UAsyncLoader::GetLazyLoadedAnimMontage(TSoftObjectPtr<UAnimMontage> Montage)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Montage.LoadSynchronous());
	}
	return Montage.LoadSynchronous();
}

UAnimSequence* UAsyncLoader::GetLazyLoadedAnimSequence(TSoftObjectPtr<UAnimSequence> Anim)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Anim.LoadSynchronous());
	}
	return Anim.LoadSynchronous();
}

TSubclassOf<UUserWidget> UAsyncLoader::GetLazyLoadedWidget(TSoftClassPtr<UUserWidget> Widget)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Widget.LoadSynchronous());
	}
	return Widget.LoadSynchronous();
}

UTexture2D* UAsyncLoader::GetLazyLoadedImage(TSoftObjectPtr<UTexture2D> Texture)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedObjects.AddUnique(Texture.LoadSynchronous());
	}
	return Texture.LoadSynchronous();
}

TSubclassOf<ABaseItem> UAsyncLoader::GetLazyLoadedItem(TSoftClassPtr<ABaseItem> Item)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedClasses.AddUnique(Item.LoadSynchronous());
	}
	return Item.LoadSynchronous();
}

TSubclassOf<ABaseWeapon> UAsyncLoader::GetLazyLoadedWeapon(TSoftClassPtr<ABaseWeapon> Item)
{
	UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
	if (GameInstance)
	{
		GameInstance->LazyLoadedClasses.AddUnique(Item.LoadSynchronous());
	}
	return Item.LoadSynchronous();
}

TArray<TSubclassOf<UClass>> UAsyncLoader::GetLazyLoadedClassArray(TArray<TSoftClassPtr<UClass>> Array)
{
	TArray<TSubclassOf<UClass>> OutClass;
	for (TSoftClassPtr<UClass> Class : Array)
	{
		UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
		if (GameInstance)
		{
			GameInstance->LazyLoadedClasses.AddUnique(Class.LoadSynchronous());
		}
		OutClass.Add(Class.LoadSynchronous());
	}
	return OutClass;
}

TArray<TSubclassOf<AReadyOrNotGameMode>> UAsyncLoader::GetLazyLoadedReadyOrNotGameModeArray(TArray<TSoftClassPtr<AReadyOrNotGameMode>> Array)
{
	TArray<TSubclassOf<AReadyOrNotGameMode>> OutClass;
	for (TSoftClassPtr<AReadyOrNotGameMode> Class : Array)
	{
		UReadyOrNotGameInstance* GameInstance = UReadyOrNotStatics::GetReadyOrNotGameInstance();
		if (GameInstance)
		{
			GameInstance->LazyLoadedClasses.AddUnique(Class.LoadSynchronous());
		}
		OutClass.Add(Class.LoadSynchronous());
	}
	return OutClass;
}
