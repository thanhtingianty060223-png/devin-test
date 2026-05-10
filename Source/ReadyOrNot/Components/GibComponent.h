// Copyright Void Interactive, 2022

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GibComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGibDelegate, FName /** Bone */)

UENUM(BlueprintType)
enum class EGibAreas : uint8
{
	GA_None,
	GA_LeftLeg,
	GA_RightLeg,
	GA_LeftArm,
	GA_RightArm,
	GA_Head
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UGibComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UGibComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Gib(EGibAreas GibArea, FVector Impulse = FVector::ZeroVector);

	FORCEINLINE bool IsLimbGibbed(EGibAreas GibAreas) { return GibbedAreas.Contains(GibAreas); }
	FORCEINLINE bool IsAnyLimbGibbed() { return GibbedAreas.Num() != 0; }
	bool IsBoneInGibArea(FName Bone, EGibAreas GibArea);
	
	UStaticMesh* GetGibMesh(EGibAreas GibAreas);
	UStaticMesh* GetBoneMesh(EGibAreas GibAreas);
	FName GetGibAttachSocket(EGibAreas GibAreas);
	FName GetBoneAttachSocket(EGibAreas GibAreas);
	FName GetConstraintName(EGibAreas GibAreas);
	FString GetHideBone(EGibAreas GibAreas);
	FString GetGibBone(EGibAreas GibAreas);

	FORCEINLINE void SetBodyMesh(USkeletalMeshComponent* NewSkeletalMesh) { BodyMesh = NewSkeletalMesh; }
	FORCEINLINE void SetFaceMesh(USkeletalMeshComponent* NewSkeletalMesh) { FaceMesh = NewSkeletalMesh;}

	FOnGibDelegate OnGib;
	
protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<EGibAreas> GibbedAreas;

	UPROPERTY(EditAnywhere)
	class UBloodData* BloodData;
	
	UPROPERTY()
	USkeletalMeshComponent* BodyMesh;
	
	UPROPERTY()
	USkeletalMeshComponent* FaceMesh;

	UPROPERTY()
	UStaticMesh* GibHead;
	
	UPROPERTY()
	UStaticMesh* GibArms;

	UPROPERTY()
	UStaticMesh* GibLegs;

	UPROPERTY()
	UStaticMesh* BoneHead;
	
	UPROPERTY()
	UStaticMesh* BoneArms;
	
	UPROPERTY()
	UStaticMesh* BoneLegs;
};
