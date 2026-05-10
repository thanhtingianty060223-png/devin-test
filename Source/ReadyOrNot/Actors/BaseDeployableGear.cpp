#include "BaseDeployableGear.h"
#include "ReadyOrNot.h"

ABaseDeployableGear::ABaseDeployableGear()
{
	bDeployable = true;
}

void ABaseDeployableGear::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}
