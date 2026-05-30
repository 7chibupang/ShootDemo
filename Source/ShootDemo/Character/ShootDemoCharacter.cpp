// ShootDemoCharacter.cpp — 第一人称角色实现

#include "ShootDemoCharacter.h"
#include "../Weapon/WeaponBase.h"
#include "../Core/ShootDemoGameMode.h"
#include "../Core/ShootDemoPlayerState.h"
#include "../Core/ShootDemoHUD.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

AShootDemoCharacter::AShootDemoCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// === 第一人称手部网格体 ===
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->SetupAttachment(GetCapsuleComponent());
	Mesh1P->SetOnlyOwnerSee(true);           // 仅自己可见
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;              // 不投射阴影
	Mesh1P->SetRelativeLocation(FVector(-30.0f, 0.0f, -150.0f));
	Mesh1P->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

	// === 第一人称摄像机 ===
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(-10.0f, 0.0f, 60.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// === 第三人称弹簧臂（观察者/旁观者使用） ===
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));

	// === 第三人称摄像机 ===
	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(SpringArm);
	ThirdPersonCamera->bUsePawnControlRotation = false;

	// === 角色移动 ===
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->JumpZVelocity = 420.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	// === 默认值 ===
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	bIsDead = false;
	RespawnDelay = 3.0f;
	RespawnInvincibilityTime = 2.0f;
	WeaponSocketName = TEXT("GripPoint");

	// Enable replication
	bReplicates = true;
}

// ===== BeginPlay =====

void AShootDemoCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 添加Enhanced Input映射
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// 装备武器（服务器端）
	if (HasAuthority())
	{
		EquipWeapon();
	}
}

void AShootDemoCharacter::EquipWeapon()
{
	if (!WeaponClass || !HasAuthority())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponClass, SpawnParams);
	if (CurrentWeapon)
	{
		CurrentWeapon->SetOwningCharacter(this);

		// 附加到第一人称手部网格体
		CurrentWeapon->AttachToComponent(Mesh1P, FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocketName);

		UE_LOG(LogTemp, Log, TEXT("[ShootDemo] 武器已装备: %s，弹药: %d/%d"),
			*CurrentWeapon->GetName(), CurrentWeapon->GetCurrentAmmo(), CurrentWeapon->GetMaxAmmo());
	}
}

void AShootDemoCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 摄像机始终使用控制旋转
	if (FirstPersonCamera)
	{
		FirstPersonCamera->bUsePawnControlRotation = true;
	}
}

// ===== 控制旋转（用于射击方向） =====

FRotator AShootDemoCharacter::GetControlRotation() const
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		return PC->GetControlRotation();
	}
	// 对于AI或非玩家角色，使用Actor旋转
	return GetActorRotation();
}

// ===== 生命值系统 =====

void AShootDemoCharacter::Heal(float Amount)
{
	if (HasAuthority())
	{
		CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);
	}
}

float AShootDemoCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority())
	{
		return 0.0f;
	}

	// 已死亡或无敌期间不受伤害
	if (bIsDead)
	{
		return 0.0f;
	}

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - ActualDamage);

	UE_LOG(LogTemp, Log, TEXT("[ShootDemo] %s 受到 %.0f 伤害，剩余生命值: %.0f"),
		*GetName(), ActualDamage, CurrentHealth);

	// 多播受击反应
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointEvent = (FPointDamageEvent*)(&DamageEvent);
		Multicast_OnHitReact(PointEvent->HitInfo.Location);
	}

	// 显示伤害数字
	if (EventInstigator)
	{
		APlayerController* PC = Cast<APlayerController>(EventInstigator);
		if (PC && PC->GetHUD())
		{
			AShootDemoHUD* HUD = Cast<AShootDemoHUD>(PC->GetHUD());
			if (HUD)
			{
				bool bIsHeadshot = DamageEvent.IsOfType(FPointDamageEvent::ClassID) &&
					((FPointDamageEvent*)(&DamageEvent))->HitInfo.BoneName.ToString().Contains(TEXT("head"));
				HUD->ShowDamageNumber(ActualDamage, GetActorLocation(), bIsHeadshot);
			}
		}
	}

	// 检查死亡
	if (CurrentHealth <= 0.0f)
	{
		Die(EventInstigator);
	}

	return ActualDamage;
}

