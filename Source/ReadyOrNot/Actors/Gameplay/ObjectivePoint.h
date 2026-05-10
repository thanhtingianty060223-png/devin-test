// Void Interactive, 2021

#pragma once

#include "GameFramework/Actor.h"
#include "ObjectivePoint.generated.h"

UCLASS()
class READYORNOT_API AObjectivePoint : public AActor
{
	GENERATED_BODY()
	
public:	
	AObjectivePoint();

	UFUNCTION(BlueprintCallable, Category = "Objective Point")
	void InitSettings(FSlateBrush Icon, FText Text, float ShowMarkerAtDistance = 100.0f);
	
	UFUNCTION(BlueprintCallable, Category = "Objective Point")
	void ToggleObjectiveMarkerVisibility();

	UFUNCTION(BlueprintCallable, Category = "Objective Point")
    void ShowObjectiveMarker();
	
	UFUNCTION(BlueprintCallable, Category = "Objective Point")
    void HideObjectiveMarker();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective Point")
	AActor* TiedToActor = nullptr;

protected:
	void BeginPlay() override;
	void Tick(float DeltaTime) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Objective Point")
	class UObjectiveMarkerComponent* ObjectiveMarkerComponent = nullptr;
	
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Objective Point")
	class UMapActorComponent* MapActorComponent;
};
