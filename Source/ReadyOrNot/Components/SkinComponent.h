// Copyright Void Interactive, 2021

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enums.h"
#include "SkinComponent.generated.h"


UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class READYORNOT_API USkinComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USkinComponent();

	UFUNCTION(BlueprintCallable, Category = "Skin Component")
	void ResetSkin();

	UFUNCTION(BlueprintPure, Category = "Skin Component")
	bool HasDLCUnlocked();

	UFUNCTION(BlueprintPure, Category = "Skin Component")
	static UTexture2D* GetClassDefaultIcon(TSubclassOf<USkinComponent> SkinComponent);
	
	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool bNoReset = false;
	void ApplySkin();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
		FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
		FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
		TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
		bool bRequiresDLC;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description, meta = (EditCondition = "bRequiresDLC"))
		EGameVersionRestriction DLC;

	// if this is true then equipping this skin resets the skin to default, removes all other skins and deletes itself
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
		bool bResetsToFactorySkin = false;

	// if set to anything but none then this will only be useable as a certain team in PVP
	// all skins always available in COOP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
		ETeamType LockedToTeam = ETeamType::TT_NONE;

	// if this is not empty then these skins will be locked to a particular blueprint class
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Description)
		TArray<TSubclassOf<AActor>> LockedToBlueprint;

	UPROPERTY(EditAnywhere, Category = "Skin")
	TMap<USkeletalMesh*, USkeletalMesh*> SkeletalMeshSkinMap;

	UPROPERTY()
	TMap<USkeletalMeshComponent*, USkeletalMesh*> PreAppliedSkeletalMeshMap;

	UPROPERTY(EditAnywhere, Category = "Skin")
	TMap<UStaticMesh*, UStaticMesh*> StaticMeshSkinMap;

	UPROPERTY()
	TMap<UStaticMeshComponent*, UStaticMesh*> PreAppliedStaticMeshMap;

	UFUNCTION(BlueprintPure, Category = "Skin Component")
	bool IsFactorySkin();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
