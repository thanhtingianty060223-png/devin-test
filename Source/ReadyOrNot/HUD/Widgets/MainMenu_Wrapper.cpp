// Copyright Void Interactive, 2023


#include "HUD/Widgets/MainMenu_Wrapper.h"

#include "CommonUISubsystemBase.h"
#include "ToolBuilderUtil.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Slate/WidgetRenderer.h"

void UMainMenu_Wrapper::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// UTexture2D* Texture = RenderToMesh(this, FVector2D(1.0f, 1.0f));
	// TActorIterator< AStaticMeshActor > ActorItr = TActorIterator< AStaticMeshActor >(GetWorld());
	// for (AStaticMeshActor Actor : ActorItr)
	// {
	// 	if(Actor.GetName().Equals("PlaneMainMenu"))
	// 	{
	// 		Actor.GetStaticMeshComponent()->SetMaterial(0, Texture.materi);
	// 		UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), Actor.GetStaticMeshComponent(), Texture);
	// 	}
	// 	
	// }
} 

void UMainMenu_Wrapper::OpenModMenu()
{
	// ##UE5UPGRADE## CommonUI
	//UCommonUISubsystemBase::bDisableVirtualAccept = true; // causes compile error "bDisableVirtaulAccept" does not exist
}

void UMainMenu_Wrapper::CloseModMenu()
{
	// ##UE5UPGRADE## CommonUI
	// UCommonUISubsystemBase::bDisableVirtualAccept = false; // causes compile error "bDisableVirtaulAccept" does not exist
}
