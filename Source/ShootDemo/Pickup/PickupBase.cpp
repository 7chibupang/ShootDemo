// PickupBase.cpp — 拾取物基类实现

#include "PickupBase.h"
#include "../Character/ShootDemoCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

APickupBase::APickupBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(80.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(CollisionSphere);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(CollisionSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RespawnTime = 10.0f;
	RotationSpeed = 90.0f;
	bIsActive = true;
}

void APickupBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &APickupBase::OnOverlapBegin);
	}
}

void APickupBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 旋转效果（纯表现）
	if (bIsActive && PickupMesh)
	{
		PickupMesh->AddLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	}
}

void APickupBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bIsActive)
	{
		return;
	}

	AShootDemoCharacter* Player = Cast<AShootDemoCharacter>(OtherActor);
	if (!Player || Player->IsDead())
	{
		return;
	}

	OnPickedUp(Player);
	DeactivatePickup();
}

void APickupBase::ActivatePickup()
{
	bIsActive = true;
	PickupMesh->SetVisibility(true);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void APickupBase::DeactivatePickup()
{
	bIsActive = false;
	PickupMesh->SetVisibility(false);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 播放特效
	if (PickupEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), PickupEffect, GetActorLocation());
	}
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickupSound, GetActorLocation());
	}

	// 延迟重生
	if (RespawnTime > 0.0f)
	{
		FTimerHandle RespawnTimer;
		FTimerDelegate RespawnDelegate;
		RespawnDelegate.BindUObject(this, &APickupBase::ActivatePickup);
		GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, RespawnTime, false);
	}
}
