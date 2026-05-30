// PickupBase.h — 拾取物基类：碰撞检测、自动重生

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SHOOTDEMO_API APickupBase : public AActor
{
	GENERATED_BODY()

public:
	APickupBase();

	virtual void BeginPlay() override;

	/** 是否可以拾取 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Pickup")
	bool CanBePickedUp() const { return bIsActive; }

protected:
	// ===== 组件 =====

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PickupMesh;

	// ===== 配置 =====

	/** 拾取后重生时间（0表示不重生） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Pickup")
	float RespawnTime;

	/** 拾取音效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Pickup")
	USoundBase* PickupSound;

	/** 拾取特效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Pickup")
	UParticleSystem* PickupEffect;

	/** 旋转速度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Pickup")
	float RotationSpeed;

	// ===== 状态 =====

	bool bIsActive;

	/** 碰撞回调 */
	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 执行拾取（子类重写） */
	virtual void OnPickedUp(class AShootDemoCharacter* Player) {}

	/** 激活拾取物 */
	void ActivatePickup();

	/** 停用拾取物（进入重生等待） */
	void DeactivatePickup();

public:
	virtual void Tick(float DeltaTime) override;
};
