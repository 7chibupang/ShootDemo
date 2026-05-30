// HealthPickup.h — 生命值拾取物

#pragma once

#include "CoreMinimal.h"
#include "PickupBase.h"
#include "HealthPickup.generated.h"

UCLASS()
class SHOOTDEMO_API AHealthPickup : public APickupBase
{
	GENERATED_BODY()

public:
	AHealthPickup();

protected:
	/** 恢复的生命值 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Pickup")
	float HealAmount;

	virtual void OnPickedUp(class AShootDemoCharacter* Player) override;
};
