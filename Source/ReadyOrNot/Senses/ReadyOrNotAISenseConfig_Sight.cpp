// Void Interactive, 2020


#include "ReadyOrNotAISenseConfig_Sight.h"

#include "ReadyOrNotAISense_Sight.h"

TSubclassOf<UAISense> UReadyOrNotAISenseConfig_Sight::GetSenseImplementation() const
{
	return UReadyOrNotAISense_Sight::StaticClass();
}
