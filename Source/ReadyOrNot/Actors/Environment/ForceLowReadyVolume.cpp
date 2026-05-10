// Copyright Void Interactive, 2023

#include "ForceLowReadyVolume.h"

AForceLowReadyVolume::AForceLowReadyVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	bColored = true;
	BrushColor = FColor(255, 0, 110);
	
	GetBrushComponent()->SetCollisionObjectType(ECC_VOLUME);
	GetBrushComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UTexture2D> Icon(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/T_LowReady.T_LowReady'"));
	
	BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>("BillboardComponent");
	if (BillboardComponent)
	{
		BillboardComponent->SetSprite(Icon.Object);
		BillboardComponent->SetRelativeScale3D_Direct(FVector(0.5f, 0.5f, 0.5f));
		BillboardComponent->SetIsVisualizationComponent(true);
		BillboardComponent->bIsScreenSizeScaled = true;
		BillboardComponent->bUseInEditorScaling = true;
		BillboardComponent->SetupAttachment(RootComponent);
	}
#endif
}



