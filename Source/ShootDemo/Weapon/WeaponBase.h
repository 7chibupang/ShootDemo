// WeaponBase.h — 武器基类：射线检测射击、弹药管理、网络复制特效

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "WeaponBase.generated.h"

class USkeletalMeshComponent;
class USceneComponent;
class UAnimMontage;

UCLASS()
class SHOOTDEMO_API AWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AWeaponBase();

	// ===== 组件 =====

	/** 武器根组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* Root;

	/** 武器网格体 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* WeaponMesh;

	/** 枪口位置（特效/射线起点） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* MuzzleLocation;

	// ===== 射击 =====

	/** 开始射击（由角色调用） */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Weapon")
	void StartFire();

	/** 停止射击 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Weapon")
	void StopFire();

	/** 换弹 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Weapon")
	void Reload();

	/** 是否正在射击 */
	bool IsFiring() const { return bIsFiring; }

	/** 是否可以射击 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Weapon")
	bool CanFire() const;

	// ===== 弹药 =====

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Weapon")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Weapon")
	int32 GetMaxAmmo() const { return MaxAmmo; }

	/** 添加弹药（拾取物使用） */
	void AddAmmo(int32 Amount);

protected:
	/** 执行射击（服务器端命中判定） */
	void PerformFire();

	// ===== 可配置属性 =====

	/** 伤害值 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Stats")
	float Damage;

	/** 射程 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Stats")
	float Range;

	/** 射速（发/秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Stats")
	float FireRate;

	/** 最大弹药 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Stats")
	int32 MaxAmmo;

	/** 换弹时间（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Stats")
	float ReloadTime;

	/** 子弹散布半径（度） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Stats")
	float BulletSpread;

	// ===== 特效和音效 =====

	/** 枪口闪光特效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Effects")
	UParticleSystem* MuzzleFlashEffect;

	/** 命中特效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Effects")
	UParticleSystem* ImpactEffect;

	/** 弹道轨迹特效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Effects")
	UParticleSystem* TrailEffect;

	/** 射击音效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Effects")
	USoundBase* FireSound;

	/** 换弹音效 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Effects")
	USoundBase* ReloadSound;

	/** 射击动画蒙太奇 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Animation")
	UAnimMontage* FireAnimation;

	/** 换弹动画蒙太奇 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon|Animation")
	UAnimMontage* ReloadAnimation;

	// ===== 网络状态 =====

	/** 当前弹药 — 复制 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Weapon|State")
	int32 CurrentAmmo;

	/** 是否在换弹中 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Weapon|State")
	bool bIsReloading;

	/** 上次射击时间 */
	float LastFireTime;

	/** 是否正在持续射击 */
	bool bIsFiring;

	/** 射击间隔定时器句柄 */
	FTimerHandle FireTimerHandle;

	/** 武器拥有者 */
	UPROPERTY()
	class AShootDemoCharacter* OwningCharacter;

public:
	void SetOwningCharacter(class AShootDemoCharacter* NewOwner) { OwningCharacter = NewOwner; }

	// ===== 网络RPC =====

	/** 服务器RPC：请求开火 */
	UFUNCTION(Server, Reliable, WithValidation, Category = "ShootDemo|Network")
	void Server_Fire(const FVector& FireLocation, const FVector& FireDirection);

	/** 多播RPC：播放开火特效 */
	UFUNCTION(NetMulticast, Unreliable, Category = "ShootDemo|Network")
	void Multicast_OnFire();

	/** 多播RPC：播放命中特效 */
	UFUNCTION(NetMulticast, Unreliable, Category = "ShootDemo|Network")
	void Multicast_OnHit(const FVector& HitLocation, const FVector& HitNormal);

	/** 多播RPC：播放换弹 */
	UFUNCTION(NetMulticast, Reliable, Category = "ShootDemo|Network")
	void Multicast_OnReload();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