void AShootDemoCharacter::Die(AController* KillerController)
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;

	// 通知GameMode处理击杀
	AGameModeBase* GM = GetWorld()->GetAuthGameMode();
	AShootDemoGameMode* ShootGM = Cast<AShootDemoGameMode>(GM);
	if (ShootGM)
	{
		ShootGM->OnPlayerKilled(GetController(), KillerController);
	}

	// 多播死亡效果
	Multicast_OnDeath();

	// 通知击杀者
	if (KillerController)
	{
		AShootDemoPlayerController* KillerPC = Cast<AShootDemoPlayerController>(KillerController);
		if (KillerPC)
		{
			AShootDemoPlayerState* KilledPS = GetPlayerState<AShootDemoPlayerState>();
			KillerPC->Client_ShowKillNotification(KilledPS ? KilledPS->GetPlayerName() : TEXT("未知"));
		}
	}

	// 禁用碰撞和输入
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// 延迟重生
	FTimerHandle RespawnTimer;
	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(this, &AShootDemoCharacter::Respawn);
	GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, RespawnDelay, false);
}

void AShootDemoCharacter::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* GM = GetWorld()->GetAuthGameMode();
	AShootDemoGameMode* ShootGM = Cast<AShootDemoGameMode>(GM);
	if (ShootGM)
	{
		ShootGM->RespawnPlayer(GetController());
	}
}

// ===== 弹药访问 =====

int32 AShootDemoCharacter::GetCurrentAmmo() const
{
	return CurrentWeapon ? CurrentWeapon->GetCurrentAmmo() : 0;
}

int32 AShootDemoCharacter::GetMaxAmmo() const
{
	return CurrentWeapon ? CurrentWeapon->GetMaxAmmo() : 0;
}

void AShootDemoCharacter::AddAmmo(int32 Amount)
{
	if (CurrentWeapon && HasAuthority())
	{
		CurrentWeapon->AddAmmo(Amount);
	}
}

// ===== 输入处理 =====

void AShootDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Error, TEXT("[ShootDemo] 需要使用EnhancedInputComponent！请在项目设置中启用Enhanced Input。"));
		return;
	}

	// 移动
	if (IA_Move)
	{
		EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AShootDemoCharacter::Move);
	}

	// 视角
	if (IA_Look)
	{
		EnhancedInput->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AShootDemoCharacter::Look);
	}

	// 开火
	if (IA_Fire)
	{
		EnhancedInput->BindAction(IA_Fire, ETriggerEvent::Started, this, &AShootDemoCharacter::StartFire);
		EnhancedInput->BindAction(IA_Fire, ETriggerEvent::Completed, this, &AShootDemoCharacter::StopFire);
	}

	// 换弹
	if (IA_Reload)
	{
		EnhancedInput->BindAction(IA_Reload, ETriggerEvent::Started, this, &AShootDemoCharacter::Reload);
	}

	// 跳跃
	if (IA_Jump)
	{
		EnhancedInput->BindAction(IA_Jump, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInput->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
}

void AShootDemoCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (Controller)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AShootDemoCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookVector = Value.Get<FVector2D>();
	if (Controller)
	{
		AddControllerYawInput(LookVector.X);
		AddControllerPitchInput(LookVector.Y);
	}
}

void AShootDemoCharacter::StartFire()
{
	if (bIsDead || !CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->StartFire();
}

void AShootDemoCharacter::StopFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AShootDemoCharacter::Reload()
{
	if (bIsDead || !CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->Reload();
}

// ===== 网络复制 =====

void AShootDemoCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShootDemoCharacter, CurrentHealth);
	DOREPLIFETIME(AShootDemoCharacter, bIsDead);
	DOREPLIFETIME(AShootDemoCharacter, CurrentWeapon);
}

void AShootDemoCharacter::OnRep_CurrentHealth()
{
	// 客户端收到生命值更新时的回调
	// 可在此触发UI更新或特效
	UE_LOG(LogTemp, Verbose, TEXT("[ShootDemo] 客户端生命值更新: %.0f"), CurrentHealth);
}

void AShootDemoCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		// 播放死亡表现
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetCharacterMovement()->DisableMovement();
	}
}

// ===== 多播RPC =====

void AShootDemoCharacter::Multicast_OnHitReact_Implementation(const FVector& DamageLocation)
{
	// 播放受击动画
	if (HitReactAnimation && Mesh1P)
	{
		Mesh1P->GetAnimInstance()->Montage_Play(HitReactAnimation);
	}
}

void AShootDemoCharacter::Multicast_OnDeath_Implementation()
{
	// 播放死亡动画
	if (DeathAnimation && Mesh1P)
	{
		Mesh1P->GetAnimInstance()->Montage_Play(DeathAnimation);
	}

	// 禁用碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 分离武器
	if (CurrentWeapon)
	{
		CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void AShootDemoCharacter::Multicast_OnRespawn_Implementation()
{
	// 重新启用碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// 重置生命值（仅视觉效果，实际值由服务器复制）
	// CurrentHealth = MaxHealth;  // 不应在客户端修改复制属性
}
