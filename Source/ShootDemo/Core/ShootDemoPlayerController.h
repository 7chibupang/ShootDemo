// ShootDemoPlayerController.h — 玩家控制器

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShootDemoPlayerController.generated.h"

UCLASS()
class SHOOTDEMO_API AShootDemoPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AShootDemoPlayerController();

	virtual void BeginPlay() override;

	// ===== UI控制 =====

	/** 客户端收到比赛结束通知 */
	UFUNCTION(Client, Reliable, Category = "ShootDemo|UI")
	void Client_OnMatchEnd(const FString& WinnerName);

	/** 显示击杀提示 */
	UFUNCTION(Client, Reliable, Category = "ShootDemo|UI")
	void Client_ShowKillNotification(const FString& VictimName);

	/** 显示HUD消息 */
	UFUNCTION(Client, Reliable, Category = "ShootDemo|UI")
	void Client_ShowMessage(const FString& Message, float Duration = 2.0f);

protected:
	/** 创建HUD Widget */
	void CreateHUDWidget();

	/** 胜利画面Widget类（在蓝图中设置） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|UI")
	TSubclassOf<class UUserWidget> HUDWidgetClass;

	/** 胜利画面Widget类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|UI")
	TSubclassOf<class UUserWidget> VictoryWidgetClass;

	/** HUD Widget实例 */
	UPROPERTY(BlueprintReadOnly, Category = "ShootDemo|UI")
	class UUserWidget* HUDWidget;

	/** 胜利画面Widget实例 */
	UPROPERTY(BlueprintReadOnly, Category = "ShootDemo|UI")
	class UUserWidget* VictoryWidget;
};
