// Copyright Void Interactive, 2017

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Animation/AnimSequence.h"
#include "AsyncLoader.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API UAsyncLoader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = Loading)
	static UStaticMesh* GetLazyLoadedMesh(TSoftObjectPtr<UStaticMesh> Mesh);
	UFUNCTION(BlueprintPure, Category = Loading)
	static USkeletalMesh* GetLazyLoadedSkeletalMesh(TSoftObjectPtr<USkeletalMesh> Mesh);
	UFUNCTION(BlueprintPure, Category = Loading)
	static USoundCue* GetLazyLoadedSoundCue(TSoftObjectPtr<USoundCue> Cue);
	UFUNCTION(BlueprintPure, Category = Loading)
	static UObject* GetLazyLoadedObject(TSoftObjectPtr<UObject> Object);
	UFUNCTION(BlueprintPure, Category = Loading)
	static UClass* GetLazyLoadedClass(TSoftClassPtr<UClass> Class);
	UFUNCTION(BlueprintPure, Category = Loading)
	static UMaterialInstance* GetLazyLoadedMaterialInstance(TSoftObjectPtr<UMaterialInstance> Material);
	UFUNCTION(BlueprintPure, Category = Loading)
	static UParticleSystem* GetLazyLoadedParticleSystem(TSoftObjectPtr<UParticleSystem> Particle);
	UFUNCTION(BlueprintPure, Category = Loading)
	static UAnimMontage* GetLazyLoadedAnimMontage(TSoftObjectPtr<UAnimMontage> Montage);
	UFUNCTION(BlueprintPure, Category = Loading)
	static UAnimSequence* GetLazyLoadedAnimSequence(TSoftObjectPtr<UAnimSequence> Anim);
	UFUNCTION(BlueprintPure, Category = Loading)
		static TSubclassOf<UUserWidget> GetLazyLoadedWidget(TSoftClassPtr<UUserWidget> Widget);
	UFUNCTION(BlueprintPure, Category = Loading)
		static UTexture2D* GetLazyLoadedImage(TSoftObjectPtr<UTexture2D> Texture);
	UFUNCTION(BlueprintPure, Category = Loading)
		static TSubclassOf<ABaseItem> GetLazyLoadedItem(TSoftClassPtr<ABaseItem> Item);
	UFUNCTION(BlueprintPure, Category = Loading)
		static TSubclassOf<ABaseWeapon> GetLazyLoadedWeapon(TSoftClassPtr<ABaseWeapon> Item);
	UFUNCTION(BlueprintPure, Category = Loading)
	static TArray<TSubclassOf<UClass>> GetLazyLoadedClassArray(TArray<TSoftClassPtr<UClass>> Array);
	UFUNCTION(BlueprintPure, Category = Loading)
		static TArray<TSubclassOf<class AReadyOrNotGameMode>> GetLazyLoadedReadyOrNotGameModeArray(TArray<TSoftClassPtr<class AReadyOrNotGameMode>> Array);


	
	
};
