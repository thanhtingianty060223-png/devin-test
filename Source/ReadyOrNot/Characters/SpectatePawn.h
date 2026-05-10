// Copyright Void Interactive, 2023

#pragma once

#include "GameFramework/SpectatorPawn.h"
#include "HUD/Widgets/Widgets.h"
#include "SpectatePawn.generated.h"

UCLASS()
class READYORNOT_API ASpectatePawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	ASpectatePawn();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(class AController* NewController) override;
	virtual void UnPossessed() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UCameraComponent* PawnCamera = nullptr;

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:
	UFUNCTION()
	void EscapeMenu();

	UFUNCTION(Client, Reliable)
	void CleanUpOldPlayer();

	UPROPERTY(EditAnywhere)
	bool bDeadSpectatePawn = false;

	bool bNVGOnViewTarget = false;

	UPROPERTY(EditAnywhere, Category = Blend)
	bool bBlendWithViewTarget = false;
	
	UFUNCTION(BlueprintCallable, Category = "ViewTarget")
	void SpectateNextPlayer();

	UFUNCTION(BlueprintCallable, Category = "ViewTarget")
	void SpectatePreviousPlayer();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
			void OnChatPressed();
	virtual void OnChatPressed_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
			void OnTeamChatPressed();
	virtual void OnTeamChatPressed_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Gameplay)
			void CenterPrint(FName Type, float Duration, class APlayerCharacter* Other);
	virtual void CenterPrint_Implementation(FName Type, float Duration, class APlayerCharacter* Other);

	// Sometimes we want to centerprint something before the HUD is created, that's A-OK.
	UPROPERTY(BlueprintReadOnly, Category = Scripting)
	bool bPendingCenterprint = false;

	UPROPERTY(BlueprintReadOnly, Category = Scripting)
	float PendingCenterprintDuration = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = Scripting)
	FName PendingCenterprintType = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = Scripting)
	class APlayerCharacter* PendingCenterprintOther = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Post Process")
	UMaterial* HeadcamMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* HeadcamMaterialInstance;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "No Specator")
	TSubclassOf<UUserWidget> SpectateWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Spectator")
	USpectatorCharacterHUD* SpectatorHUD;

	UFUNCTION(BlueprintCallable, Category = "Spectator")
	void SetSpectatorCharacterWidget(USpectatorCharacterHUD* NewHud) { SpectatorHUD = NewHud; }
	
	UPROPERTY(BlueprintReadWrite, Category = "Specator")
	bool bShouldShowViewTargetHUD;

	UPROPERTY(BlueprintReadOnly, Category = "Spectator")
	AActor* CurrentViewTarget;

	TArray<AReadyOrNotCharacter*> GetCompatibleViewTargets() const;
	
	UFUNCTION(BlueprintCallable, Category = Gameplay)
	ETeamType GetTeam() const;

	UFUNCTION(BlueprintCallable, Category = "Spectator")
	void SetViewTarget(AReadyOrNotCharacter* inCharacter);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spectator")
	bool bHideWidgets = false;

	UPROPERTY(Replicated)
	APlayerCharacter* Killer;

	UPROPERTY(Replicated)
	APlayerCharacter* KilledCharacter;
};
