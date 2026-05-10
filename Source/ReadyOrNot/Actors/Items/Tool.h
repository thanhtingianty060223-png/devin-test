// Void Interactive, 2020

#pragma once

#include "Actors/BaseItem.h"
#include "Tool.generated.h"

/**
 * 
 */
UCLASS()
class READYORNOT_API ATool : public ABaseItem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void StartUsingTool(AActor* Target);
	UFUNCTION(BlueprintCallable)
	void StopUsingTool(AActor* Target);

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void Server_ToolComplete();
	
	UFUNCTION(BlueprintPure, Category = "Multitool")
	FORCEINLINE bool IsOperating() const { return bOperating; }
	
	UFUNCTION(BlueprintPure, Category = "Multitool")
	FORCEINLINE float GetCurrentOperatingTime() const { return CurrentOperatingTime; }
	
	UFUNCTION(BlueprintPure, Category = "Multitool")
	FORCEINLINE float GetMaxOperatingTime() const { return MaxOperatingTime; }
	
protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(Server, Reliable, WithValidation)
			void Server_StartUsingTool(AActor* Target);
	virtual void Server_StartUsingTool_Implementation(AActor* Target);
	virtual bool Server_StartUsingTool_Validate(AActor* Target) { return true; }
	
	UFUNCTION(Server, Reliable, WithValidation)
			void Server_StopUsingTool(AActor* Target);
	virtual void Server_StopUsingTool_Implementation(AActor* Target);
	virtual bool Server_StopUsingTool_Validate(AActor* Target) { return true; }
	
	virtual void Server_ToolComplete_Implementation();
	virtual bool Server_ToolComplete_Validate() { return true; }

	UFUNCTION(Client, Reliable)
			void Client_FinishedToolUse(AActor* Target, class APlayerCharacter* pc);
	virtual void Client_FinishedToolUse_Implementation(AActor* Target, class APlayerCharacter* pc);

	UFUNCTION(Client, Reliable)
			void Client_StopToolAnimation();
	virtual void Client_StopToolAnimation_Implementation();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Setup")
	float MaxOperatingTime = 1.0f;

	UPROPERTY(BlueprintReadOnly, Replicated)
	float CurrentOperatingTime = 0.0f;
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	uint8 bOperating : 1;
	
	UPROPERTY(BlueprintReadOnly, Replicated)
	AActor* TargetActor = nullptr;
};
