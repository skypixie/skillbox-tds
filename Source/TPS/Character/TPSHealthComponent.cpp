// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSHealthComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UTPSHealthComponent::UTPSHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
	// ...
}


// Called when the game starts
void UTPSHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UTPSHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

float UTPSHealthComponent::GetCurrentHealth()
{
	return Health;
}

void UTPSHealthComponent::SetCurrentHealth(float NewHealth)
{
	Health = NewHealth;
}

bool UTPSHealthComponent::GetIsAlive()
{
	return bIsAlive;
}

void UTPSHealthComponent::ChangeHealthValue_OnServer_Implementation(float ChangeValue)
{
	if (bIsAlive)
	{
		ChangeValue = ChangeValue * CoefDamage;

		Health += ChangeValue;

		//OnHealthChange.Broadcast(Health, ChangeValue);
		HealthChangeEvent_Multicast(Health, ChangeValue);

		if (Health > 100.0f)
		{
			Health = 100.0f;
		}
		else
		{
			if (Health < 0.0f)
			{
				bIsAlive = false;
				//OnDead.Broadcast();		
				DeadEvent_Multicast();
			}
		}
	}
	
}

void UTPSHealthComponent::HealthChangeEvent_Multicast_Implementation(float newHealth, float value)
{
	OnHealthChange.Broadcast(newHealth, value);
}

void UTPSHealthComponent::DeadEvent_Multicast_Implementation()
{
	OnDead.Broadcast();
}

void UTPSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTPSHealthComponent, Health);
	DOREPLIFETIME(UTPSHealthComponent, bIsAlive);

}