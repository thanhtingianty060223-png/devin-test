// Copyright Void Interactive, 2023

#include "Collectable.h"

#include "Engine/AssetManager.h"

ACollectable::ACollectable()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");
}

FPrimaryAssetId ACollectable::GetPrimaryAssetId() const
{
	return FPrimaryAssetId("Collectable", FPackageName::GetShortFName(GetPackage()->GetName()));
}

TArray<ACollectable*> ACollectable::GetAllCollectables()
{
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FAssetData> AssetDatas;
	AssetManager.GetPrimaryAssetDataList("Collectable", AssetDatas);

	TArray<ACollectable*> Collectables;
	for (FAssetData& AssetData : AssetDatas)
	{
		TSubclassOf<ACollectable> Class = AssetManager.GetPrimaryAssetObjectClass<ACollectable>(AssetData.GetPrimaryAssetId());
		if (!Class)
			continue;

		Collectables.Add(Class->GetDefaultObject<ACollectable>());
	}

	return Collectables;
}

#if WITH_EDITOR
void ACollectable::CenterRootChildren()
{
	if (!RootComponent)
		return;

	for (USceneComponent* SceneComponent : RootComponent->GetAttachChildren())
	{
		if (!SceneComponent)
			continue;
		
		FVector Center = SceneComponent->Bounds.GetBox().GetCenter();
		FVector RelativeCenter = Center - SceneComponent->GetComponentLocation();
		
		SceneComponent->SetRelativeLocation(-RelativeCenter);
	}
}
#endif
