// ShootDemoPlayerController.cpp — 玩家控制器实现

#include "ShootDemoPlayerController.h"
#include "ShootDemoGameState.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/GameModeBase.h"

AShootDemoPlayerController::AShootDemoPlayerController()
{
}

void AShootDemoPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 绑定比赛结束事件
	AShootDemoGameState* GS = GetWorld()->GetGameState<AShootDemoGameState>();
	if (GS)
	{
		GS->OnMatchEnded.AddDynamic(this, &AShootDemoPlayerController::Client_OnMatchEnd);
	}

	// 仅在本地玩家控制器的客户端创建HUD
	if (IsLocalPlayerController())
	{
		CreateHUDWidget();
	}
}

void AShootDemoPlayerController::CreateHUDWidget()
{
	if (HUDWidgetClass && IsLocalPlayerController())
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}
}

void AShootDemoPlayerController::Client_OnMatchEnd_Implementation(const FString& WinnerName)
{
	// 显示胜利画面
	if (VictoryWidgetClass && IsLocalPlayerController())
	{
		VictoryWidget = CreateWidget<UUserWidget>(this, VictoryWidgetClass);
		if (VictoryWidget)
		{
			VictoryWidget->AddToViewport(10); // 高ZOrder确保在最上层
		}
	}
}

void AShootDemoPlayerController::Client_ShowKillNotification_Implementation(const FString& VictimName)
{
	// 显示击杀提示（在HUD Widget中处理）
	UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 击杀提示: 你击杀了 %s"), *VictimName);
}

void AShootDemoPlayerController::Client_ShowMessage_Implementation(const FString& Message, float Duration)
{
	UE_LOG(LogTemp, Log, TEXT("[ShootDemo] HUD消息: %s"), *Message);
}
