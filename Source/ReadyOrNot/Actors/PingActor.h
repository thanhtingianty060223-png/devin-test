// Void Interactive, 2021

#pragma once

#include "GameFramework/Actor.h"
#include "PingActor.generated.h"

UCLASS()
class READYORNOT_API APingActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APingActor();

	UFUNCTION(BlueprintCallable, Category = "Ping")
	void Setup(AActor* InActor);
	
	UFUNCTION(BlueprintCallable, Category = "Ping")
	void ToggleObjectiveMarkerVisibility() const;

	UFUNCTION(BlueprintCallable, Category = "Ping")
    void ShowObjectiveMarker() const;
	
	UFUNCTION(BlueprintCallable, Category = "Ping")
    void HideObjectiveMarker() const;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ping")
	AActor* PingedActor = nullptr;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SetIconBrush, Category = "Ping")
	FSlateBrush IconBrush;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_SetPingText, Category = "Ping")
	FText PingText;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Ping Actor|Components")
	class UObjectiveMarkerComponent* ObjectiveMarkerComponent = nullptr;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Ping Actor|Components")
	class UMapActorComponent* MapActorComponent;

	UFUNCTION()
	void OnRep_SetIconBrush();
	
	UFUNCTION()
	void OnRep_SetPingText();
};
