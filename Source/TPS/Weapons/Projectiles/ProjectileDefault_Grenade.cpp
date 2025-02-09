// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileDefault_Grenade.h"
#include "Kismet/GameplayStatics.h"
#include "Interface/TPS_IGameActor.h"
#include "DrawDebugHelpers.h"

int32 DebugExplodeShow = 0;
FAutoConsoleVariableRef CVARExplodeShow(
	TEXT("TPS.DebugExplode"),
	DebugExplodeShow,
	TEXT("Draw Debug for Explode"),
	ECVF_Cheat);

void AProjectileDefault_Grenade::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectileDefault_Grenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority())
	{
		TimerExplose(DeltaTime);
	}	
}

void AProjectileDefault_Grenade::TimerExplose(float DeltaTime)
{
	if (TimerEnabled)
	{
		if (TimerToExplose > TimeToExplose)
		{
			Explose_Onserver();			
		}
		else
		{
			TimerToExplose += DeltaTime;
		}
	}
}

void AProjectileDefault_Grenade::BulletCollisionSphereHit(class UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!TimerEnabled)
	{
		Explose_Onserver();
	}
	Super::BulletCollisionSphereHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void AProjectileDefault_Grenade::ImpactProjectile()
{
	//Init Grenade
	TimerEnabled = true;
}

void AProjectileDefault_Grenade::Explose_Onserver_Implementation()
{
	if (DebugExplodeShow)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), ProjectileSetting.ProjectileMinRadiusDamage, 12, FColor::Green, false, 12.0f);
		DrawDebugSphere(GetWorld(), GetActorLocation(), ProjectileSetting.ProjectileMaxRadiusDamage, 12, FColor::Red, false, 12.0f);
	}

	TimerEnabled = false;
	if (ProjectileSetting.ExplodeFX)
	{
		SpawnExplodeFX_Multicast(ProjectileSetting.ExplodeFX);
	}
	if (ProjectileSetting.ExplodeSound)
	{
		SpawnExplodeSound_Multicast(ProjectileSetting.ExplodeSound);		
	}
	

	TArray<AActor*> IgnoredActor;
	UGameplayStatics::ApplyRadialDamageWithFalloff(GetWorld(),
		ProjectileSetting.ExplodeMaxDamage,
		ProjectileSetting.ExplodeMaxDamage*0.2f,
		GetActorLocation(),
		ProjectileSetting.ProjectileMinRadiusDamage,
		ProjectileSetting.ProjectileMaxRadiusDamage,
		5,
		NULL, IgnoredActor,this,nullptr);

	this->Destroy();
}

void AProjectileDefault_Grenade::SpawnExplodeFX_Multicast_Implementation(UParticleSystem* FxTemplate)
{
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FxTemplate, GetActorLocation(), GetActorRotation(), FVector(1.0f));
}

void AProjectileDefault_Grenade::SpawnExplodeSound_Multicast_Implementation(USoundBase* ExplodeSound)
{
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplodeSound, GetActorLocation());
}
