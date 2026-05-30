// ShootDemoGameMode.h — 游戏模式：计分、胜利条件、敌人生成管理

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShootDemoGameMode.generated.h"

class AEnemyBase;

UCLASS()
class SHOOTDEMO_API AShootDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShootDemoGameMode();

	virtual void BeginPlay() override;

	// ===== 计分与胜利条件 =====

	/** 给指定玩家加分，服务器端执行 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Scoring")
	void AddScore(class AController* ScoredPlayer, int32 Points);

	/** 检查是否有人达到目标分数 */
	void CheckWinCondition();

	/** 获取目标分数 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Scoring")
	int32 GetTargetScore() const { return TargetScore; }

	// ===== 敌人生成管理 =====

	/** 在导航网格上随机位置生成敌人 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Enemy")
	void SpawnEnemy();

	/** 敌人被击杀时回调 */
	void OnEnemyKilled(class AController* KillerController);

	// ===== 玩家管理 =====

	/** 玩家被杀时的处理 */
	void OnPlayerKilled(class AController* KilledController, class AController* KillerController);

	/** 重生玩家 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Player")
	void RespawnPlayer(class AController* PlayerController);

	/** 处理PostLogin — 新玩家加入 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	// ===== 可配置属性 =====

	/** 获胜所需分数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Scoring")
	int32 TargetScore;

	/** 击杀敌人获得的分数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Scoring")
	int32 KillScore;

	/** 击杀玩家获得的分数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Scoring")
	int32 PlayerKillScore;

	// ===== 敌人生成 =====

	/** 敌人生成间隔（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy")
	float EnemySpawnInterval;

	/** 同时存活的最大敌人数量 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy")
	int32 MaxEnemiesAlive;

	/** 可生成的敌人类型列表 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy")
	TArray<TSubclassOf<AEnemyBase>> EnemyClasses;

	/** 敌人出生最小距离（距离玩家） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy")
	float MinSpawnDistanceFromPlayers;

	/** 敌人出生最大距离（距离玩家） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|Enemy")
	float MaxSpawnDistanceFromPlayers;

private:
	/** 敌人生成定时器句柄 */
	FTimerHandle EnemySpawnTimerHandle;

	/** 当前存活敌人数量 */
	int32 CurrentEnemyCount;

	/** 获取随机出生位置（在导航网格上） */
	FVector GetRandomSpawnLocation();

	/** 检查位置是否远离所有玩家 */
	bool IsLocationFarFromPlayers(const FVector& Location, float MinDistance) const;

	/** 比赛是否已经开始 */
	bool bMatchStarted;
};
