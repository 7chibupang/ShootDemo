// EnemyAIController.h — AI控制器：驱动敌人导航和基础行动

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

UCLASS()
class SHOOTDEMO_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	virtual void BeginPlay() override;

	/** 命令AI移动到目标位置 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|AI")
	void MoveToTarget(const FVector& TargetLocation);

	/** 停止移动 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|AI")
	void StopAIMovement();

protected:
	/** 行为树资产（在蓝图中设置） */
	UPROPERTY(EditDefaultsOnly, Category = "ShootDemo|AI")
	class UBehaviorTree* BehaviorTreeAsset;

	/** 黑板资产 */
	UPROPERTY(EditDefaultsOnly, Category = "ShootDemo|AI")
	class UBlackboardData* BlackboardAsset;
};
