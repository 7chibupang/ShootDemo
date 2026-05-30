// ShootDemoGameMode.cpp — 游戏模式实现

#include "ShootDemoGameMode.h"
#include "ShootDemoGameState.h"
#include "ShootDemoPlayerState.h"
#include "ShootDemoPlayerController.h"
#include "../Enemy/EnemyBase.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AShootDemoGameMode::AShootDemoGameMode()
{
	// 设置默认类
	PlayerStateClass = AShootDemoPlayerState::StaticClass();
	GameStateClass = AShootDemoGameState::StaticClass();
	PlayerControllerClass = AShootDemoPlayerController::StaticClass();

	// 默认配置值
	TargetScore = 30;
	KillScore = 1;
	PlayerKillScore = 3;
	EnemySpawnInterval = 5.0f;
	MaxEnemiesAlive = 8;
	MinSpawnDistanceFromPlayers = 1000.0f;
	MaxSpawnDistanceFromPlayers = 3000.0f;

	CurrentEnemyCount = 0;
	bMatchStarted = false;
}

void AShootDemoGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 开始比赛
		AShootDemoGameState* GS = GetGameState<AShootDemoGameState>();
		if (GS)
		{
			GS->StartMatch();
		}

		bMatchStarted = true;

		// 启动敌人生成定时器
		GetWorldTimerManager().SetTimer(
			EnemySpawnTimerHandle,
			this,
			&AShootDemoGameMode::SpawnEnemy,
			EnemySpawnInterval,
			true // 循环
		);

		// 立即生成第一批敌人
		for (int32 i = 0; i < 3; i++)
		{
			SpawnEnemy();
		}
	}
}

void AShootDemoGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (HasAuthority())
	{
		// 新玩家加入时重生
		RespawnPlayer(NewPlayer);
	}
}

// ===== 计分系统 =====

void AShootDemoGameMode::AddScore(AController* ScoredPlayer, int32 Points)
{
	if (!HasAuthority() || !ScoredPlayer)
	{
		return;
	}

	AShootDemoPlayerState* PS = ScoredPlayer->GetPlayerState<AShootDemoPlayerState>();
	if (PS)
	{
		PS->AddScorePoints(Points);
		UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 玩家 %s 获得 %d 分，当前总分: %.0f"),
			*PS->GetPlayerName(), Points, PS->GetScore());

		// 检查胜利条件
		CheckWinCondition();
	}
}

void AShootDemoGameMode::CheckWinCondition()
{
	if (!HasAuthority())
	{
		return;
	}

	AShootDemoGameState* GS = GetGameState<AShootDemoGameState>();
	if (!GS || GS->IsMatchOver())
	{
		return;
	}

	// 遍历所有玩家，检查是否有人达到目标分数
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (PS && PS->GetScore() >= TargetScore)
		{
			GS->EndMatch(PS->GetPlayerName());
			UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 玩家 %s 获胜！分数: %.0f"), *PS->GetPlayerName(), PS->GetScore());
			return;
		}
	}
}

// ===== 敌人生成 =====

void AShootDemoGameMode::SpawnEnemy()
{
	if (!HasAuthority())
	{
		return;
	}

	// 检查数量上限
	if (CurrentEnemyCount >= MaxEnemiesAlive || EnemyClasses.Num() == 0)
	{
		return;
	}

	FVector SpawnLocation = GetRandomSpawnLocation();
	if (SpawnLocation.IsZero())
	{
		// 未找到合适位置
		return;
	}

	// 随机选择敌人类型
	int32 RandomIndex = FMath::RandRange(0, EnemyClasses.Num() - 1);
	TSubclassOf<AEnemyBase> EnemyClass = EnemyClasses[RandomIndex];

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnemyBase* NewEnemy = GetWorld()->SpawnActor<AEnemyBase>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (NewEnemy)
	{
		CurrentEnemyCount++;
		UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 敌人生成于: %s，当前存活: %d"), *SpawnLocation.ToString(), CurrentEnemyCount);
	}
}

FVector AShootDemoGameMode::GetRandomSpawnLocation()
{
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSystem)
	{
		return FVector::ZeroVector;
	}

	// 尝试多次寻找合适位置
	for (int32 Attempt = 0; Attempt < 10; Attempt++)
	{
		FNavLocation NavLocation;
		if (NavSystem->GetRandomReachablePointInRadius(
			FVector::ZeroVector, // 以原点为中心搜索
			5000.0f,             // 搜索半径
			NavLocation))
		{
			if (IsLocationFarFromPlayers(NavLocation.Location, MinSpawnDistanceFromPlayers))
			{
				return NavLocation.Location;
			}
		}
	}

	// 如果10次都没找到符合条件的，返回任意可达位置
	FNavLocation FallbackLocation;
	if (NavSystem->GetRandomReachablePointInRadius(FVector::ZeroVector, 5000.0f, FallbackLocation))
	{
		return FallbackLocation.Location;
	}

	return FVector::ZeroVector;
}

bool AShootDemoGameMode::IsLocationFarFromPlayers(const FVector& Location, float MinDistance) const
{
	for (APlayerController* PC : TActorRange<APlayerController>(GetWorld()))
	{
		if (!PC) continue;

		APawn* Pawn = PC->GetPawn();
		if (Pawn)
		{
			float Dist = FVector::Dist(Location, Pawn->GetActorLocation());
			if (Dist < MinDistance)
			{
				return false;
			}
		}
	}
	return true;
}

void AShootDemoGameMode::OnEnemyKilled(AController* KillerController)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentEnemyCount = FMath::Max(0, CurrentEnemyCount - 1);

	// 给击杀者加分
	AddScore(KillerController, KillScore);
}

// ===== 玩家管理 =====

void AShootDemoGameMode::OnPlayerKilled(AController* KilledController, AController* KillerController)
{
	if (!HasAuthority() || !KilledController)
	{
		return;
	}

	// 记录被击杀者死亡
	AShootDemoPlayerState* KilledPS = KilledController->GetPlayerState<AShootDemoPlayerState>();
	if (KilledPS)
	{
		KilledPS->AddDeath();
	}

	// 给击杀者加分
	if (KillerController && KillerController != KilledController)
	{
		AShootDemoPlayerState* KillerPS = KillerController->GetPlayerState<AShootDemoPlayerState>();
		if (KillerPS)
		{
			KillerPS->AddKill();
		}
		AddScore(KillerController, PlayerKillScore);
	}

	// 延迟重生
	FTimerHandle RespawnTimer;
	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(this, &AShootDemoGameMode::RespawnPlayer, KilledController);
	GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, 3.0f, false);
}

void AShootDemoGameMode::RespawnPlayer(AController* PlayerController)
{
	if (!HasAuthority() || !PlayerController)
	{
		return;
	}

	// 找到PlayerStart或随机位置
	AActor* PlayerStart = FindPlayerStart(PlayerController);
	if (PlayerStart)
	{
		APawn* NewPawn = SpawnDefaultPawnFor(PlayerController, PlayerStart);
		if (NewPawn)
		{
			PlayerController->Possess(NewPawn);
		}
	}
}
