// Copyright Void Interactive, 2023


#include "GasNavLinkGenerator.h"

#include "NavigationSystem.h"
#include "NavLinkRenderingComponent.h"
#include "Navigation/NavLinkProxy.h"
#include "Navigation/ReadyOrNotNavAreas.h"
#include "Sound/PortalVolume.h"


AGasNavLinkGenerator::AGasNavLinkGenerator()
{
	PrimaryActorTick.bCanEverTick = true;

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("Billboard");
}

void AGasNavLinkGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGasNavLinkGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGasNavLinkGenerator::GenerateNavLinks()
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
		return;
	
	for (TActorIterator<APortalVolume> It(GetWorld()); It; ++It)
	{
		APortalVolume* Volume = *It;
		if (!IsValid(Volume))
			continue;

		if (Volume->AttachedObjects.Num())
			continue;

		float PathLength = 0;
		const FVector PathStart = Volume->GetActorLocation() + (Volume->GetActorForwardVector() * 100);
		const FVector PathEnd = Volume->GetActorLocation() + (Volume->GetActorForwardVector() * -100);
		FNavLocation PathStartProjected, PathEndProjected;
		bool bProjectionSuccess = false;
		bProjectionSuccess = NavSys->ProjectPointToNavigation(PathStart, PathStartProjected, FVector(50, 50, 250));
		bProjectionSuccess &= NavSys->ProjectPointToNavigation(PathEnd, PathEndProjected, FVector(50, 50, 250));

		if (!bProjectionSuccess)
			continue;
		
		NavSys->GetPathLength(this, PathStartProjected, PathEndProjected, PathLength);

		if (PathLength <= 450 && PathLength != 0)
			continue;

		ANavLinkProxy* NavLinkProxy = GetWorld()->SpawnActor<ANavLinkProxy>(ANavLinkProxy::StaticClass(), Volume->GetTransform());
		NavLinkProxy->PointLinks.Reset();

		FNavigationLink NavigationLink = FNavigationLink(FVector(100, 0, 0), FVector(-100, 0, 0));
		NavigationLink.Direction = ENavLinkDirection::BothWays;

		NavigationLink.SetAreaClass(UNavArea_CSGas::StaticClass());
		NavigationLink.LeftProjectHeight = 500;
		NavigationLink.MaxFallDownLength = 500;
		NavigationLink.SnapRadius = 50;

		NavLinkProxy->SetSmartLinkEnabled(false);
		NavLinkProxy->PointLinks.Add(NavigationLink);

		
		if (NavSys)
		{
			NavSys->UpdateActorInNavOctree(*NavLinkProxy);
		}

		for (FNavigationLink& Link : NavLinkProxy->PointLinks)
		{
			Link.InitializeAreaClass(/*bForceRefresh=*/true);
		}


#if WITH_EDITORONLY_DATA
		// Little hack to update the rendering component
		if (UNavLinkRenderingComponent* RenderingComponent = NavLinkProxy->GetEdRenderComp())
		{
			RenderingComponent->SetRelativeLocation(FVector(0, 0, 1));
		}
#endif
	}
}
