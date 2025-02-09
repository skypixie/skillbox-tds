// Fill out your copyright notice in the Description page of Project Settings.


#include "TPS_StateEffect.h"
#include "Character/TPSHealthComponent.h"
#include "Interface/TPS_IGameActor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

bool UTPS_StateEffect::InitObject(AActor* Actor, FName NameBoneHit)
{	
	myActor = Actor;
	NameBone = NameBoneHit;

	ITPS_IGameActor* myInterface = Cast<ITPS_IGameActor>(myActor);
	if (myInterface)
	{
		myInterface->Execute_AddEffect(myActor,this);
	}

	return true;
}

void UTPS_StateEffect::DestroyObject()
{
	ITPS_IGameActor* myInterface = Cast<ITPS_IGameActor>(myActor);
	if (myInterface)
	{
		myInterface->Execute_RemoveEffect(myActor, this);
	}

	myActor = nullptr;
	if (this && this->IsValidLowLevel())
	{
		this->ConditionalBeginDestroy();
	}	
}

bool UTPS_StateEffect_ExecuteOnce::InitObject(AActor* Actor, FName NameBoneHit)
{
	Super::InitObject(Actor, NameBoneHit);
	ExecuteOnce();
	return true;
}

void UTPS_StateEffect_ExecuteOnce::DestroyObject()
{
	Super::DestroyObject();
}

void UTPS_StateEffect_ExecuteOnce::ExecuteOnce()
{
	if (myActor)
	{
		UTPSHealthComponent* myHealthComp = Cast<UTPSHealthComponent>(myActor->GetComponentByClass(UTPSHealthComponent::StaticClass()));
		if (myHealthComp)
		{
			myHealthComp->ChangeHealthValue_OnServer(Power);
		}
	}	

	DestroyObject();
}

bool UTPS_StateEffect_ExecuteTimer::InitObject(AActor* Actor, FName NameBoneHit)
{
	Super::InitObject(Actor, NameBoneHit);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_EffectTimer, this, &UTPS_StateEffect_ExecuteTimer::DestroyObject, Timer, false);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ExecuteTimer, this, &UTPS_StateEffect_ExecuteTimer::Execute, RateTime, true);
	}
	
	//if (ParticleEffect)
	//{
		//FXSpawnByStateEffect_Multicast(ParticleEffect, NameBoneHit);		To REmove
	//}
	
	return true;
}

void UTPS_StateEffect_ExecuteTimer::DestroyObject()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}

	//ParticleEmitter->DestroyComponent();
	//ParticleEmitter = nullptr;
	Super::DestroyObject();
}

void UTPS_StateEffect_ExecuteTimer::Execute()
{
	if (myActor)
	{	
		UTPSHealthComponent* myHealthComp = Cast<UTPSHealthComponent>(myActor->GetComponentByClass(UTPSHealthComponent::StaticClass()));
		if (myHealthComp)
		{
			myHealthComp->ChangeHealthValue_OnServer(Power);
		}
	}
}

void UTPS_StateEffect::FXSpawnByStateEffect_Multicast_Implementation(UParticleSystem* Effect, FName NameBoneHit)
{
	//ToDo for object with interface create func return offset, Name Bones, 
	//ToDo Random init Effect with available array (For)
	/*if (Effect)
	{
		FName NameBoneToAttached = NameBoneHit;
		FVector Loc = FVector(0);


		USceneComponent* myMesh = Cast<USceneComponent>(myActor->GetComponentByClass(USkeletalMeshComponent::StaticClass()));
		if (myMesh)
		{
			ParticleEmitter = UGameplayStatics::SpawnEmitterAttached(Effect, myMesh, NameBoneToAttached, Loc, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
		}
		else
		{
			ParticleEmitter = UGameplayStatics::SpawnEmitterAttached(Effect, myActor->GetRootComponent(), NameBoneToAttached, Loc, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
		}
	}*/
}

void UTPS_StateEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTPS_StateEffect, NameBone);
}