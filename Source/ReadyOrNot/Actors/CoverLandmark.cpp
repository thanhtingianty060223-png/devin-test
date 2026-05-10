// Void Interactive, 2020

#include "CoverLandmark.h"
#include "CoverLandmarkProxy.h"
#include "Characters/CyberneticController.h"

#include "Info/Activities/SearchLandmarkActivity.h"

#if WITH_EDITOR
#include "EditorViewportClient.h"
#endif

TAutoConsoleVariable<int32> CVarCoverLandmarkDraw(TEXT("CoverLandmark.Draw"), 0, TEXT("0 = Hide cover landmarks in world. 1 = Draw cover landmarks in world"));

ACoverLandmark::ACoverLandmark()
{
	#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	PrimaryActorTick.TickInterval = 0.0f;
	#else
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	PrimaryActorTick.TickInterval = 0.033f;
	#endif

	SetCanBeDamaged(false);
	bFindCameraComponentWhenViewTarget = false;
	bBlockInput = true;
	bRelevantForLevelBounds = false;
	AutoReceiveInput = EAutoReceiveInput::Disabled;
	bEnableAutoLODGeneration = false;
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));
	SetRootComponent(SceneComponent);
	SceneComponent->SetMobility(EComponentMobility::Static);
	SceneComponent->SetCanEverAffectNavigation(false);

	#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BaseMale_SkelMesh(TEXT("SkeletalMesh'/Game/ReadyOrNot/Character/BaseMale/Mesh/SK_mesh_w_interaction_cam.SK_mesh_w_interaction_cam'"));
	PreviewMesh = BaseMale_SkelMesh.Object;
	
	static ConstructorHelpers::FObjectFinder<UTexture2D> HidingIcon_(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/HidingIcon.HidingIcon'"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> HidingIconOff_(TEXT("Texture2D'/Game/ReadyOrNot/UI/Elements/HidingIcon_Off.HidingIcon_Off'"));

	HidingIcon = HidingIcon_.Object;
	HidingIcon_Off = HidingIconOff_.Object;

	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));
	BillboardComponent->SetupAttachment(GetRootComponent());
	BillboardComponent->SetRelativeLocation(FVector::ZeroVector);
	BillboardComponent->SetWorldScale3D(FVector(0.85f));
	BillboardComponent->SetMobility(EComponentMobility::Static);
	BillboardComponent->SetCanEverAffectNavigation(false);
	BillboardComponent->bEnableAutoLODGeneration = false;
	BillboardComponent->bReceiveMobileCSMShadows = false;
	BillboardComponent->bIsScreenSizeScaled = true;
	BillboardComponent->ScreenSize = 0.0035f;
	BillboardComponent->SetSprite(HidingIcon_.Object);

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow Component"));
	ArrowComponent->SetupAttachment(GetRootComponent());
	ArrowComponent->SetRelativeLocation(FVector::ZeroVector);
	#endif

	bEnabled = true;

	AllowedTeamsForCover.Empty();
	AllowedTeamsForCover.Add(ETeamType::TT_SUSPECT);
	AllowedTeamsForCover.Add(ETeamType::TT_CIVILIAN);

	bDisableCollision = true;
}

void ACoverLandmark::EnableLandmark()
{
	bEnabled = true;
}

void ACoverLandmark::DisableLandmark()
{
	bEnabled = false;
}

void ACoverLandmark::ToggleLandmarkEnabled(const bool bEnable)
{
	bEnabled = bEnable;
}

void ACoverLandmark::AddCooldownFor(AController* InController, const float InCooldownTime)
{
	CooldownMap.Remove(InController);
	
	if (InCooldownTime > 0.0f)
		CooldownMap.Add(InController, InCooldownTime);
}

bool ACoverLandmark::IsCooldownActiveFor(const AController* InController) const
{
	return CooldownMap.Find(InController) != nullptr;
}

void ACoverLandmark::BeginPlay()
{
	Super::BeginPlay();

	EntryPoints.Remove(nullptr);
	ExitPoints.Remove(nullptr);
	bClearedBySwat = false;

	if (!CoverObject.LoadSynchronous())
	{
		Destroy();
		return;
	}

	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllCoverLandmarks.AddUnique(this);
	}
}

void ACoverLandmark::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	if (AReadyOrNotGameState* GS = GetWorld()->GetGameState<AReadyOrNotGameState>())
	{
		GS->AllCoverLandmarks.Remove(this);
	}
}

