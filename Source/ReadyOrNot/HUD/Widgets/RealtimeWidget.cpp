#include "RealtimeWidget.h"
#include "ReadyOrNot.h"

void URealtimeWidget::OnSynchronizeProperties_Implementation()
{
	// Override in blueprints!
}

void URealtimeWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	OnSynchronizeProperties();
}
