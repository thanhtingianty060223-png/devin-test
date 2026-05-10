// Copyright Void Interactive, 2017

#pragma once

#include "Animation/SkeletalMeshActor.h"
#include "Pouch.generated.h"
/**
 * 
 */
UCLASS()
class READYORNOT_API APouch : public ASkeletalMeshActor
{
	GENERATED_BODY()

		APouch();

		UPROPERTY(VisibleDefaultsOnly, Category = Gameplay)
		USkeletalMeshComponent* MagazineComponent;

		UPROPERTY(ReplicatedUsing = OnRep_Attach)
			USceneComponent* AttachToComp;

		UPROPERTY(ReplicatedUsing = OnRep_Attach)
			FName AttachToSocket;

		UPROPERTY(ReplicatedUsing = OnRep_UpdateVisibility)
			bool bShowMagazine;


		UPROPERTY(EditAnywhere, Category = Equip)
			FName MagazineSocket;
public:



	void AttachPouch(USceneComponent* SceneComp = nullptr, FName SocketName = NAME_None);



	void ShowMagazine();
	void HideMagazine();
	void SetMesh(USkeletalMesh* NewMesh);

	UFUNCTION()
	void OnRep_UpdateVisibility();

	UFUNCTION()
		void OnRep_Attach();



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
		UAnimSequence* OpenPouch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Animation)
		UAnimSequence* ClosePouch;


	FORCEINLINE class USkeletalMeshComponent* GetMagazineComponent() const { return MagazineComponent; }
	
	
};
