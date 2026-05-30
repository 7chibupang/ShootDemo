// ShootDemoPlayerState.cpp — 玩家状态实现

#include "ShootDemoPlayerState.h"
#include "Net/UnrealNetwork.h"

AShootDemoPlayerState::AShootDemoPlayerState()
{
	Kills = 0;
	Deaths = 0;
}

void AShootDemoPlayerState::AddKill()
{
	if (HasAuthority())
	{
		Kills++;
	}
}

void AShootDemoPlayerState::AddDeath()
{
	if (HasAuthority())
	{
		Deaths++;
	}
}

void AShootDemoPlayerState::AddScorePoints(int32 Points)
{
	if (HasAuthority())
	{
		// 使用APlayerState内置的Score属性（已自动复制）
		SetScore(GetScore() + Points);
	}
}

void AShootDemoPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShootDemoPlayerState, Kills);
	DOREPLIFETIME(AShootDemoPlayerState, Deaths);
}
