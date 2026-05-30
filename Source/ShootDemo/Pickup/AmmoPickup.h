// AmmoPickup.h — 弹药拾取物

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "AmmoPickup.generated.h"

UCLASS()
class SHOOTDEMO_API AAmmoPickup : public APickupBase
{
	GENERATED_BODY()

public:
	AAmmoPickup();

protected:
	/** 补充的弹药数量 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Pickup")
	int32 AmmoAmount;

	virtual void OnPickedUp(class AShootDemoCharacter* Player) override;
};
