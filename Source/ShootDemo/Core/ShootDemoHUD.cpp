// ShootDemoHUD.cpp — HUD实现

#include "ShootDemoHUD.h"
#include "ShootDemoPlayerState.h"
#include "../Character/ShootDemoCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "CanvasItem.h"
#include "UObject/ConstructorHelpers.h"

AShootDemoHUD::AShootDemoHUD()
{
	CrosshairSize = 24.0f;
}

void AShootDemoHUD::DrawHUD()
{
	Super::DrawHUD();

	DrawCrosshair();
	DrawHealthBar();
	DrawAmmoInfo();
	DrawScoreInfo();

	// 绘制伤害数字
	for (int32 i = DamageNumbers.Num() - 1; i >= 0; i--)
	{
		FDamageNumber& DmgNum = DamageNumbers[i];
		DmgNum.Lifetime -= GetWorld()->GetDeltaSeconds();
		if (DmgNum.Lifetime <= 0.0f)
		{
			DamageNumbers.RemoveAt(i);
			continue;
		}

		float Alpha = DmgNum.Lifetime / DmgNum.MaxLifetime;
		FVector2D DrawPos = DmgNum.ScreenPosition;
		DrawPos.Y -= (1.0f - Alpha) * 50.0f; // 向上浮动

		FLinearColor TextColor = DmgNum.bIsHeadshot ? FLinearColor::Yellow : FLinearColor::White;
		TextColor.A = Alpha;

		FString DamageText = FString::Printf(TEXT("%.0f"), DmgNum.Damage);
		if (DmgNum.bIsHeadshot)
		{
			DamageText += TEXT(" HEADSHOT");
		}

		DrawText(DamageText, TextColor, DrawPos.X, DrawPos.Y, nullptr, 1.5f);
	}
}

void AShootDemoHUD::DrawCrosshair()
{
	float CenterX = Canvas->ClipX * 0.5f;
	float CenterY = Canvas->ClipY * 0.5f;

	if (CrosshairTexture)
	{
		float HalfSize = CrosshairSize * 0.5f;
		FCanvasTileItem TileItem(
			FVector2D(CenterX - HalfSize, CenterY - HalfSize),
			CrosshairTexture->GetResource(),
			FVector2D(CrosshairSize, CrosshairSize),
			FLinearColor::White
		);
		TileItem.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(TileItem);
	}
	else
	{
		// 后备：绘制简单十字准星
		FLinearColor CrosshairColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.8f);
		float Gap = 12.0f;
		float Length = 10.0f;
		float Thickness = 2.0f;

		// 上线
		DrawRect(CrosshairColor, CenterX - Thickness * 0.5f, CenterY - Gap - Length, Thickness, Length);
		// 下线
		DrawRect(CrosshairColor, CenterX - Thickness * 0.5f, CenterY + Gap, Thickness, Length);
		// 左线
		DrawRect(CrosshairColor, CenterX - Gap - Length, CenterY - Thickness * 0.5f, Length, Thickness);
		// 右线
		DrawRect(CrosshairColor, CenterX + Gap, CenterY - Thickness * 0.5f, Length, Thickness);
	}
}

void AShootDemoHUD::DrawHealthBar()
{
	AShootDemoCharacter* Player = Cast<AShootDemoCharacter>(GetOwningPawn());
	if (!Player)
	{
		return;
	}

	float HealthPercent = Player->GetCurrentHealth() / Player->GetMaxHealth();
	float BarWidth = 200.0f;
	float BarHeight = 20.0f;
	float BarX = 20.0f;
	float BarY = Canvas->ClipY - 60.0f;

	// 背景
	DrawRect(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f), BarX, BarY, BarWidth, BarHeight);

	// 血量（红→绿渐变）
	FLinearColor HealthColor = FMath::Lerp(FLinearColor::Red, FLinearColor::Green, HealthPercent);
	DrawRect(HealthColor, BarX, BarY, BarWidth * HealthPercent, BarHeight);

	// 文字
	FString HealthText = FString::Printf(TEXT("生命值: %.0f / %.0f"), Player->GetCurrentHealth(), Player->GetMaxHealth());
	DrawText(HealthText, FLinearColor::White, BarX, BarY - 20.0f, nullptr, 1.0f);
}

void AShootDemoHUD::DrawAmmoInfo()
{
	AShootDemoCharacter* Player = Cast<AShootDemoCharacter>(GetOwningPawn());
	if (!Player)
	{
		return;
	}

	FString AmmoText = FString::Printf(TEXT("弹药: %d / %d"), Player->GetCurrentAmmo(), Player->GetMaxAmmo());
	float TextWidth, TextHeight;
	GetTextSize(AmmoText, TextWidth, TextHeight, nullptr, 1.2f);

	DrawText(AmmoText, FLinearColor::White, Canvas->ClipX - TextWidth - 20.0f, Canvas->ClipY - 50.0f, nullptr, 1.2f);
}

void AShootDemoHUD::DrawScoreInfo()
{
	AShootDemoPlayerState* PS = GetOwningPlayerController() ?
		GetOwningPlayerController()->GetPlayerState<AShootDemoPlayerState>() : nullptr;
	if (!PS)
	{
		return;
	}

	FString ScoreText = FString::Printf(TEXT("分数: %.0f | 击杀: %d | 死亡: %d"),
		PS->GetScore(), PS->GetKills(), PS->GetDeaths());

	DrawText(ScoreText, FLinearColor(1.0f, 0.8f, 0.0f, 1.0f), Canvas->ClipX * 0.5f, 20.0f, nullptr, 1.2f);
}

void AShootDemoHUD::ShowDamageNumber(float Damage, FVector WorldLocation, bool bIsHeadshot)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	FVector2D ScreenPos;
	if (PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPos))
	{
		FDamageNumber DmgNum;
		DmgNum.Damage = Damage;
		DmgNum.ScreenPosition = ScreenPos;
		DmgNum.Lifetime = 1.5f;
		DmgNum.MaxLifetime = 1.5f;
		DmgNum.bIsHeadshot = bIsHeadshot;
		DamageNumbers.Add(DmgNum);
	}
}
