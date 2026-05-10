// Void Interactive, 2020

#pragma once
#ifdef PLANAR_REFLECTION
#include "Engine/PlanarReflection.h"
#endif
#include "Mirror.generated.h"

DECLARE_STATS_GROUP(TEXT("Mirror"), STATGROUP_Mirror, STATCAT_Advanced);

/**
 * Renders accurate reflections of the environment
 */
UCLASS(HideCategories = ("Rendering", "Replication", "Collision", "Input", "Actor", "LOD", "Cooking"))
class READYORNOT_API AMirror : public AActor
{
	GENERATED_BODY()

public:
	AMirror();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void EditorTick(float DeltaSeconds);
	#endif

	#if WITH_EDITORONLY_DATA
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup")
	uint8 bDrawVisibilityBounds : 1;
	#endif
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup")
	FTransform VisibilityBoundsTransform;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup")
	FVector VisibilityBoundsExtent;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Setup")
	uint8 bDynamicShadowsDisabled : 1;
};
