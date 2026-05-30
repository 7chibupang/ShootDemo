// EnemyBase.cpp — 敌人角色实现（AI状态机、战斗、网络复制）

#include "EnemyBase.h"
#include "EnemyAIController.h"
#include "../Core/ShootDemoGameMode.h"
#include "../Character/ShootDemoCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Perception/PawnSensingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// === 感知组件 ===
	PawnSensing = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensing"));
	PawnSensing->bSeePawns = true;
	PawnSensing->bHearNoises = true;

	// === 头顶血条 ===
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	HealthBarWidget->SetupAttachment(GetRootComponent());
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen); // 始终面向屏幕

	// === 默认属性 ===
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	PatrolSpeed = 200.0f;
	ChaseSpeed = 500.0f;
	DamageAmount = 15.0f;
	AttackRange = 150.0f;
	AttackCooldown = 1.0f;
	SightAngle = 60.0f;
	SightRadius = 2000.0f;
	ChaseLoseTargetTime = 5.0f;
	ScoreValue = 1;

	PatrolRadius = 1000.0f;
	PatrolWaitTime = FVector2D(1.0f, 3.0f);

	CurrentState = EEnemyState::Idle;
	LastAttackTime = -999.0f;
	ChaseLoseTimer = 0.0f;
	PatrolWaitTimer = 0.0f;

	// 自动拥有AI控制器
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();

	bReplicates = true;

	// 设置移动组件
	GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
}

// ===== BeginPlay =====

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 服务器端绑定感知事件
	if (HasAuthority())
	{
		PawnSensing->OnSeePawn.AddDynamic(this, &AEnemyBase::OnSeePawn);
		PawnSensing->OnHearPawn.AddDynamic(this, &AEnemyBase::OnHearPawn);

		PawnSensing->SightAngle = SightAngle;
		PawnSensing->SightRadius = SightRadius / 2.0f; // 使用半值
		PawnSensing->SetPeripheralVisionAngle(SightAngle);

		// 开始巡逻
		SetEnemyState(EEnemyState::Patrol);
	}
}

// ===== Tick/状态机 =====

void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 服务器端运行AI状态机
	if (!HasAuthority())
	{
		return;
	}

	switch (CurrentState)
	{
	case EEnemyState::Idle:
		UpdateIdleState(DeltaTime);
		break;
	case EEnemyState::Patrol:
		UpdatePatrolState(DeltaTime);
		break;
	case EEnemyState::Chasing:
		UpdateChasingState(DeltaTime);
		break;
	case EEnemyState::Attacking:
		UpdateAttackingState(DeltaTime);
		break;
	case EEnemyState::Dead:
		break;
	}
}

