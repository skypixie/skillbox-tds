// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSCharacterHealthComponent.h"



void UTPSCharacterHealthComponent::ChangeHealthValue_OnServer(float ChangeValue)
{
	float CurrentDamage = ChangeValue * CoefDamage;

	if (Shield > 0.0f && ChangeValue < 0.0f)
	{
		ChangeShieldValue(ChangeValue);
		
		if (Shield < 0.0f)
		{
			//FX
			//UE_LOG(LogTemp, Warning, TEXT("UTPSCharacterHealthComponent::ChangeHealthValue - Sheild < 0"));
		}
	}
	else
	{
		Super::ChangeHealthValue_OnServer(ChangeValue);
	}
}

float UTPSCharacterHealthComponent::GetCurrentShield()
{
	return Shield;
}

void UTPSCharacterHealthComponent::ChangeShieldValue(float ChangeValue)
{
	Shield += ChangeValue;
	//OnShieldChange.Broadcast(Shield, ChangeValue);
	ShieldChangeEvent_Multicast(Shield,ChangeValue);

	if (Shield > 100.0f)
	{
		Shield = 100.0f;
	}
	else
	{
		if(Shield < 0.0f)
			Shield = 0.0f;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_CollDownShieldTimer, this, &UTPSCharacterHealthComponent::CoolDownShieldEnd, CoolDownShieldRecoverTime, false);

		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ShieldRecoveryRateTimer);
	}		
}

void UTPSCharacterHealthComponent::CoolDownShieldEnd()
{
	if (GetWorld())
	{		
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_ShieldRecoveryRateTimer, this, &UTPSCharacterHealthComponent::RecoveryShield, ShieldRecoverRate, true);
	}	
}

void UTPSCharacterHealthComponent::RecoveryShield()
{
	float tmp = Shield;
	tmp = tmp + ShieldRecoverValue;
	if (tmp > 100.0f)
	{
		Shield = 100.0f;
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ShieldRecoveryRateTimer);
		}
	}
	else
	{
		Shield = tmp;
	}

	//OnShieldChange.Broadcast(Shield, ShieldRecoverValue);
	ShieldChangeEvent_Multicast(Shield, ShieldRecoverValue);
}

float UTPSCharacterHealthComponent::GetShieldValue()
{
	return Shield;
}

void UTPSCharacterHealthComponent::ShieldChangeEvent_Multicast_Implementation(float newShield, float Damage)
{
	OnShieldChange.Broadcast(newShield, Damage);
}
