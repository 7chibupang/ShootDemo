// AmmoPickup.cpp — 弹药拾取物实现

#include "AmmoPickup.h"
#include "../Character/ShootDemoCharacter.h"

AAmmoPickup::AAmmoPickup()
{
	AmmoAmount = 15;
}

void AAmmoPickup::OnPickedUp(AShootDemoCharacter* Player)
{
	if (Player && HasAuthority())
	{
		Player->AddAmmo(AmmoAmount);
	}
}