#if WITH_EDITOR
void ACoverLandmark::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	BillboardComponent->SetSprite(bEnabled ? HidingIcon : HidingIcon_Off);

	if (PropertyChangedEvent.GetPropertyName() == "LandmarkName")
	{
		SetActorLabel("CoverLandmark_" + LandmarkName.Replace(TEXT(" "), TEXT("_")) + "_" + FString::FromInt(GetUniqueID()));
	}
	else if (PropertyChangedEvent.GetPropertyName() == "EntryPoints")
	{
		switch (PropertyChangedEvent.ChangeType)
		{
			case EPropertyChangeType::ArrayClear:
			case EPropertyChangeType::ArrayRemove:
			{
				for (TActorIterator<ACoverLandmarkProxy> It(GetWorld()); It; ++It)
				{
					ACoverLandmarkProxy* Proxy = *It;

					if (Proxy != IdlePoint)
					{
						if (Proxy->LandmarkOwner == this && !EntryPoints.Contains(Proxy) && !ExitPoints.Contains(Proxy))
						{
							Proxy->LandmarkOwner = nullptr;
						}
					}
				}
			}
			break;

			default:
			break;
		}
		
		for (ACoverLandmarkProxy* EntryPoint : EntryPoints)
		{
			if (EntryPoint)
			{
				EntryPoint->LandmarkOwner = this;
				EntryPoint->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

				if (ExitPoints.Contains(EntryPoint))
				{
					EntryPoint->SetActorLabel("CoverLandmarkProxy_" + LandmarkName.Replace(TEXT(" "), TEXT("_")) + "_Entry/Exit_" + FString::FromInt(EntryPoint->GetUniqueID()));
				}
				else
				{
					EntryPoint->SetActorLabel("CoverLandmarkProxy_" + LandmarkName.Replace(TEXT(" "), TEXT("_")) + "_Entry_" + FString::FromInt(EntryPoint->GetUniqueID()));
				}

				if (IdlePoint)
				{
					const FVector DirectionToIdlePoint = (IdlePoint->GetActorLocation() - EntryPoint->GetActorLocation()).GetSafeNormal2D();
					const float DotProduct = FVector::DotProduct(DirectionToIdlePoint, IdlePoint->GetActorRightVector());
					EntryPoint->EntryDirection = DotProduct < 0.0f ? ECoverLandmarkAnimDirection::Left : ECoverLandmarkAnimDirection::Right;
				}
			}
		}
	}
	else if (PropertyChangedEvent.GetPropertyName() == "ExitPoints")
	{
		switch (PropertyChangedEvent.ChangeType)
		{
			case EPropertyChangeType::ArrayClear:
			case EPropertyChangeType::ArrayRemove:
			{
				for (TActorIterator<ACoverLandmarkProxy> It(GetWorld()); It; ++It)
				{
					ACoverLandmarkProxy* Proxy = *It;
					
					if (Proxy != IdlePoint)
					{
						if (Proxy->LandmarkOwner == this && !EntryPoints.Contains(Proxy) && !ExitPoints.Contains(Proxy))
						{
							Proxy->LandmarkOwner = nullptr;
						}
					}
				}
			}
			break;

			default:
			break;
		}
		
		for (ACoverLandmarkProxy* ExitPoint : ExitPoints)
		{
			if (ExitPoint)
			{
				ExitPoint->LandmarkOwner = this;
				ExitPoint->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

				if (EntryPoints.Contains(ExitPoint))
				{
					ExitPoint->SetActorLabel("CoverLandmarkProxy_" + LandmarkName.Replace(TEXT(" "), TEXT("_")) + "_Entry/Exit_" + FString::FromInt(ExitPoint->GetUniqueID()));
				}
				else
				{
					ExitPoint->SetActorLabel("CoverLandmarkProxy_" + LandmarkName.Replace(TEXT(" "), TEXT("_")) + "_Exit_" + FString::FromInt(ExitPoint->GetUniqueID()));
				}

				if (IdlePoint)
				{
					const FVector DirectionToIdlePoint = (IdlePoint->GetActorLocation() - ExitPoint->GetActorLocation()).GetSafeNormal2D();
					const float DotProduct = FVector::DotProduct(DirectionToIdlePoint, IdlePoint->GetActorRightVector());
					ExitPoint->ExitDirection = DotProduct > 0.0f ? ECoverLandmarkAnimDirection::Left : ECoverLandmarkAnimDirection::Right;
				}
			}
		}
	}
	else if (PropertyChangedEvent.GetPropertyName() == "IdlePoint")
	{
		if (IdlePoint)
		{
			IdlePoint->LandmarkOwner = this;
			IdlePoint->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

			IdlePoint->SetActorLabel("CoverLandmarkProxy_" + LandmarkName.Replace(TEXT(" "), TEXT("_")) + "_Idle_" + FString::FromInt(IdlePoint->GetUniqueID()));

			IdlePoint_EditorCopy = IdlePoint;

			for (ACoverLandmarkProxy* EntryPoint : EntryPoints)
			{
				if (EntryPoint)
				{
					if (Entry.bForwardOnly)
					{
						EntryPoint->EntryDirection = ECoverLandmarkAnimDirection::Forward;
					}
					else
					{
						const FVector DirectionToIdlePoint = (IdlePoint->GetActorLocation() - EntryPoint->GetActorLocation()).GetSafeNormal2D();
						const float DotProduct = FVector::DotProduct(DirectionToIdlePoint, IdlePoint->GetActorRightVector());
						EntryPoint->EntryDirection = DotProduct < 0.0f ? ECoverLandmarkAnimDirection::Left : ECoverLandmarkAnimDirection::Right;
					}
				}
			}

			for (ACoverLandmarkProxy* ExitPoint : ExitPoints)
			{
				if (ExitPoint)
				{
					if (Exit.bForwardOnly)
					{
						ExitPoint->ExitDirection = ECoverLandmarkAnimDirection::Forward;
					}
					else
					{
						const FVector DirectionToIdlePoint = (IdlePoint->GetActorLocation() - ExitPoint->GetActorLocation()).GetSafeNormal2D();
						const float DotProduct = FVector::DotProduct(DirectionToIdlePoint, IdlePoint->GetActorRightVector());
						ExitPoint->ExitDirection = DotProduct > 0.0f ? ECoverLandmarkAnimDirection::Left : ECoverLandmarkAnimDirection::Right;
					}
				}
			}
		}
		else
		{
			if (IdlePoint_EditorCopy)
			{
				IdlePoint_EditorCopy->LandmarkOwner = nullptr;
			}
		}
	}
	/*else if (PropertyChangedEvent.GetPropertyName() == "MaxExitLoopTime" ||
			PropertyChangedEvent.GetPropertyName() == "ExitLoopAnim" ||
			PropertyChangedEvent.GetPropertyName() == "bUseExitLoop")
	{
		if (ExitLoopAnim)
		{
			if (MaxExitLoopTime <= ExitLoopAnim->GetPlayLength())
			{
				MaxExitLoopTime = ExitLoopAnim->GetPlayLength() * 2.0f;
			}
		}
	}*/
}

