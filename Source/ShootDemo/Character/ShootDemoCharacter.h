// ShootDemoCharacter.h — 第一人称角色：移动、射击、受伤、死亡重生、网络复制

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ShootDemoCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class AWeaponBase;

UCLASS()
class SHOOTDEMO_API AShootDemoCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AShootDemoCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ===== 组件访问 =====

	/** 第一人称手部骨骼网格体 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Components")
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	/** 第一人称摄像机 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Components")
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	/** 获取控制旋转（用于射击方向） */
	FRotator GetControlRotation() const;

	// ===== 生命值 =====

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Health")
	bool IsDead() const { return bIsDead; }

	/** 治疗 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Health")
	void Heal(float Amount);

	// ===== 弹药 =====

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Ammo")
	int32 GetCurrentAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Ammo")
	int32 GetMaxAmmo() const;

	/** 添加弹药 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Ammo")
	void AddAmmo(int32 Amount);

	// ===== 武器 =====

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Weapon")
	AWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	// ===== 输入 =====

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartFire();
	void StopFire();
	void Reload();

protected:
	// ===== 组件 =====

	/** 第一人称手部骨骼网格体 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* Mesh1P;

	/** 第一人称摄像机 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FirstPersonCamera;

	/** 第三人称弹簧臂（观察者视角） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	/** 第三人称摄像机 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* ThirdPersonCamera;

	// ===== Enhanced Input =====

	/** 输入映射上下文 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Input")
	UInputMappingContext* DefaultMappingContext;

	/** 移动输入动作 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Input")
	UInputAction* IA_Move;

	/** 视角输入动作 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Input")
	UInputAction* IA_Look;

	/** 开火输入动作 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Input")
	UInputAction* IA_Fire;

	/** 换弹输入动作 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Input")
	UInputAction* IA_Reload;

	/** 跳跃输入动作 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Input")
	UInputAction* IA_Jump;

	// ===== 生命值 =====

	/** 最大生命值 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Health")
	float MaxHealth;

	/** 当前生命值 — 复制 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "ShootDemo|Health")
	float CurrentHealth;

	/** 是否已死亡 — 复制 */
	UPROPERTY(ReplicatedUsing = OnRep_IsDead, BlueprintReadOnly, Category = "ShootDemo|Health")
	bool bIsDead;

	// ===== 武器配置 =====

	/** 默认武器类型 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon")
	TSubclassOf<AWeaponBase> WeaponClass;

	/** 武器挂载点Socket名称 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Weapon")
	FName WeaponSocketName;

	// ===== 重生 =====

	/** 死亡后重生延迟（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Respawn")
	float RespawnDelay;

	/** 重生无敌时间 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Respawn")
	float RespawnInvincibilityTime;

	// ===== 伤害 =====

	/** 受击动画蒙太奇 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Animation")
	UAnimMontage* HitReactAnimation;

	/** 死亡动画蒙太奇 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Animation")
	UAnimMontage* DeathAnimation;

	/** 当前武器实例 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Weapon")
	AWeaponBase* CurrentWeapon;

	/** 上次受伤时间（用于无敌） */
	float LastDamageTime;

private:
	/** 装备武器 */
	void EquipWeapon();

	/** 生命值复制回调 */
	UFUNCTION()
	void OnRep_CurrentHealth();

	/** 死亡状态复制回调 */
	UFUNCTION()
	void OnRep_IsDead();

	/** 执行死亡 */
	void Die(AController* KillerController);

	/** 执行重生 */
	void Respawn();

public:
	// ===== 重写接口 =====

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ===== 网络RPC =====

	/** 多播RPC：播放受击反应 */
	UFUNCTION(NetMulticast, Unreliable, Category = "ShootDemo|Network")
	void Multicast_OnHitReact(const FVector& DamageLocation);

	/** 多播RPC：播放死亡效果 */
	UFUNCTION(NetMulticast, Reliable, Category = "ShootDemo|Network")
	void Multicast_OnDeath();

	/** 多播RPC：播放重生效果 */
	UFUNCTION(NetMulticast, Reliable, Category = "ShootDemo|Network")
	void Multicast_OnRespawn();
};
