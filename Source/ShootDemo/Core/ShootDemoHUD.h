// ShootDemoHUD.h — HUD：屏幕UI绘制（后备，主要UI使用UMG Widget）

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShootDemoHUD.generated.h"

UCLASS()
class SHOOTDEMO_API AShootDemoHUD : public AHUD
{
	GENERATED_BODY()

public:
	AShootDemoHUD();

	/** 主绘制函数 */
	virtual void DrawHUD() override;

	/** 显示伤害数字 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|HUD")
	void ShowDamageNumber(float Damage, FVector WorldLocation, bool bIsHeadshot = false);

protected:
	/** 绘制准星 */
	void DrawCrosshair();

	/** 绘制生命值 */
	void DrawHealthBar();

	/** 绘制弹药信息 */
	void DrawAmmoInfo();

	/** 绘制分数信息 */
	void DrawScoreInfo();

	/** 准星纹理 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|HUD")
	UTexture2D* CrosshairTexture;

	/** 准星大小 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ShootDemo|HUD")
	float CrosshairSize;

	/** 伤害数字结构 */
	struct FDamageNumber
	{
		float Damage;
		FVector2D ScreenPosition;
		float Lifetime;
		float MaxLifetime;
		bool bIsHeadshot;
	};

	/** 激活的伤害数字列表 */
	TArray<FDamageNumber> DamageNumbers;
};