bool ACoverLandmark::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ACoverLandmark::PostEditMove(const bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		SetActorLabel("CoverLandmark_" + LandmarkName.Replace(TEXT(" "), TEXT("_")) + "_" + FString::FromInt(GetUniqueID()));
	}
}

bool ACoverLandmark::IsAnyProxySelected() const
{
	for (ACoverLandmarkProxy* EntryPoint : EntryPoints)
	{
		if (EntryPoint)
		{
			if (EntryPoint->IsSelected())
				return true;
		}
	}
	
	for (ACoverLandmarkProxy* ExitPoint : ExitPoints)
	{
		if (ExitPoint)
		{
			if (ExitPoint->IsSelected())
				return true;
		}
	}

	if (IdlePoint)
	{
		if (IdlePoint->IsSelected())
			return true;
	}

	return false;
}
#endif

void ACoverLandmark::PreviewEntryAnim()
{
	#if WITH_EDITOR
	if (EntryPoints.Num() > 0)
		PreviewAnim(EntryPoints[FMath::RandRange(0, EntryPoints.Num()-1)], Entry.bForwardOnly ? Entry.ForwardAnim : Entry.LeftAnim, Entry.AnimYawOffset);
	#endif
}

void ACoverLandmark::PreviewIdleAnim()
{
	#if WITH_EDITOR
	if (IdlePoint)
	{
		PreviewAnim(IdlePoint, IdleAnim);
	}
	#endif
}

