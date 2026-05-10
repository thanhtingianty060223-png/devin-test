// Copyright Void Interactive, 2022

#pragma once

#include "PooledActor.h"
#include "BaseShell.generated.h"

UENUM(BlueprintType)
enum class EShellType : uint8
{
	Bullet,
	Shotgun
};

UCLASS()
class READYORNOT_API ABaseShell final : public APooledActor
{
	GENERATED_BODY()
	
public:	
	ABaseShell();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ShellMesh = nullptr;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> MID_ShellMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Shell")
	EShellType ShellType = EShellType::Bullet;
	
	UPROPERTY(VisibleAnywhere, Category = "Sounds")
	UFMODEvent* ShellBounceFMODAudio = nullptr;

	UPROPERTY(EditAnywhere, Category = "Sounds")
	float ShellNormalizeMax;

	UPROPERTY(EditAnywhere, Category = "Sounds")
	float ShellNormalizeMin;
	
	FORCEINLINE UStaticMeshComponent* GetShellMesh() const { return ShellMesh; }

	void PropelFromGun(class ABaseMagazineWeapon* Weapon);
	
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
protected:
	virtual void BeginPlay() override;

	void StopPhysics();
	void DisableWeaponFOV();

	FFMODEventInstance ShellHitEventInst;
};
