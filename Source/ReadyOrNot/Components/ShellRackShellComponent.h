// Copyright Void Interactive, 2017

#pragma once

#include "Components/StaticMeshComponent.h"
#include "ShellRackShellComponent.generated.h"

// A Shell Rack Shell component is used for shotguns with a shell rack. Each component represents one shell on the rack.
UCLASS(meta = (BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class READYORNOT_API UShellRackShellComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	int32 ShellNumber;
};