void ACoverLandmark::PreviewExitAnim()
{
	#if WITH_EDITOR
	if (ExitPoints.Num() > 0)
		PreviewAnim(ExitPoints[FMath::RandRange(0, ExitPoints.Num()-1)], Exit.bForwardOnly ? Exit.ForwardAnim : Exit.LeftAnim, Exit.AnimYawOffset);
	#endif
}

bool ACoverLandmark::IsSecured_Implementation() const
{
	return bClearedBySwat;
}

FVector ACoverLandmark::GetLocation_Implementation() const
{
	return GetActorLocation();
}

bool ACoverLandmark::CanBeSecured_Implementation() const
{
	return !bClearedBySwat;
}

#if WITH_EDITORONLY_DATA
void ACoverLandmark::PreviewAnim(ACoverLandmarkProxy* Proxy, UAnimSequenceBase* Anim, const float YawOffset)
{
	if (PreviewSkeletalMesh)
	{
		PreviewSkeletalMesh->Destroy();
	}

	if (Proxy && PreviewMesh && Anim)
	{
		PreviewSkeletalMesh = GetWorld()->SpawnActor<ASkeletalMeshActor>(Proxy->GetActorLocation() - FVector::UpVector * 10.0f, Proxy->GetActorRotation() - FRotator(0.0f, 90.0f, 0.0f) + FRotator(0.0f, YawOffset, 0.0f));
		
		PreviewSkeletalMesh->GetSkeletalMeshComponent()->SetSkeletalMesh(PreviewMesh);
		PreviewSkeletalMesh->GetSkeletalMeshComponent()->SetUpdateAnimationInEditor(true);
		Anim->EnableRootMotionSettingFromMontage(false, ERootMotionRootLock::RefPose);
		PreviewSkeletalMesh->GetSkeletalMeshComponent()->PlayAnimation(Anim, true);

		PreviewSkeletalMesh->AttachToActor(Proxy, FAttachmentTransformRules::KeepWorldTransform);
		
		PreviewSkeletalMesh->SetLifeSpan(Anim->GetPlayLength() - 0.05f);
	}
}
#endif

