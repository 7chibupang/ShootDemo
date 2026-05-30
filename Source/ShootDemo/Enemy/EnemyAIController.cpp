// EnemyAIController.cpp — AI控制器实现

#include "EnemyAIController.h"
#include "EnemyBase.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Navigation/PathFollowingComponent.h"

AEnemyAIController::AEnemyAIController()
{
	// 配置AI感知（可选：如果需要行为树感知系统）
	bSetControlRotationFromPawnOrientation = true;
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	// 如果有行为树则运行
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AEnemyAIController::MoveToTarget(const FVector& TargetLocation)
{
	AEnemyBase* Enemy = Cast<AEnemyBase>(GetPawn());
	if (!Enemy)
	{
		return;
	}

	// 使用AI移动
	MoveToLocation(TargetLocation, 50.0f, true, true, false, true);
}

void AEnemyAIController::StopAIMovement()
{
	StopMovement();
}
