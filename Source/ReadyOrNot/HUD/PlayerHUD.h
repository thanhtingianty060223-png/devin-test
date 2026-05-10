// Copyright Void Interactive, 2017

#pragma once

#include "Characters/PlayerCharacter.h"
#include "Widgets/Widgets.h"
#include "Components/CanvasPanel.h"
#include "PlayerHUD.generated.h"


/**
 * 
 */
UCLASS()
class READYORNOT_API APlayerHUD : public AHUD
{
	GENERATED_BODY()
	
	APlayerHUD();

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

	UFUNCTION(BlueprintCallable, Category = Widget)
	static void SetWidgetTranslationByMouseDelta(APlayerController* Controller, UUserWidget* Widget, float DeltaSeconds, float InterpSpeed = 1.0f, float InputScale = 1.0f, float ClampAt = 0.0f);

	UFUNCTION(BlueprintCallable, Category = Widget)
		static void SetCanvasTranslationByMouseDelta(APlayerController* Controller, UCanvasPanel* Widget, float DeltaSeconds, float InterpSpeed = 1.0f, float InputScale = 1.0f, float ClampAt = 0.0f);
	
public:

	UPROPERTY(BlueprintReadOnly, Category = Widgets)
		UUserWidget* PlayerHUD;
};
