// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FuncLibrary/Types.h"
#include "Weapons/WeaponDefault.h"
#include "Character/TPSInventoryComponent.h"
#include "Character/TPSCharacterHealthComponent.h"
#include "Interface/TPS_IGameActor.h"
#include "StateEffects/TPS_StateEffect.h"
#include "TPSCharacter.generated.h"

UCLASS(Blueprintable)
class ATPSCharacter : public ACharacter, public ITPS_IGameActor
{
	GENERATED_BODY()
protected:
	bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	void BeginPlay() override;

	//Inputs
	void InputAxisY(float Value);
	void InputAxisX(float Value);

	void InputAttackPressed();
	void InputAttackReleased();

	void InputWalkPressed();
	void InputWalkReleased();

	void InputSprintPressed();
	void InputSprintReleased();

	void InputAimPressed();
	void InputAimReleased();

	//Inventory Inputs
	void TrySwitchNextWeapon();
	void TrySwitchPreviosWeapon();
	//Ability Inputs
	void TryAbilityEnabled();

	template<int32 Id>
	void TKeyPressed()
	{
		TrySwitchWeaponToIndexByKeyInput_OnServer(Id);
	}
	//Inputs End

	//Input Flags
	float AxisX = 0.0f;
	float AxisY = 0.0f;

	bool SprintRunEnabled = false;
	bool WalkEnabled = false;
	bool AimEnabled = false;

	UPROPERTY(Replicated)
	EMovementState MovementState = EMovementState::Run_State;
	UPROPERTY(Replicated)
	AWeaponDefault* CurrentWeapon = nullptr;

	UDecalComponent* CurrentCursor = nullptr;

	UPROPERTY(Replicated)
	TArray<UTPS_StateEffect*> Effects;
	UPROPERTY(ReplicatedUsing = EffectAdd_OnRep)
	UTPS_StateEffect* EffectAdd = nullptr;
	UPROPERTY(ReplicatedUsing = EffectRemove_OnRep)
	UTPS_StateEffect* EffectRemove = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	TArray<UParticleSystemComponent*> ParticleSystemEffects;

	UPROPERTY(Replicated)
	int32 CurrentIndexWeapon = 0;

	UFUNCTION()
	void CharDead();
	UFUNCTION(NetMulticast, Reliable)
	void EnableRagdoll_Multicast();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;



public:
	ATPSCharacter();

	FTimerHandle TimerHandle_RagDollTimer;

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
		class UTPSInventoryComponent* InventoryComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
		class UTPSCharacterHealthComponent* CharHealthComponent;

	//Cursor material on decal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
		UMaterialInterface* CursorMaterial = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cursor")
		FVector CursorSize = FVector(20.0f, 40.0f, 40.0f);
	//Default move rule and state character
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		FCharacterSpeed MovementSpeedInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim")
		TArray<UAnimMontage*> DeadsAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
		TSubclassOf<UTPS_StateEffect> AbilityEffect;

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

public:

	// Tick Func
	UFUNCTION()
	void MovementTick(float DeltaTime);
	// Tick Func End

	//Func
	void CharacterUpdate();
	void ChangeMovementState();

	void AttackCharEvent(bool bIsFiring);

	UFUNCTION()
	void InitWeapon(FName IdWeaponName, FAdditionalWeaponInfo WeaponAdditionalInfo, int32 NewCurrentIndexWeapon);
	void TryReloadWeapon();
	UFUNCTION()
	void WeaponFireStart(UAnimMontage* Anim);
	UFUNCTION()
	void WeaponReloadStart(UAnimMontage* Anim);
	UFUNCTION()
	void WeaponReloadEnd(bool bIsSuccess, int32 AmmoSafe);
	//
	UFUNCTION(Server, Reliable)
	void TrySwitchWeaponToIndexByKeyInput_OnServer(int32 ToIndex);
	void DropCurrentWeapon();
	UFUNCTION(BlueprintNativeEvent)
		void WeaponReloadStart_BP(UAnimMontage* Anim);
	UFUNCTION(BlueprintNativeEvent)
		void WeaponReloadEnd_BP(bool bIsSuccess);
	UFUNCTION(BlueprintNativeEvent)
		void WeaponFireStart_BP(UAnimMontage* Anim);

	UFUNCTION(BlueprintCallable, BlueprintPure)
		AWeaponDefault* GetCurrentWeapon();
	UFUNCTION(BlueprintCallable, BlueprintPure)
		UDecalComponent* GetCursorToWorld();
	UFUNCTION(BlueprintCallable, BlueprintPure)
		EMovementState GetMovementState();
	UFUNCTION(BlueprintCallable, BlueprintPure)
		TArray<UTPS_StateEffect*> GetCurrentEffectsOnChar();
	UFUNCTION(BlueprintCallable, BlueprintPure)
		int32 GetCurrentWeaponIndex();

	UFUNCTION(BlueprintCallable, BlueprintPure)
		bool GetIsAlive();
	//Func End

	//Interface
	EPhysicalSurface GetSurfuceType() override;
	TArray<UTPS_StateEffect*> GetAllCurrentEffects() override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void RemoveEffect(UTPS_StateEffect* RemoveEffect);
	void RemoveEffect_Implementation(UTPS_StateEffect* RemoveEffect)override;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void AddEffect(UTPS_StateEffect* newEffect);
	void AddEffect_Implementation(UTPS_StateEffect* newEffect)override;
	//End Interface

	UFUNCTION(BlueprintNativeEvent)
	void CharDead_BP();

	UFUNCTION(Server, Unreliable)
	void SetActorRotationByYaw_OnServer(float Yaw);
	UFUNCTION(NetMulticast, Unreliable)
	void SetActorRotationByYaw_Multicast(float Yaw);

	UFUNCTION(Server, Reliable)
		void SetMovementState_OnServer(EMovementState NewState);
	UFUNCTION(NetMulticast, Reliable)
		void SetMovementState_Multicast(EMovementState NewState);

	UFUNCTION(Server, Reliable)
		void TryReloadWeapon_OnServer();

	UFUNCTION(NetMulticast, Reliable)
		void PlayAnim_Multicast(UAnimMontage* Anim);

	UFUNCTION()
		void EffectAdd_OnRep();
	UFUNCTION()
		void EffectRemove_OnRep();

	UFUNCTION(Server, Reliable)
		void ExecuteEffectAdded_OnServer(UParticleSystem* ExecuteFX);

	UFUNCTION(NetMulticast, Reliable)
		void ExecuteEffectAdded_Multicast(UParticleSystem* ExecuteFX);
	
	UFUNCTION()
	void SwitchEffect(UTPS_StateEffect* Effect, bool bIsAdd);
};

