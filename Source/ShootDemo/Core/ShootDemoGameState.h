// ShootDemoGameState.h — 游戏状态：比赛时间、比分、胜利信息，支持网络复制

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ShootDemoGameState.generated.h"

// 比赛状态变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchEnded, const FString&, WinnerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchStarted);

UCLASS()
class SHOOTDEMO_API AShootDemoGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AShootDemoGameState();

	/** 开始比赛 */
	void StartMatch();

	/** 结束比赛，通知所有客户端 */
	void EndMatch(const FString& WinnerName);

	/** 检查比赛是否已结束 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|GameState")
	bool IsMatchOver() const { return bMatchOver; }

	/** 获取获胜者名字 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|GameState")
	FString GetWinnerName() const { return WinnerPlayerName; }

	// ===== 委托 =====
	UPROPERTY(BlueprintAssignable, Category = "ShootDemo|Events")
	FOnMatchEnded OnMatchEnded;

	UPROPERTY(BlueprintAssignable, Category = "ShootDemo|Events")
	FOnMatchStarted OnMatchStarted;

protected:
	/** 比赛是否已结束 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Match")
	bool bMatchOver;

	/** 比赛已用时间（秒） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Match")
	float MatchElapsedTime;

	/** 获胜者名字 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Match")
	FString WinnerPlayerName;

	/** 比赛是否正在计时 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Match")
	bool bMatchInProgress;

public:
	// ===== 蓝图和网络 =====

	/** 多播RPC：通知所有客户端比赛结束 */
	UFUNCTION(NetMulticast, Reliable, Category = "ShootDemo|Network")
	void Multicast_OnMatchEnd(const FString& InWinnerName);

	/** 多播RPC：通知所有客户端比赛开始 */
	UFUNCTION(NetMulticast, Reliable, Category = "ShootDemo|Network")
	void Multicast_OnMatchStart();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 每帧更新比赛计时
	virtual void Tick(float DeltaSeconds) override;
};
