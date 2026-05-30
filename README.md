# ShootDemo — UE5 第一人称射击多人对战Demo

## 项目简介

基于UE5第一人称模板开发的多人竞技场射击游戏。

### 核心玩法
- **击杀敌人得分**：击败AI敌人获得1分，击杀其他玩家获得3分
- **率先达到30分获胜**：目标分数可在GameMode蓝图中配置
- **敌人AI**：在导航网格上随机生成，自动巡逻、发现玩家后追击并攻击
- **多人联机**：支持Listen Server模式，最多4人对战

### 技术特性
- **C++ 驱动**：所有逻辑用C++实现，蓝图层只做资源配置
- **服务器权威**：所有伤害、计分、AI移动在服务器端执行
- **Enhanced Input**：使用UE5新版输入系统
- **Pawn Sensing**：敌人通过视觉+听觉感知玩家
- **导航网格**：敌人AI使用导航系统自动寻路

---

## 文件结构

```
ShootDemo/
├── ShootDemo.uproject
├── README.md
├── Config/
│   ├── DefaultEngine.ini          # 引擎配置（网络、导航）
│   ├── DefaultGame.ini            # 游戏配置（地图、模式）
│   └── DefaultInput.ini           # 输入配置（备用）
├── Source/
│   ├── ShootDemo.Target.cs
│   ├── ShootDemoEditor.Target.cs
│   └── ShootDemo/
│       ├── ShootDemo.Build.cs     # 模块构建配置
│       ├── ShootDemo.h/.cpp       # 模块入口
│       ├── Core/
│       │   ├── ShootDemoGameMode.h/.cpp        # 游戏模式
│       │   ├── ShootDemoGameState.h/.cpp       # 游戏状态(复制)
│       │   ├── ShootDemoPlayerState.h/.cpp     # 玩家状态(复制)
│       │   ├── ShootDemoPlayerController.h/.cpp # 玩家控制器
│       │   ├── ShootDemoHUD.h/.cpp             # HUD绘制
│       │   └── ShootDemoGameInstance.h/.cpp    # 多人会话管理
│       ├── Character/
│       │   └── ShootDemoCharacter.h/.cpp       # 第一人称角色
│       ├── Enemy/
│       │   ├── EnemyBase.h/.cpp                # 敌人角色+AI状态机
│       │   └── EnemyAIController.h/.cpp        # AI控制器
│       ├── Weapon/
│       │   └── WeaponBase.h/.cpp               # 武器+射击系统
│       └── Pickup/
│           ├── PickupBase.h/.cpp               # 拾取物基类
│           ├── HealthPickup.h/.cpp             # 生命值拾取
│           └── AmmoPickup.h/.cpp               # 弹药拾取
```

---

## 构建与运行

### 前置条件
- **Unreal Engine 5.3+** 已安装
- **Visual Studio 2022**（Windows）

### 步骤

1. **生成项目文件**
   右键点击 `ShootDemo.uproject` → `Generate Visual Studio project files`

2. **编译项目**
   - 双击 `ShootDemo.uproject`，在UE编辑器中打开
   - 或在 Visual Studio 中打开 `ShootDemo.sln` 编译

3. **创建蓝图资源**（详见下方蓝图配置清单）

4. **运行游戏**
   - 单机测试：在编辑器中点击 Play
   - 多人测试：Play as → Net Mode → Play as Listen Server，然后通过命令行加入

---

## 蓝图配置清单

在编辑器中创建以下蓝图资产：

### 1. 游戏蓝图
| 蓝图名称 | 父类 | 路径 |
|---------|------|------|
| `BP_ShootDemoGameMode` | `ShootDemoGameMode` | `/Game/Blueprints/` |
| `BP_ShootDemoGameInstance` | `ShootDemoGameInstance` | `/Game/Blueprints/` |
| `BP_ShootDemoCharacter` | `ShootDemoCharacter` | `/Game/Blueprints/` |
| `BP_EnemyBase` | `EnemyBase` | `/Game/Blueprints/` |
| `BP_WeaponBase` | `WeaponBase` | `/Game/Blueprints/` |

### 2. 输入资产
| 资产名称 | 类型 | 说明 |
|---------|------|------|
| `IMC_Default` | Input Mapping Context | 映射Move/Look/Fire/Reload/Jump |
| `IA_Move` | Input Action | Value Type: Axis2D |
| `IA_Look` | Input Action | Value Type: Axis2D |
| `IA_Fire` | Input Action | Value Type: Digital/Bool |
| `IA_Reload` | Input Action | Value Type: Digital/Bool |
| `IA_Jump` | Input Action | Value Type: Digital/Bool |

### 3. 地图
| 地图名称 | 说明 |
|---------|------|
| `MainArena` | 主游戏关卡（需要NavMeshBoundsVolume包裹） |
| `MainMenu` | 主菜单关卡 |

### 4. UMG Widget
| Widget名称 | 说明 |
|-----------|------|
| `WBP_GameHUD` | 游戏中HUD（分数、生命、弹药） |
| `WBP_VictoryScreen` | 胜利画面 |

---

## 多人联机说明

### 托管游戏（主机）
在 `BP_ShootDemoGameInstance` 中调用 `HostGame` 函数。

### 加入游戏
调用 `FindGames` → 选择房间 → `JoinGame`。

### 多人测试命令行
```batch
# 主机（Listen Server）
# 直接在编辑器中 Play as Listen Server

# 客户端加入（在另一台机器或另一个进程）
# 通过UE5命令行：
UnrealEditor.exe ShootDemo.uproject 192.168.1.XXX -game

# 或打包后：
ShootDemo.exe 192.168.1.XXX
```

---

## 网络架构说明

| 数据 | 复制方式 |
|------|---------|
| 玩家位置/旋转 | 自动(CharacterMovement) |
| 玩家生命值 | `DOREPLIFETIME` |
| 玩家分数/击杀/死亡 | `DOREPLIFETIME`(PlayerState) |
| 敌人位置/状态 | 自动 + `DOREPLIFETIME` |
| 武器弹药 | `DOREPLIFETIME` |
| 射击请求 | Server RPC |
| 开火/命中特效 | Multicast RPC |
| 比赛开始/结束 | Multicast RPC |

---

## 自定义配置

在 `BP_ShootDemoGameMode` 详情面板中可调整：
- **TargetScore**：获胜所需分数（默认30）
- **MaxEnemiesAlive**：同时存活敌人数上限（默认8）
- **EnemySpawnInterval**：敌人生成间隔（秒，默认5）
- **EnemyClasses**：可生成的敌人蓝图列表
- **KillScore**：击杀敌人得分（默认1）
- **PlayerKillScore**：击杀玩家得分（默认3）

---

## 开发日志

- ✅ 项目骨架 + 配置
- ✅ 核心框架（GameMode/GameState/PlayerState）
- ✅ 第一人称角色（移动/射击/受伤/死亡重生）
- ✅ 武器系统（射线检测/弹药/特效/网络）
- ✅ 敌人AI（状态机/感知/追击/攻击）
- ✅ 拾取物（生命值/弹药）
- ✅ 多人联机（会话管理/Join/Host）
- ✅ HUD（生命/弹药/分数/准星）
