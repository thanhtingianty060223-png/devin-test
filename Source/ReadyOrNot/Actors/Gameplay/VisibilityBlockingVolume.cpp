// Void Interactive, 2020


#include "VisibilityBlockingVolume.h"

AVisibilityBlockingVolume::AVisibilityBlockingVolume()
{
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetBrushComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetBrushComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}
