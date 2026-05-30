// ShootDemoGameInstance.cpp — 游戏实例实现（多人会话管理）

#include "ShootDemoGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

UShootDemoGameInstance::UShootDemoGameInstance()
{
	LobbyMapName = TEXT("MainArena");
}

void UShootDemoGameInstance::Init()
{
	Super::Init();

	// 获取OnlineSession接口
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		SessionInterface = OnlineSub->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("[ShootDemo] OnlineSubsystem 就绪: %s"), *OnlineSub->GetSubsystemName().ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ShootDemo] 无法获取OnlineSession接口"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShootDemo] 未找到OnlineSubsystem，多人功能不可用"));
	}
}

// ===== 托管游戏 =====

void UShootDemoGameInstance::HostGame(int32 MaxPlayers, bool bIsLAN)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[ShootDemo] SessionInterface无效，无法创建房间"));
		return;
	}

	// 先销毁已有会话
	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	// 创建新会话
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = bIsLAN;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.NumPrivateConnections = 0;
	SessionSettings.bAllowInvites = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bIsDedicated = false;
	SessionSettings.Set(TEXT("GameName"), FString(TEXT("ShootDemo Arena")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UShootDemoGameInstance::OnCreateSessionComplete));

	if (!SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
	{
		UE_LOG(LogTemp, Error, TEXT("[ShootDemo] 创建会话失败"));
	}
}

void UShootDemoGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 房间创建成功！"));

		// 进入地图（作为服务器）
		GetWorld()->ServerTravel(FString::Printf(TEXT("/Game/Maps/%s?listen"), *LobbyMapName));

		OnHostGameComplete.Broadcast();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ShootDemo] 创建会话失败"));
	}
}

// ===== 查找游戏 =====

void UShootDemoGameInstance::FindGames(bool bIsLAN)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = bIsLAN;
	SessionSearch->MaxSearchResults = 50;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UShootDemoGameInstance::OnFindSessionsComplete));

	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UShootDemoGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShootDemo] 查找游戏失败"));
		return;
	}

	TArray<FString> GameNames;
	for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
	{
		FString GameName;
		Result.Session.SessionSettings.Get(TEXT("GameName"), GameName);
		GameNames.Add(GameName.IsEmpty() ? TEXT("未知房间") : GameName);
	}

	UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 找到 %d 个房间"), GameNames.Num());
	OnFindGamesComplete.Broadcast(GameNames);
}

// ===== 加入游戏 =====

void UShootDemoGameInstance::JoinGame(int32 SessionIndex)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
	{
		OnJoinGameFailed.Broadcast(TEXT("未搜索到可用游戏"));
		return;
	}

	if (SessionIndex < 0 || SessionIndex >= SessionSearch->SearchResults.Num())
	{
		OnJoinGameFailed.Broadcast(TEXT("无效的房间索引"));
		return;
	}

	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UShootDemoGameInstance::OnJoinSessionComplete));

	if (!SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SessionIndex]))
	{
		OnJoinGameFailed.Broadcast(TEXT("加入游戏失败"));
	}
}

void UShootDemoGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		OnJoinGameFailed.Broadcast(TEXT("加入游戏失败"));
		return;
	}

	// 获取服务器地址
	FString ConnectInfo;
	if (SessionInterface->GetResolvedConnectString(SessionName, ConnectInfo))
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC)
		{
			PC->ClientTravel(ConnectInfo, TRAVEL_Absolute);
			OnJoinGameComplete.Broadcast();
		}
	}
	else
	{
		OnJoinGameFailed.Broadcast(TEXT("无法获取服务器地址"));
	}
}

// ===== 离开游戏 =====

void UShootDemoGameInstance::LeaveGame()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UShootDemoGameInstance::OnDestroySessionComplete));

		SessionInterface->DestroySession(NAME_GameSession);
	}

	// 返回主菜单
	UGameplayStatics::OpenLevel(GetWorld(), TEXT("MainMenu"), true);
}

void UShootDemoGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 已离开游戏"));
}
