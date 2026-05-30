// ShootDemoPlayerState.h — 玩家状态：记录击杀/死亡/分数，支持网络复制

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShootDemoPlayerState.generated.h"

UCLASS()
class SHOOTDEMO_API AShootDemoPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AShootDemoPlayerState();

	/** 添加击杀数 */
	void AddKill();

	/** 添加死亡数 */
	void AddDeath();

	/** 添加分数（带有网络复制） */
	void AddScorePoints(int32 Points);

	// ===== 属性 =====
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|PlayerState")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintCallable, Category = "ShootDemo|PlayerState")
	int32 GetDeaths() const { return Deaths; }

protected:
	/** 击杀数 — 复制到所有客户端 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Stats")
	int32 Kills;

	/** 死亡数 — 复制到所有客户端 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ShootDemo|Stats")
	int32 Deaths;

	// ===== 网络复制 =====
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
