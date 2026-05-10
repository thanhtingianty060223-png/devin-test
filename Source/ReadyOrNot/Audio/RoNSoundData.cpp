// Copyright Void Interactive, 2017

#include "RoNSoundData.h"
#include "ReadyOrNot.h"




FWeaponSoundData UWeaponSound::GetFiringSound(bool bOutside)
{
	if (bOutside)
		return Firing_Outside;
	return Firing_Inside;
}
