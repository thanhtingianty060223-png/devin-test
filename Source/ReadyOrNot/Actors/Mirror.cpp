// Void Interactive, 2020

#include "Actors/Mirror.h"

#include "Components/PlanarReflectionComponent.h"

#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

DECLARE_CYCLE_STAT(TEXT("RoN ~ Mirror Tick"), STAT_MirrorTick, STATGROUP_Mirror);

AMirror::AMirror()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	#if WITH_EDITOR
	PrimaryActorTick.TickInterval = 0.0167f;
	#else
	PrimaryActorTick.TickInterval = 0.25f;
	#endif
	PrimaryActorTick.TickGroup = TG_PostPhysics;

#ifdef PLANAR_REFLECTION
	GetPlanarReflectionComponent()->NormalDistortionStrength = 0.0f;
	GetPlanarReflectionComponent()->PrefilterRoughness = 0.0f;
	GetPlanarReflectionComponent()->DistanceFromPlaneFadeoutStart = 60.0f;
	GetPlanarReflectionComponent()->DistanceFromPlaneFadeoutEnd = 0.0f;
	GetPlanarReflectionComponent()->AngleFromPlaneFadeStart = 90.0f;
	GetPlanarReflectionComponent()->AngleFromPlaneFadeEnd = 90.0f;

	GetPlanarReflectionComponent()->ScreenPercentage = 100.0f;
	GetPlanarReflectionComponent()->bRenderSceneTwoSided = false;

	VisibilityBoundsExtent = FVector(2500.0f, 2500.0f, 200.0f);
#endif
} 

void AMirror::BeginPlay()
{
	Super::BeginPlay();

	// ShowOnlyList experiment
	//TArray<FHitResult> BoxTests;
	//UKismetSystemLibrary::BoxTraceMultiForObjects(GetWorld(), GetActorLocation(), GetActorLocation() + GetPlanarReflectionComponent()->GetUpVector() * 2500.0f, FVector(2500.0f, 2500.0f, 2500.0f), GetPlanarReflectionComponent()->GetUpVector().Rotation(), {UEngineTypes::ConvertToObjectType(ECC_WorldStatic), 
	//UEngineTypes::ConvertToObjectType(ECC_WorldDynamic), UEngineTypes::ConvertToObjectType(ECC_DOOR), UEngineTypes::ConvertToObjectType(ECC_Pawn)}, true, {}, EDrawDebugTrace::Persistent, BoxTests, true);

	//for (FHitResult Hit : BoxTests)
	//{
	//	GetPlanarReflectionComponent()->ShowOnlyActors.AddUnique(Hit.GetActor());
	//}
}

