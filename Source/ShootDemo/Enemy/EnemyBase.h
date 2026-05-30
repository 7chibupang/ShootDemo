// EnemyBase.h — 敌人角色：追踪玩家、近战/远程攻击、生命值、网络复制

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyBase.generated.h"

class UPawnSensingComponent;
class UWidgetComponent;
class AShootDemoCharacter;

/** 敌人AI状态枚举 */
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Idle        UMETA(DisplayName = "待机"),
	Patrol      UMETA(DisplayName = "巡逻"),
	Chasing     UMETA(DisplayName = "追击"),
	Attacking   UMETA(DisplayName = "攻击"),
	Dead        UMETA(DisplayName = "死亡")
};

UCLASS()
class SHOOTDEMO_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ===== 组件访问 =====

	/** 感知组件（视觉和听觉） */
	UPawnSensingComponent* GetPawnSensing() const { return PawnSensing; }

	/** 头顶血条 */
	UWidgetComponent* GetHealthBar() const { return HealthBarWidget; }

	// ===== AI状态 =====

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	EEnemyState GetEnemyState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	void SetEnemyState(EEnemyState NewState);

	/** 获取当前追击目标 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	AActor* GetTargetActor() const { return TargetActor; }

	/** 找到最近的可攻击玩家 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	AActor* FindClosestPlayer() const;

	// ===== 攻击 =====

	/** 对目标造成伤害 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	void AttackTarget();

	/** 检查是否有目标在攻击范围内 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	bool IsTargetInAttackRange() const;

	// ===== 生命值 =====

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	bool IsDead() const { return CurrentState == EEnemyState::Dead; }

	/** 获取分数奖励值 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	int32 GetScoreValue() const { return ScoreValue; }

protected:
	// ===== 组件 =====

	/** 感知组件 — 检测附近玩家 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPawnSensingComponent* PawnSensing;

	/** 头顶血条Widget */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* HealthBarWidget;

	// ===== 属性配置 =====

	/** 最大生命值 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Stats")
	float MaxHealth;

	/** 移动速度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Stats")
	float PatrolSpeed;

	/** 追击速度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Stats")
	float ChaseSpeed;

	/** 攻击伤害 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Combat")
	float DamageAmount;

	/** 攻击范围 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Combat")
	float AttackRange;

	/** 攻击冷却时间（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Combat")
	float AttackCooldown;

	/** 视觉感知角度（度） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Sensing")
	float SightAngle;

	/** 视觉感知半径 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Sensing")
	float SightRadius;

	/** 失去目标后继续追击的时间 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Sensing")
	float ChaseLoseTargetTime;

	/** 被击杀时奖励的分数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Scoring")
	int32 ScoreValue;

	// ===== 巡逻 =====

	/** 巡逻半径（如果未设置巡逻点） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Patrol")
	float PatrolRadius;

	/** 巡逻随机等待时间范围 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Patrol")
	FVector2D PatrolWaitTime;

	// ===== 特效和动画 =====

	/** 攻击动画 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Animation")
	UAnimMontage* AttackAnimation;

	/** 受击动画 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Animation")
	UAnimMontage* HitReactAnimation;

	/** 死亡动画 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Animation")
	UAnimMontage* DeathAnimation;

	// ===== 受击音效 =====

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Sound")
	USoundBase* HitSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Sound")
	USoundBase* DeathSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy|Sound")
	USoundBase* AttackSound;

	// ===== 复制属性 =====

	/** 当前生命值 — 复制 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "ShootDemo|Enemy|State")
	float CurrentHealth;

	/** AI状态 — 复制 */
	UPROPERTY(ReplicatedUsing = OnRep_EnemyState, BlueprintReadOnly, Category = "ShootDemo|Enemy|State")
	EEnemyState CurrentState;

	/** 当前目标 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Enemy|State")
	AActor* TargetActor;

	/** 巡逻目标位置 */
	FVector TargetPatrolLocation;

	/** 上次攻击时间 */
	float LastAttackTime;

	/** 失去目标后的计时器 */
	float ChaseLoseTimer;

	/** 巡逻等待计时器 */
	float PatrolWaitTimer;

private:
	// ===== 感知回调 =====

	/** 看到Pawn时回调 */
	UFUNCTION()
	void OnSeePawn(APawn* SeenPawn);

	/** 听到Pawn时回调 */
	UFUNCTION()
	void OnHearPawn(APawn* HeardPawn, const FVector& Location, float Volume);

	// ===== 状态机更新 =====

	void UpdateIdleState(float DeltaTime);
	void UpdatePatrolState(float DeltaTime);
	void UpdateChasingState(float DeltaTime);
	void UpdateAttackingState(float DeltaTime);

	/** 移动到目标位置 */
	void MoveToLocation(const FVector& TargetLocation);

	/** 移动到目标Actor */
	void MoveToActor(AActor* Target);

	// ===== 网络回调 =====

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnRep_EnemyState();

	// ===== 死亡 =====

	void Die(AController* KillerController);

public:
	// ===== 重写接口 =====

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ===== 网络RPC =====

	/** 多播RPC：播放攻击动画 */
	UFUNCTION(NetMulticast, Unreliable, Category = "ShootDemo|Network")
	void Multicast_OnAttack();

	/** 多播RPC：播放受击反应 */
	UFUNCTION(NetMulticast, Unreliable, Category = "ShootDemo|Network")
	void Multicast_OnHitReact();

	/** 多播RPC：播放死亡效果 */
	UFUNCTION(NetMulticast, Reliable, Category = "ShootDemo|Network")
	void Multicast_OnDeath();

	/** 多播RPC：更新AI状态 */
	UFUNCTION(NetMulticast, Reliable, Category = "ShootDemo|Network")
	void Multicast_OnStateChanged(EEnemyState NewState);
};
