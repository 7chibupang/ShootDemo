// HealthPickup.cpp — 生命值拾取物实现

#include "HealthPickup.h"
#include "../Character/ShootDemoCharacter.h"

AHealthPickup::AHealthPickup()
{
	HealAmount = 30.0f;
}

void AHealthPickup::OnPickedUp(AShootDemoCharacter* Player)
{
	if (Player && HasAuthority())
	{
		Player->Heal(HealAmount);
	}
}
