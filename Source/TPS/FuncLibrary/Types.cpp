// Fill out your copyright notice in the Description page of Project Settings.

#include "Types.h"
#include "TPS.h"
#include "Interface/TPS_IGameActor.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void UTypes::AddEffectBySurfaceType( AActor* TakeEffectActor, FName NameBoneHit, TSubclassOf<UTPS_StateEffect> AddEffectClass, EPhysicalSurface SurfaceType)
{
	if (SurfaceType != EPhysicalSurface::SurfaceType_Default && TakeEffectActor && AddEffectClass)
	{
		UTPS_StateEffect* myEffect = Cast<UTPS_StateEffect>(AddEffectClass->GetDefaultObject());
		if (myEffect)
		{
			bool bIsHavePossibleSurface = false;
			int8 i = 0;
			while (i < myEffect->PossibleInteractSurface.Num() && !bIsHavePossibleSurface)
			{
				if (myEffect->PossibleInteractSurface[i] == SurfaceType)
				{			
					bIsHavePossibleSurface = true;
					bool bIsCanAddEffect = false;
					if (!myEffect->bIsStakable)
					{
						int8 j = 0;
						TArray<UTPS_StateEffect*> CurrentEffects;
						ITPS_IGameActor* myInterface = Cast<ITPS_IGameActor>(TakeEffectActor);
						if (myInterface)
						{
							CurrentEffects = myInterface->GetAllCurrentEffects();
						}

						if (CurrentEffects.Num() > 0)
						{
							while (j < CurrentEffects.Num() && !bIsCanAddEffect)
							{
								if (CurrentEffects[j]->GetClass() != AddEffectClass)
								{
									bIsCanAddEffect = true;
								}
								j++;
							}
						}
						else
						{
							bIsCanAddEffect = true;
						}
						
					}
					else
					{
						bIsCanAddEffect = true;
					}

					if (bIsCanAddEffect)
					{
						
						UTPS_StateEffect* NewEffect = NewObject<UTPS_StateEffect>(TakeEffectActor, AddEffectClass);
						if (NewEffect)
						{
							NewEffect->InitObject(TakeEffectActor, NameBoneHit);
						}
					}
					
				}
				i++;
			}
		}

	}
}

void UTypes::ExecuteEffectAdded(UParticleSystem* ExecuteFX, AActor* target, FVector offset, FName Socket)
{
	if (target)
	{
		FName SocketToAttached = Socket;
		FVector Loc = offset;
		ACharacter* myCharacter = Cast<ACharacter>(target);
		if (myCharacter && myCharacter->GetMesh())
		{
			UGameplayStatics::SpawnEmitterAttached(ExecuteFX, myCharacter->GetMesh(), SocketToAttached, Loc, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
		}
		else
		{
			if (target->GetRootComponent())
			{
				UGameplayStatics::SpawnEmitterAttached(ExecuteFX, target->GetRootComponent(), SocketToAttached, Loc, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
			}			
		}
	}	
}
