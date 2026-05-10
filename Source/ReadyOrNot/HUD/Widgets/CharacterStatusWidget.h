// Copyright Void Interactive, 2023

#pragma once

#include "CoreMinimal.h"
#include "Commander/RosterManager.h"
#include "CharacterStatusWidget.generated.h"

UCLASS()
class UCharacterStatusWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	const TArray<class UCharacterProxy*>& GetCharacterProxies() { return CharacterProxies; }

	UFUNCTION(BlueprintImplementableEvent)
	void OnCharacterAdded(class UCharacterProxy* CharacterProxy);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnCharacterRemoved(class UCharacterProxy* CharacterProxy);

private:
	UPROPERTY(Transient)
	TArray<class UCharacterProxy*> CharacterProxies;
	
	void HandlePlayerAdded(APlayerState* PlayerState);
	void HandlePlayerRemoved(APlayerState* PlayerState);
};

/**
 * 
 */
UCLASS(BlueprintType)
class READYORNOT_API UCharacterProxy : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FText GetName() { return FText(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FText GetFirstName() { return FText(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual ETeamType GetTeam() { return ETeamType::TT_NONE; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual int32 GetNumber() { return 0; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FText GetStatus() { return FText(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual float GetHealth() { return 0.0f; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual FRosterLoadout GetLoadout() { return FRosterLoadout(); }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual class URosterTrait* GetTrait(bool& bIsUnlocked) { bIsUnlocked = false; return nullptr; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual TSoftObjectPtr<UTexture2D> GetImage() { return nullptr; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual bool IsPlayer() { return false; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	virtual bool IsLocalPlayer() { return false; }
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterUpdated);
	
	UPROPERTY(BlueprintAssignable)
	FOnCharacterUpdated OnStatusUpdated;

	UPROPERTY(BlueprintAssignable)
	FOnCharacterUpdated OnLoadoutUpdated;

protected:
	static FText StatusFromCharacter(AReadyOrNotCharacter* Character);
	static float HealthFromCharacter(AReadyOrNotCharacter* Character);
	static FRosterLoadout LoadoutFromCharacter(AReadyOrNotCharacter* Character);
};

UCLASS(NotBlueprintType)
class READYORNOT_API UPlayerCharacterProxy : public UCharacterProxy
{
	GENERATED_BODY()

public:
	virtual FText GetName() override;
	virtual ETeamType GetTeam() override;
	virtual int32 GetNumber() override;
	virtual FText GetStatus() override;
	virtual float GetHealth() override;
	virtual FRosterLoadout GetLoadout() override;
	virtual bool IsPlayer() override;
	virtual bool IsLocalPlayer() override;
	
	UPROPERTY()
	class APlayerState* PlayerState;
};

UCLASS(NotBlueprintType)
class READYORNOT_API UCoopCharacterProxy : public UCharacterProxy
{
	GENERATED_BODY()

public:
	virtual FText GetName() override;
	virtual FText GetFirstName() override;
	virtual ETeamType GetTeam() override;
	virtual FText GetStatus() override;
	virtual float GetHealth() override;
	virtual FRosterLoadout GetLoadout() override;
	virtual class URosterTrait* GetTrait(bool& bIsUnlocked) override;
	virtual TSoftObjectPtr<UTexture2D> GetImage() override;
	
	UPROPERTY()
	class ASWATCharacter* SwatCharacter;
};




