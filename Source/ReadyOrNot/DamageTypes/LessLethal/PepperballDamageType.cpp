#include "PepperballDamageType.h"
#include "ReadyOrNot.h"
#include "Characters/CyberneticCharacter.h"
#include "ReadyOrNotGameMode.h"
#include "Actors/Items/PepperballGun.h"
#include "Actors/Projectiles/DamageProjectiles/BulletProjectile.h"

void UPepperballDamageType::ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, 
	struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// check for cybernetics stuff
	ACyberneticCharacter* CyberneticChar = Cast<ACyberneticCharacter>(Victim);
	if (CyberneticChar)
	{
		ABulletProjectile* Bullet = Cast<ABulletProjectile>(DamageCauser);
		APepperballGun* PepperballGun;
		FPointDamageEvent PointHitEvent = *((FPointDamageEvent*)&DamageEvent);

		if (Bullet)
		{
			PepperballGun = Cast<APepperballGun>(Bullet->FiredFromWeapon);
			if (PepperballGun)
			{
				if (CyberneticChar->GetTeam() == ETeamType::TT_CIVILIAN)
				{	// civilians can get abuse docked in certain circumstances
					if (Victim->DamageHitHead(PointHitEvent))
					{	// headshot
						PepperballGun->IncrementHeadshotCounter(CyberneticChar);
					}
				}

				if (CyberneticChar->IsStunned())
				{
					PepperballGun->IncrementStunShotCounter(CyberneticChar);
				}
			}
		}

		if (CyberneticChar->IsStunnedWith(StunType))
		{
			return; // don't stack the amount
		}
	}

	Super::ScriptedStunEvent(Victim, Damage, DamageEvent, EventInstigator, DamageCauser);
}