void AMirror::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#ifdef PLANAR_REFLECTION
	if (!GetWorld() || GetWorld() && GetWorld()->bIsTearingDown)
		return;
	
	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		EditorTick(DeltaSeconds);
		return;
	}
	#endif

	SCOPE_CYCLE_COUNTER(STAT_MirrorTick);

	bool bMirrorReflectionEnabled;
	UBpGameplayHelperLib::LoadMirrorReflectionEnabled(bMirrorReflectionEnabled);

	bool bMirrorEnabledOnlyInLobby;
	UBpGameplayHelperLib::LoadMirrorEnabledOnlyInLobby(bMirrorEnabledOnlyInLobby);

	// Don't bother changing the state, if already disabled
	if (bMirrorReflectionEnabled)
		bMirrorReflectionEnabled = (bMirrorEnabledOnlyInLobby ? UReadyOrNotFunctionLibrary::IsInLobby() : bMirrorReflectionEnabled);

	if (LOCAL_PLAYER)
	{
		// If 0 = means infinite extent
		if (VisibilityBoundsExtent != FVector::ZeroVector)
		{
			FTransform BoundsTransformTest;
			BoundsTransformTest.SetLocation(GetActorLocation() + VisibilityBoundsTransform.GetLocation());
			BoundsTransformTest.SetRotation(VisibilityBoundsTransform.GetRotation());
			BoundsTransformTest.SetScale3D(VisibilityBoundsTransform.GetScale3D());
			
			const bool bInBounds = UKismetMathLibrary::IsPointInBoxWithTransform(LocalPlayer->GetActorLocation(), BoundsTransformTest, VisibilityBoundsExtent);

			#if WITH_EDITORONLY_DATA
			if (bDrawVisibilityBounds)
				DrawDebugBox(GetWorld(), GetActorLocation() + VisibilityBoundsTransform.GetLocation(), VisibilityBoundsExtent, VisibilityBoundsTransform.GetRotation(), bInBounds ? FColor::Green : FColor::Red, false, DeltaSeconds + 0.02f, 0, 1.5f);
			//DrawDebugSphere(GetWorld(), LocalPlayer->GetActorLocation() + LocalPlayer->GetActorForwardVector() * 25.0f, 10.0f, 4, FColor::Green, false, DeltaSeconds + 0.02f, 0, 1.5f);
			#endif

			if (!bInBounds)
			{
				GetPlanarReflectionComponent()->SetVisibility(false);
				GetPlanarReflectionComponent()->SetComponentTickEnabled(false);
				
				return;
			}
		}
	}

	bool bMirrorAAEnabled;
	UBpGameplayHelperLib::LoadMirrorAntiAliasEnabled(bMirrorAAEnabled);
	
	bool bMirrorDecalsEnabled;
	UBpGameplayHelperLib::LoadMirrorDecalsEnabled(bMirrorDecalsEnabled);

	bool bMirrorDynamicShadowsEnabled;
	UBpGameplayHelperLib::LoadMirrorDynamicShadowsEnabled(bMirrorDynamicShadowsEnabled);
	
	GetPlanarReflectionComponent()->ShowFlags.SetAntiAliasing(bMirrorAAEnabled);
	GetPlanarReflectionComponent()->ShowFlags.SetDecals(bMirrorDecalsEnabled);
	GetPlanarReflectionComponent()->ShowFlags.SetDynamicShadows(bMirrorDynamicShadowsEnabled);
	GetPlanarReflectionComponent()->ShowFlags.SetSubsurfaceScattering(false);
	GetPlanarReflectionComponent()->ShowFlags.SetGrain(false);
	GetPlanarReflectionComponent()->ShowFlags.SetVignette(false);
	GetPlanarReflectionComponent()->ShowFlags.SetEyeAdaptation(false);
	GetPlanarReflectionComponent()->ShowFlags.SetLensFlares(false);
	GetPlanarReflectionComponent()->ShowFlags.SetDepthOfField(false);

	if (bDynamicShadowsDisabled)
	{
		GetPlanarReflectionComponent()->ShowFlags.SetDynamicShadows(false);
	}

	// ShowOnlyList experiment
	//GetPlanarReflectionComponent()->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	// Note(Ali): MirrorFPS does not affect the capture rate, it's tied 1:1 with the current frame rate
	// FRendererModule::BeginRenderingViewFamily line 3994
	//float MirrorFPS;
	//UBpGameplayHelperLib::LoadMirrorFPS(MirrorFPS);
	
	float MirrorResScale;
	UBpGameplayHelperLib::LoadMirrorResolutionScale(MirrorResScale);
	
	GetPlanarReflectionComponent()->SetVisibility(bMirrorReflectionEnabled);
	GetPlanarReflectionComponent()->SetComponentTickEnabled(bMirrorReflectionEnabled);
	//GetPlanarReflectionComponent()->SetComponentTickInterval(1.0f/MirrorFPS);
	GetPlanarReflectionComponent()->ScreenPercentage = MirrorResScale*100.0f;

	for (TActorIterator<APlayerCharacter> It(GetWorld()); It; ++It)
	{
		const APlayerCharacter* PlayerCharacter = *It;

		// ShowOnlyList experiment
		//GetPlanarReflectionComponent()->ShowOnlyActors.AddUnique(PlayerCharacter);

		for (ABaseItem* Item : PlayerCharacter->GetInventoryComponent()->GetInventoryItems())
		{
			if (Item && Item->IsA(ABaseWeapon::StaticClass()))
				GetPlanarReflectionComponent()->HideActorComponents(Item);

			// ShowOnlyList experiment
			//if (Item && Item->IsA(ABaseWeapon::StaticClass()))
			//{
			//	PlayerCharacter->IsLocalPlayer() ? GetPlanarReflectionComponent()->HideActorComponents(Item) : GetPlanarReflectionComponent()->ShowOnlyActorComponents(Item);
			//}
		}
	
		GetPlanarReflectionComponent()->HideComponent(PlayerCharacter->GetMesh1P());
		GetPlanarReflectionComponent()->HideComponent(PlayerCharacter->GetMeshBody1P());
	}

	// Hide all the hidden actor components too (if any are specified)
	for (AActor* Actor : GetPlanarReflectionComponent()->HiddenActors)
	{
		GetPlanarReflectionComponent()->HideActorComponents(Actor);
	}
#else
	Destroy();
#endif
}

#if WITH_EDITOR
bool AMirror::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AMirror::EditorTick(const float DeltaSeconds)
{
#if 0
	GetPlanarReflectionComponent()->ShowFlags.SetSubsurfaceScattering(false);
	GetPlanarReflectionComponent()->ShowFlags.SetGrain(false);
	GetPlanarReflectionComponent()->ShowFlags.SetVignette(false);
	GetPlanarReflectionComponent()->ShowFlags.SetEyeAdaptation(false);
	GetPlanarReflectionComponent()->ShowFlags.SetLensFlares(false);
	GetPlanarReflectionComponent()->ShowFlags.SetDepthOfField(false);
	
	ISGAMEVIEWRETURN()
	
	if (IsSelectedInEditor())
	{
		DrawDebugBox(GetWorld(), GetActorLocation() + VisibilityBoundsTransform.GetLocation(), VisibilityBoundsExtent, VisibilityBoundsTransform.GetRotation(), FColor::Red, false, DeltaSeconds + 0.02f, 0, 1.5f);
	}
#endif
}
#endif