void ACoverLandmark::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::Editor)
	{
		// Force 1, 1, 1 scale to ensure correct setup
		SetActorScale3D(FVector::OneVector);
		
		if (IsSelected() || IsAnyProxySelected())
		{
			ISGAMEVIEWRETURN()
			
			if (IdlePoint)
			{
				for (ACoverLandmarkProxy* EntryPoint : EntryPoints)
				{
					if (EntryPoint)
					{
						const FColor LineColor = ExitPoints.Contains(EntryPoint) ? FColor::Orange : FColor::Green;
						DrawDebugLine(GetWorld(), GetActorLocation(), EntryPoint->GetActorLocation(), LineColor, false, -1.0f, 0, 1.5f);

						const FVector DirectionToIdlePoint = (IdlePoint->GetActorLocation() - EntryPoint->GetActorLocation()).GetSafeNormal2D();
						const float DotProduct = FVector::DotProduct(DirectionToIdlePoint, IdlePoint->GetActorRightVector());
						EntryPoint->EntryDirection = DotProduct < 0.0f ? ECoverLandmarkAnimDirection::Left : ECoverLandmarkAnimDirection::Right;
					}
				}
				
				for (ACoverLandmarkProxy* ExitPoint : ExitPoints)
				{
					if (ExitPoint)
					{
						const FColor LineColor = EntryPoints.Contains(ExitPoint) ? FColor::Orange : FColor::Red;
						DrawDebugLine(GetWorld(), GetActorLocation(), ExitPoint->GetActorLocation(), LineColor, false, -1.0f, 0, 1.5f);

						DrawDebugBox(GetWorld(), ExitPoint->GetActorLocation() + ExitTriggerBoxTransform.GetLocation(), ExitTriggerBoxExtent, ExitTriggerBoxTransform.GetRotation(), FColor::Red);

						const FVector DirectionToIdlePoint = (IdlePoint->GetActorLocation() - ExitPoint->GetActorLocation()).GetSafeNormal2D();
						const float DotProduct = FVector::DotProduct(DirectionToIdlePoint, IdlePoint->GetActorRightVector());
						ExitPoint->ExitDirection = DotProduct > 0.0f ? ECoverLandmarkAnimDirection::Left : ECoverLandmarkAnimDirection::Right;
					}
				}

				DrawDebugLine(GetWorld(), GetActorLocation(), IdlePoint->GetActorLocation(), FColor::Blue, false, -1.0f, 0, 1.5f);
				
				DrawDebugBox(GetWorld(), IdlePoint->GetActorLocation() + IdleTriggerBoxTransform.GetLocation(), IdleTriggerBoxExtent, IdleTriggerBoxTransform.GetRotation(), FColor::Red);
			}
		}

		return;
	}
	#endif
	
	#if !UE_BUILD_SHIPPING
	if (CVarCoverLandmarkDraw.GetValueOnAnyThread() > 0)
	{
		if (const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			const FVector DirectionToLandmark = (GetActorLocation() - CameraManager->GetCameraLocation()).GetSafeNormal2D();
			const float DotProduct = FVector::DotProduct(CameraManager->GetCameraRotation().Vector(), DirectionToLandmark);

			if (DotProduct > 0.95f)
			{
				DrawDebugSphere(GetWorld(), GetActorLocation(), 25.0f, 4, FColor::Blue, false, -1.0f, 0, 1.5f);

				FString CooldownMapAsString = "";
				if (CooldownMap.Num() > 0)
				{
					CooldownMapAsString = FString("Cooldowns: ") + LINE_TERMINATOR;

					for (const auto& Element : CooldownMap)
					{
						CooldownMapAsString += GetNameSafe(Element.Key) + ": " + FString::SanitizeFloat(Element.Value) + "s";
					}
				}
				
				const FString DebugString = FString(*LandmarkName) + LINE_TERMINATOR +
											"Occupied By: " + GetNameSafe(OccupiedByController) + LINE_TERMINATOR +
											CooldownMapAsString;

				DrawDebugString(GetWorld(), GetActorLocation(), DebugString, nullptr, FColor::White, DeltaSeconds + 0.02f, true);

				for (ACoverLandmarkProxy* EntryPoint : EntryPoints)
				{
					if (EntryPoint)
					{
						const FColor LineColor = ExitPoints.Contains(EntryPoint) ? FColor::Orange : FColor::Green;
						DrawDebugLine(GetWorld(), GetActorLocation(), EntryPoint->GetActorLocation(), LineColor, false, -1.0f, 0, 1.5f);
					}
				}
				
				for (ACoverLandmarkProxy* ExitPoint : ExitPoints)
				{
					if (ExitPoint)
					{
						const FColor LineColor = EntryPoints.Contains(ExitPoint) ? FColor::Orange : FColor::Red;
						DrawDebugLine(GetWorld(), GetActorLocation(), ExitPoint->GetActorLocation(), LineColor, false, -1.0f, 0, 1.5f);

						DrawDebugBox(GetWorld(), ExitPoint->GetActorLocation() + ExitTriggerBoxTransform.GetLocation(), ExitTriggerBoxExtent, ExitTriggerBoxTransform.GetRotation(), FColor::Red);
					}
				}

				if (IdlePoint)
				{
					DrawDebugLine(GetWorld(), GetActorLocation(), IdlePoint->GetActorLocation(), FColor::Blue, false, -1.0f, 0, 1.5f);
					
					DrawDebugBox(GetWorld(), IdlePoint->GetActorLocation() + IdleTriggerBoxTransform.GetLocation(), IdleTriggerBoxExtent, IdleTriggerBoxTransform.GetRotation(), FColor::Red);
				}
			}
		}
	}
	#endif

	for (TMap<AController*, float>::TIterator It = CooldownMap.CreateIterator(); It; ++It)
	{
		It.Value() -= DeltaSeconds;
		if (It.Value() <= 0.0f)
			It.RemoveCurrent();
	}

	/*
	UActivityManager::IterateAllActivitiesOfType<UTakeCoverAtLandmarkActivity>([&](UTakeCoverAtLandmarkActivity* Activity)
	{
		if (Activity->CoverLandmark == this && Activity->GetActiveStateID() == 2) // hiding state
		{
			bool bSwatIsSearchingThis = false;
			UActivityManager::IterateAllActivitiesOfType<USearchLandmarkActivity>([&](USearchLandmarkActivity* Activity2)
			{
				if (Activity2->CoverLandmark == this)
				{
					bSwatIsSearchingThis = true;
					return false;
				}

				return true;
			});
			
			if (!bSwatIsSearchingThis)
			{
				//bClearedBySwat = false;
			}
			
			return false;
		}

		return true;
	});
	*/
}
