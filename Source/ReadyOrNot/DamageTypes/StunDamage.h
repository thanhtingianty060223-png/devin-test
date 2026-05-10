// Copyright Void Interactive, 2017

#pragma once

#include "GameFramework/DamageType.h"
#include "LegacyCameraShake.h"
#include "StunDamage.generated.h"



UENUM(BlueprintType)
enum class EStunType : uint8
{
	ST_None,
	ST_Tased,
	ST_Gassed,
	ST_Flash,
	ST_Stung,
	ST_Beanbag,
	ST_Pepperball,
	ST_Rubberball,
	ST_Pepperspray,
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class READYORNOT_API UStunDamage : public UDamageType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bProjectileStun = false;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	TSubclassOf<ULegacyCameraShake> StunShake;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	FPostProcessSettings PostProcessSettings;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		UAnimMontage* HitMontage_3P;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	FRotator CameraRotationOffset;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		float AppliedSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float MaxMovementSpeedWhenStunned = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float DoorDamageMultiplier = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bDamageAllDoorPiecesAtOnce = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool bCanPushDoorWithForce = false;

	// 0.0f = open door no amount
	// 1.0f = open door fully
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float DoorPushScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = Sound)
	USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	EStunType StunType;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bPlayAudioWhenHit = false;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		class UFMODEvent* StunSoundEffect;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bCauseHealthDamage = false;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bCausesSuppression = false;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bBreaksDestructibles = true;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		float SuppressionAmount;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		TSubclassOf<ULegacyCameraShake> SuppressionCameraShake;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bMustBeLookingAtDamageCauser = false;
	
	UPROPERTY(EditAnywhere, Category = Gameplay)
		bool bStunLocksAim = false;

	UPROPERTY(EditAnywhere, Category = Gameplay)
		float StunSpeedMultiplier = 1.0f;

	// Player stun duration.
	UPROPERTY(EditAnywhere, Category = Gameplay)
		float WeaponDownLengthOnStun = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		bool bNonLethal = false;

	// How much 'LTL' to apply per hit, if 1.0f applied then we play an animation and reset the LTL, rather than applying per hit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		float LessThanLethalAmount = 1.0f;

	// additional damage for this stun damage to apply if hitting a player up close anywhere
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		float AdditionalUpcloseDamageIncrease = 0.0f;

	// additional damage for this stun damage to apply if hitting a player in the head
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		float AdditionalHeadDamageIncrease = 0.0f;

	UFUNCTION(BlueprintCallable)
	virtual void ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	// If true, using this against a SWAT AI counts as abuse
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
		bool bSwatAIIsAbuse = false;

	// If true, using this against a child AI counts as abuse
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
		bool bChildAIIsAbuse = false;

	// If true, using this against a compliant AI counts as abuse
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
		bool bCompliantIsAbuse = false;

	// If true, using this against an arrested AI counts as abuse
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cybernetics)
		bool bArrestedIsAbuse = false;
};
