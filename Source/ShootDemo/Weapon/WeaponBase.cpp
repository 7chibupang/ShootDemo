// WeaponBase.cpp — 武器基类实现

#include "WeaponBase.h"
#include "../Character/ShootDemoCharacter.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimMontage.h"

AWeaponBase::AWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 创建组件
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(Root);
	WeaponMesh->SetIsReplicated(true);

	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	MuzzleLocation->SetupAttachment(WeaponMesh, TEXT("Muzzle"));

	// 默认值
	Damage = 25.0f;
	Range = 10000.0f;
	FireRate = 600.0f; // 每分钟600发 = 每秒10发
	MaxAmmo = 30;
	ReloadTime = 2.0f;
	BulletSpread = 1.5f;

	CurrentAmmo = MaxAmmo;
	bIsReloading = false;
	LastFireTime = -999.0f;
	bIsFiring = false;
}

// ===== 射击逻辑 =====

void AWeaponBase::StartFire()
{
	if (!CanFire())
	{
		return;
	}

	bIsFiring = true;

	// 第一发立即射击
	PerformFire();

	// 设置射击间隔定时器
	float FireInterval = 60.0f / FireRate;
	GetWorldTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&AWeaponBase::PerformFire,
		FireInterval,
		true
	);
}

void AWeaponBase::StopFire()
{
	bIsFiring = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

bool AWeaponBase::CanFire() const
{
	return CurrentAmmo > 0 && !bIsReloading && OwningCharacter != nullptr;
}

void AWeaponBase::PerformFire()
{
	if (!CanFire())
	{
		StopFire();
		return;
	}

	if (!OwningCharacter)
	{
		return;
	}

	// 计算射击起点和方向（考虑散布）
	FVector FireLocation = MuzzleLocation->GetComponentLocation();
	FVector FireDirection = OwningCharacter->GetControlRotation().Vector();

	// 添加散布
	if (BulletSpread > 0.0f)
	{
		float SpreadRad = FMath::DegreesToRadians(BulletSpread * 0.5f);
		FireDirection = FMath::VRandCone(FireDirection, SpreadRad);
	}

	// 服务器执行命中判定；客户端发送RPC给服务器
	if (HasAuthority())
	{
		Server_Fire_Implementation(FireLocation, FireDirection);
	}
	else
	{
		Server_Fire(FireLocation, FireDirection);
	}

	// 客户端预消耗弹药（服务器会复制正确的值）
	CurrentAmmo--;

	// 本地立即播放开火特效（减少延迟感）
	// 枪口闪光特效
	if (MuzzleFlashEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleFlashEffect,
			MuzzleLocation,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}
	// 射击音效
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, MuzzleLocation->GetComponentLocation());
	}
	// 射击动画
	if (OwningCharacter && FireAnimation)
	{
		USkeletalMeshComponent* Mesh1P = OwningCharacter->GetMesh1P();
		if (Mesh1P)
		{
			Mesh1P->GetAnimInstance()->Montage_Play(FireAnimation);
		}
	}

	// 服务器会通过Multicast RPC让其他客户端也看到
}

// ===== 换弹 =====

void AWeaponBase::Reload()
{
	if (bIsReloading || CurrentAmmo >= MaxAmmo)
	{
		return;
	}

	bIsReloading = true;

	// 播放换弹
	Multicast_OnReload();

	// 延迟完成换弹
	FTimerHandle ReloadTimer;
	FTimerDelegate ReloadDelegate;
	ReloadDelegate.BindLambda([this]()
	{
		if (HasAuthority())
		{
			CurrentAmmo = MaxAmmo;
		}
		bIsReloading = false;
	});

	GetWorldTimerManager().SetTimer(ReloadTimer, ReloadDelegate, ReloadTime, false);
}

void AWeaponBase::AddAmmo(int32 Amount)
{
	if (HasAuthority())
	{
		CurrentAmmo = FMath::Min(CurrentAmmo + Amount, MaxAmmo);
	}
}

// ===== 网络RPC实现 =====

void AWeaponBase::Server_Fire_Implementation(const FVector& FireLocation, const FVector& FireDirection)
{
	// 服务器验证弹药
	if (!CanFire())
	{
		return;
	}

	CurrentAmmo--;

	// 射线检测
	FVector Start = FireLocation;
	FVector End = Start + FireDirection * Range;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(OwningCharacter);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_GameTraceChannel1, // 自定义碰撞通道，可在项目中配置
		QueryParams
	);

	// 如果没有自定义通道，使用Visibility通道作为后备
	if (!bHit)
	{
		bHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Start,
			End,
			ECC_Visibility,
			QueryParams
		);
	}

	// 通知所有客户端播放开火特效（本地客户端已在PerformFire中播放）
	Multicast_OnFire();

	// 调试射线（开发时使用）
	// DrawDebugLine(GetWorld(), Start, bHit ? HitResult.Location : End, bHit ? FColor::Green : FColor::Red, false, 1.0f);

	if (bHit)
	{
		// 造成伤害
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			// 暴击判定（命中头部骨骼等）
			bool bIsHeadshot = HitResult.BoneName.ToString().Contains(TEXT("head")) ||
				HitResult.BoneName.ToString().Contains(TEXT("Head"));

			float FinalDamage = bIsHeadshot ? Damage * 2.0f : Damage;

			FPointDamageEvent DamageEvent(FinalDamage, HitResult, FireDirection, nullptr);
			HitActor->TakeDamage(FinalDamage, DamageEvent, OwningCharacter ? OwningCharacter->GetController() : nullptr, this);
		}

		// 多播命中特效
		Multicast_OnHit(HitResult.Location, HitResult.Normal);
	}
}

bool AWeaponBase::Server_Fire_Validate(const FVector& FireLocation, const FVector& FireDirection)
{
	// 验证射击参数合法性
	return FireLocation.Size() < 100000.0f && FireDirection.IsNormalized();
}

void AWeaponBase::Multicast_OnFire_Implementation()
{
	// 跳过本地玩家（PerformFire中已播放，避免双重视觉效果）
	if (OwningCharacter && OwningCharacter->IsLocallyControlled())
	{
		return;
	}

	// 枪口闪光
	if (MuzzleFlashEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleFlashEffect,
			MuzzleLocation,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	// 射击音效
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FireSound, MuzzleLocation->GetComponentLocation());
	}

	// 射击动画
	if (OwningCharacter && FireAnimation)
	{
		USkeletalMeshComponent* Mesh1P = OwningCharacter->GetMesh1P();
		if (Mesh1P)
		{
			Mesh1P->GetAnimInstance()->Montage_Play(FireAnimation);
		}
	}
}

void AWeaponBase::Multicast_OnHit_Implementation(const FVector& HitLocation, const FVector& HitNormal)
{
	// 命中特效
	if (ImpactEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactEffect,
			HitLocation,
			HitNormal.Rotation(),
			true
		);
	}
}

void AWeaponBase::Multicast_OnReload_Implementation()
{
	// 换弹音效
	if (ReloadSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ReloadSound, GetActorLocation());
	}

	// 换弹动画
	if (OwningCharacter && ReloadAnimation)
	{
		USkeletalMeshComponent* Mesh1P = OwningCharacter->GetMesh1P();
		if (Mesh1P)
		{
			Mesh1P->GetAnimInstance()->Montage_Play(ReloadAnimation);
		}
	}
}

void AWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeaponBase, CurrentAmmo);
	DOREPLIFETIME(AWeaponBase, bIsReloading);
}