void AEnemyBase::SetEnemyState(EEnemyState NewState)
{
	if (CurrentState == NewState || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	EEnemyState OldState = CurrentState;
	CurrentState = NewState;

	// 根据状态调整移动速度
	switch (NewState)
	{
	case EEnemyState::Chasing:
	case EEnemyState::Attacking:
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
		break;
	default:
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
		break;
	}

	Multicast_OnStateChanged(NewState);

	UE_LOG(LogTemp, Verbose, TEXT("[ShootDemo] 敌人 %s 状态变化: %d -> %d"),
		*GetName(), (int32)OldState, (int32)NewState);
}

// ===== 状态更新函数 =====

void AEnemyBase::UpdateIdleState(float DeltaTime)
{
	// 短暂待机后切换到巡逻
	PatrolWaitTimer -= DeltaTime;
	if (PatrolWaitTimer <= 0.0f)
	{
		SetEnemyState(EEnemyState::Patrol);
	}
}

void AEnemyBase::UpdatePatrolState(float DeltaTime)
{
	// 有目标就追击
	if (TargetActor)
	{
		SetEnemyState(EEnemyState::Chasing);
		return;
	}

	// 检查是否到达巡逻目标位置
	float DistToTarget = FVector::Dist(GetActorLocation(), TargetPatrolLocation);
	if (DistToTarget < 100.0f || TargetPatrolLocation.IsNearlyZero())
	{
		// 随机等待后选择新位置
		PatrolWaitTimer -= DeltaTime;
		if (PatrolWaitTimer <= 0.0f)
		{
			// 选择随机巡逻点
			UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
			if (NavSystem)
			{
				FNavLocation NavLocation;
				if (NavSystem->GetRandomReachablePointInRadius(GetActorLocation(), PatrolRadius, NavLocation))
				{
					TargetPatrolLocation = NavLocation.Location;
					MoveToLocation(TargetPatrolLocation);
				}
			}

			PatrolWaitTimer = FMath::RandRange(PatrolWaitTime.X, PatrolWaitTime.Y);
		}
	}
}

void AEnemyBase::UpdateChasingState(float DeltaTime)
{
	// 目标丢失检查
	if (!TargetActor || TargetActor->IsPendingKillPending())
	{
		ChaseLoseTimer -= DeltaTime;
		if (ChaseLoseTimer <= 0.0f)
		{
			TargetActor = nullptr;
			SetEnemyState(EEnemyState::Patrol);
			return;
		}
	}
	else
	{
		ChaseLoseTimer = ChaseLoseTargetTime;

		// 目标存活检查
		AShootDemoCharacter* PlayerChar = Cast<AShootDemoCharacter>(TargetActor);
		if (PlayerChar && PlayerChar->IsDead())
		{
			TargetActor = nullptr;
			SetEnemyState(EEnemyState::Patrol);
			return;
		}
	}

	// 目标在攻击范围内 → 攻击
	if (IsTargetInAttackRange())
	{
		SetEnemyState(EEnemyState::Attacking);
	}
	else
	{
		MoveToActor(TargetActor);
	}
}

void AEnemyBase::UpdateAttackingState(float DeltaTime)
{
	// 目标丢失
	if (!TargetActor || TargetActor->IsPendingKillPending())
	{
		ChaseLoseTimer -= DeltaTime;
		if (ChaseLoseTimer <= 0.0f)
		{
			TargetActor = nullptr;
			SetEnemyState(EEnemyState::Patrol);
		}
		return;
	}

	// 目标已死亡
	AShootDemoCharacter* PlayerChar = Cast<AShootDemoCharacter>(TargetActor);
	if (PlayerChar && PlayerChar->IsDead())
	{
		TargetActor = nullptr;
		SetEnemyState(EEnemyState::Patrol);
		return;
	}

	// 目标超出攻击范围 → 继续追击
	if (!IsTargetInAttackRange())
	{
		SetEnemyState(EEnemyState::Chasing);
		return;
	}

	// 面向目标
	FVector DirectionToTarget = TargetActor->GetActorLocation() - GetActorLocation();
	DirectionToTarget.Z = 0.0f;
	FRotator TargetRotation = DirectionToTarget.Rotation();
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, 10.0f));

	// 攻击冷却检查
	AttackTarget();
}

// ===== 攻击 =====

void AEnemyBase::AttackTarget()
{
	if (!HasAuthority()) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastAttackTime < AttackCooldown)
	{
		return;
	}

	LastAttackTime = CurrentTime;

	if (TargetActor)
	{
		// 对目标造成伤害
		FPointDamageEvent DamageEvent(DamageAmount, FHitResult(), GetActorForwardVector(), nullptr);
		TargetActor->TakeDamage(DamageAmount, DamageEvent, GetController(), this);

		// 多播攻击表现
		Multicast_OnAttack();

		UE_LOG(LogTemp, Verbose, TEXT("[ShootDemo] 敌人 %s 攻击 %s，造成 %.0f 伤害"),
			*GetName(), *TargetActor->GetName(), DamageAmount);
	}
}

bool AEnemyBase::IsTargetInAttackRange() const
{
	if (!TargetActor) return false;
	float Dist = FVector::Dist(GetActorLocation(), TargetActor->GetActorLocation());
	return Dist <= AttackRange;
}

AActor* AEnemyBase::FindClosestPlayer() const
{
	float ClosestDist = FLT_MAX;
	AActor* ClosestActor = nullptr;

	for (APlayerController* PC : TActorRange<APlayerController>(GetWorld()))
	{
		if (!PC) continue;

		APawn* Pawn = PC->GetPawn();
		if (!Pawn) continue;

		// 跳过已死亡的玩家
		AShootDemoCharacter* PlayerChar = Cast<AShootDemoCharacter>(Pawn);
		if (PlayerChar && PlayerChar->IsDead()) continue;

		float Dist = FVector::Dist(GetActorLocation(), Pawn->GetActorLocation());
		if (Dist < ClosestDist)
		{
			ClosestDist = Dist;
			ClosestActor = Pawn;
		}
	}

	return ClosestActor;
}

// ===== 移动 =====

void AEnemyBase::MoveToLocation(const FVector& TargetLocation)
{
	if (!HasAuthority()) return;

	AEnemyAIController* AIC = Cast<AEnemyAIController>(GetController());
	if (AIC)
	{
		AIC->MoveToLocation(TargetLocation);
	}
}

void AEnemyBase::MoveToActor(AActor* Target)
{
	if (!HasAuthority()) return;

	AEnemyAIController* AIC = Cast<AEnemyAIController>(GetController());
	if (AIC)
	{
		AIC->MoveToActor(Target);
	}
}

