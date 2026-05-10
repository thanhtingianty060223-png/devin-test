// Copyright Void Interactive, 2021


#include "BaseRadialMenuScript.h"

#include "HUD/Widgets/RadialWidgetBase.h"

#include "UObject/ConstructorHelpers.h"

UBaseRadialMenuScript::UBaseRadialMenuScript(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> T_UnderConstruction(TEXT("Texture2D'/Game/ReadyOrNot/UI/HUD_Revised/UnderConstruction_Icon_256.UnderConstruction_Icon_256'"));

	RadialMenuIcon = T_UnderConstruction.Object;
	World = nullptr;
}

void UBaseRadialMenuScript::Initialize(URadialWidgetBase* InRadialMenuOwner)
{
	RadialMenuOwner = InRadialMenuOwner;
	Actor = RadialMenuOwner->GetOwningPlayerPawn();
	World = GetWorld();
}

void UBaseRadialMenuScript::ExecuteScript_Implementation()
{
}
