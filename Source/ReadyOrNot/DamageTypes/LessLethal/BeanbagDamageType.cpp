// Void Interactive, 2020

#include "BeanbagDamageType.h"

#include "Actors/Projectiles/DamageProjectiles/BulletProjectile.h"
#include "Characters/CyberneticCharacter.h"
#include "Characters/CyberneticController.h"

void UBeanbagDamageType::ScriptedStunEvent(class AReadyOrNotCharacter* Victim, float& Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ACyberneticCharacter* CyberneticChar = Cast<ACyberneticCharacter>(Victim);
	if (CyberneticChar)
	{
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			FPointDamageEvent* PointHitEvent = (FPointDamageEvent*)&DamageEvent;
			if (Victim->DamageHitHead(*PointHitEvent) && Cast<APlayerController>(EventInstigator))
			{
				CyberneticChar->ReactToHeadBeanbagHit(Damage, PointHitEvent);

				float ImpulseStrength = 4000.0f;
				const ABaseWeapon* Weapon = Cast<ABaseWeapon>(DamageCauser);
				if (!Weapon)
				{
					if (const ABulletProjectile* Projectile = Cast<ABulletProjectile>(DamageCauser))
					{
						Weapon = Cast<ABaseWeapon>(Projectile->FiredFromWeapon);
					}
				}
				
				if (Weapon && Weapon->GetCurrentAmmoType())
					ImpulseStrength = Weapon->GetCurrentAmmoType()->HeadRagdollImpulseStrength;
			
				// add forces on initial shot
				CyberneticChar->GetMesh()->AddImpulseAtLocation((PointHitEvent->HitInfo.TraceEnd - PointHitEvent->HitInfo.TraceStart).GetSafeNormal() * ImpulseStrength, PointHitEvent->HitInfo.Location, PointHitEvent->HitInfo.BoneName);
			}
			else
			{
				Victim->StartStun(EStunType::ST_Beanbag);
			}
		}
		else
		{
			Victim->StartStun(EStunType::ST_Beanbag);
		}
	}
	else
	{
		Super::ScriptedStunEvent(Victim, Damage, DamageEvent, EventInstigator, DamageCauser);
	}
}
