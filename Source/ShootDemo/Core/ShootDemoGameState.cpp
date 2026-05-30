// ShootDemoGameState.cpp — 游戏状态实现

#include "ShootDemoGameState.h"
#include "Net/UnrealNetwork.h"

AShootDemoGameState::AShootDemoGameState()
{
	// 允许Tick
	PrimaryActorTick.bCanEverTick = true;

	bMatchOver = false;
	bMatchInProgress = false;
	MatchElapsedTime = 0.0f;
}

void AShootDemoGameState::StartMatch()
{
	if (HasAuthority())
	{
		bMatchInProgress = true;
		bMatchOver = false;
		MatchElapsedTime = 0.0f;
		Multicast_OnMatchStart();
	}
}

void AShootDemoGameState::EndMatch(const FString& WinnerName)
{
	if (HasAuthority() && !bMatchOver)
	{
		bMatchOver = true;
		bMatchInProgress = false;
		WinnerPlayerName = WinnerName;
		Multicast_OnMatchEnd(WinnerName);
	}
}

void AShootDemoGameState::Multicast_OnMatchEnd_Implementation(const FString& InWinnerName)
{
	// 所有客户端收到此RPC后广播委托
	OnMatchEnded.Broadcast(InWinnerName);
}

void AShootDemoGameState::Multicast_OnMatchStart_Implementation()
{
	OnMatchStarted.Broadcast();
}

void AShootDemoGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && bMatchInProgress && !bMatchOver)
	{
		MatchElapsedTime += DeltaSeconds;
	}
}

void AShootDemoGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShootDemoGameState, bMatchOver);
	DOREPLIFETIME(AShootDemoGameState, MatchElapsedTime);
	DOREPLIFETIME(AShootDemoGameState, WinnerPlayerName);
	DOREPLIFETIME(AShootDemoGameState, bMatchInProgress);
}
