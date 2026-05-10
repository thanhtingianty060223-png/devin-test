// Void Interactive, 2020

#pragma once

#include "Components/SceneComponent.h"
#include "MapActorComponent.generated.h"

UCLASS(ClassGroup=(Map), HideCategories=("Activation", "Rendering", "Cooking", "Physics", "LOD", "Assest User Data", "Collision"), meta=(BlueprintSpawnableComponent))
class READYORNOT_API UMapActorComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	UMapActorComponent();
	

	void InitializeMapActorSettings(const FSlateBrush& InIconBrush, const FLinearColor& InIconColor = FColor::White, const FText& InIconText = FText::FromString(""), const FLinearColor& InIconTextColor = FColor::White);
	
	UFUNCTION(BlueprintCallable, Category = "Map Actor")
	void SetIconText(const FText& InIconText);
	
	UFUNCTION(BlueprintCallable, Category = "Map Actor")
	void SetIconTextColor(const FLinearColor& InIconTextColor);
	
	UFUNCTION(BlueprintCallable, Category = "Map Actor")
	void SetIconColor(const FLinearColor& InIconColor);
	
	UFUNCTION(BlueprintCallable, Category = "Map Actor")
	void EnableMapActor();

	UFUNCTION(BlueprintCallable, Category = "Map Actor")
	void DisableMapActor();
	
	UFUNCTION(BlueprintPure, Category = "Map Actor Component")
	FORCEINLINE bool IsUsingActorRotation() const { return bUseActorRotation; }
	
	UFUNCTION(BlueprintPure, Category = "Map Actor Component")
	FORCEINLINE FText GetIconText() const { return IconText; }

	UPROPERTY(BlueprintReadWrite, Category = "Map Actor Component")
	uint8 bCondition : 1;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	void AddToMap(UHumanCharacterHUD_V2* HUD);
	void RemoveFromMap(UHumanCharacterHUD_V2* HUD);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	uint8 bEnabled : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (EditCondition="bEnabled"))
	TSubclassOf<class UMapActorWidget> MapActorWidgetClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition="bEnabled"))
	FText IconText;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition="bEnabled"))
    FLinearColor IconTextColor = FColor::White;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition="bEnabled"))
    FSlateBrush IconBrush;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition="bEnabled"))
    FLinearColor IconColor = FColor::White;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition="bEnabled"))
    uint8 bUseActorRotation : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition="bEnabled && bUseActorRotation"))
	float RotationOffset = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Data")
	bool bAddedToMap = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	class UMapActorIconWidget* MapIconWidget = nullptr;
};
