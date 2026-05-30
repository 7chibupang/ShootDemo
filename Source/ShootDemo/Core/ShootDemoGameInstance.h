// ShootDemoGameInstance.h — 游戏实例：处理多人会话创建/查找/加入

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ShootDemoGameInstance.generated.h"

UCLASS()
class SHOOTDEMO_API UShootDemoGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UShootDemoGameInstance();

	virtual void Init() override;

	// ===== 多人会话 =====

	/** 创建/托管游戏 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Multiplayer")
	void HostGame(int32 MaxPlayers = 4, bool bIsLAN = true);

	/** 查找可用游戏 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Multiplayer")
	void FindGames(bool bIsLAN = true);

	/** 加入指定索引的游戏 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Multiplayer")
	void JoinGame(int32 SessionIndex);

	/** 离开游戏 */
	UFUNCTION(BlueprintCallable, Category = "ShootDemo|Multiplayer")
	void LeaveGame();

	// ===== 事件委托 =====

	/** 房间创建成功 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHostGameComplete);
	UPROPERTY(BlueprintAssignable, Category = "ShootDemo|Events")
	FOnHostGameComplete OnHostGameComplete;

	/** 找到游戏列表更新 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindGamesComplete, const TArray<FString>&, GameNames);
	UPROPERTY(BlueprintAssignable, Category = "ShootDemo|Events")
	FOnFindGamesComplete OnFindGamesComplete;

	/** 加入游戏成功 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJoinGameComplete);
	UPROPERTY(BlueprintAssignable, Category = "ShootDemo|Events")
	FOnJoinGameComplete OnJoinGameComplete;

	/** 加入游戏失败 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinGameFailed, const FString&, ErrorMessage);
	UPROPERTY(BlueprintAssignable, Category = "ShootDemo|Events")
	FOnJoinGameFailed OnJoinGameFailed;

protected:
	/** 会话接口 */
	IOnlineSessionPtr SessionInterface;

	/** 搜索结果 */
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// ===== 回调 =====

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/** 地图名称 */
	UPROPERTY(EditDefaultsOnly, Category = "ShootDemo|Multiplayer")
	FString LobbyMapName;
};