// ===== 感知 =====

void AEnemyBase::OnSeePawn(APawn* SeenPawn)
{
	if (!HasAuthority() || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	// 只追踪玩家角色
	AShootDemoCharacter* PlayerChar = Cast<AShootDemoCharacter>(SeenPawn);
	if (PlayerChar && !PlayerChar->IsDead())
	{
		TargetActor = SeenPawn;
		ChaseLoseTimer = ChaseLoseTargetTime;
		SetEnemyState(EEnemyState::Chasing);
		UE_LOG(LogTemp, Verbose, TEXT("[ShootDemo] 敌人 %s 发现玩家 %s"), *GetName(), *SeenPawn->GetName());
	}
}

void AEnemyBase::OnHearPawn(APawn* HeardPawn, const FVector& Location, float Volume)
{
	if (!HasAuthority() || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	// 听到声音，但不一定切换目标（视觉优先）
	AShootDemoCharacter* PlayerChar = Cast<AShootDemoCharacter>(HeardPawn);
	if (PlayerChar && !PlayerChar->IsDead() && !TargetActor)
	{
		TargetActor = HeardPawn;
		ChaseLoseTimer = ChaseLoseTargetTime * 0.5f; // 听觉追踪时间较短
		SetEnemyState(EEnemyState::Chasing);
	}
}

// ===== 生命值与伤害 =====

float AEnemyBase::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || CurrentState == EEnemyState::Dead)
	{
		return 0.0f;
	}

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);

	UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 敌人 %s 受到 %.0f 伤害，剩余生命值: %.0f"),
		*GetName(), ActualDamage, CurrentHealth);

	// 受击反应
	Multicast_OnHitReact();

	// 如果还没有目标，反击攻击者
	if (!TargetActor && EventInstigator)
	{
		TargetActor = EventInstigator->GetPawn();
		SetEnemyState(EEnemyState::Chasing);
	}

	// 检查死亡
	if (CurrentHealth <= 0.0f)
	{
		Die(EventInstigator);
	}

	return ActualDamage;
}

void AEnemyBase::Die(AController* KillerController)
{
	if (!HasAuthority() || CurrentState == EEnemyState::Dead)
	{
		return;
	}

	CurrentState = EEnemyState::Dead;

	// 通知GameMode
	AGameModeBase* GM = GetWorld()->GetAuthGameMode();
	AShootDemoGameMode* ShootGM = Cast<AShootDemoGameMode>(GM);
	if (ShootGM)
	{
		ShootGM->OnEnemyKilled(KillerController);
	}

	// 多播死亡效果
	Multicast_OnDeath();

	// 禁用碰撞和AI
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();

	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
		AIC->UnPossess();
	}

	// 延迟销毁
	SetLifeSpan(5.0f); // 5秒后自动销毁（尸体表现时间）

	UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 敌人 %s 被击杀"), *GetName());
}

// ===== 网络复制 =====

void AEnemyBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnemyBase, CurrentHealth);
	DOREPLIFETIME(AEnemyBase, CurrentState);
	DOREPLIFETIME(AEnemyBase, TargetActor);
}

void AEnemyBase::OnRep_CurrentHealth()
{
	// 客户端更新血条
	UE_LOG(LogTemp, Verbose, TEXT("[ShootDemo] 客户端敌人生命值更新: %.0f"), CurrentHealth);
}

void AEnemyBase::OnRep_EnemyState()
{
	// 客户端收到状态变化，播放对应动画
	switch (CurrentState)
	{
	case EEnemyState::Chasing:
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed;
		break;
	case EEnemyState::Dead:
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->DisableMovement();
		break;
	default:
		GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
		break;
	}
}

// ===== 多播RPC =====

void AEnemyBase::Multicast_OnAttack_Implementation()
{
	if (AttackAnimation)
	{
		GetMesh()->GetAnimInstance()->Montage_Play(AttackAnimation);
	}
	if (AttackSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackSound, GetActorLocation());
	}
}

void AEnemyBase::Multicast_OnHitReact_Implementation()
{
	if (HitReactAnimation)
	{
		GetMesh()->GetAnimInstance()->Montage_Play(HitReactAnimation);
	}
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
	}
}

void AEnemyBase::Multicast_OnDeath_Implementation()
{
	if (DeathAnimation)
	{
		GetMesh()->GetAnimInstance()->Montage_Play(DeathAnimation);
	}
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, GetActorLocation());
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 隐藏血条
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}
}

void AEnemyBase::Multicast_OnStateChanged_Implementation(EEnemyState NewState)
{
	// 客户端处理状态变化的表现
	// 动画切换等在OnRep_EnemyState中处理
}
