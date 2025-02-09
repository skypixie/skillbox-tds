// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"

#include "FuncLibrary/Types.h"
#include "Weapons/Projectiles/ProjectileDefault.h"
#include "WeaponDefault.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFireStart, UAnimMontage*, AnimFireChar);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponReloadStart,UAnimMontage*, AnimReloadChar);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponReloadEnd, bool, bIsSuccess, int32, AmmoSafe);

UCLASS()
class TPS_API AWeaponDefault : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponDefault();

	FOnWeaponFireStart OnWeaponFireStart;
	FOnWeaponReloadEnd OnWeaponReloadEnd;
	FOnWeaponReloadStart OnWeaponReloadStart;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = Components)
	class USceneComponent* SceneComponent = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = Components)
	class USkeletalMeshComponent* SkeletalMeshWeapon = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = Components)
	class UStaticMeshComponent* StaticMeshWeapon = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = Components)
	class UArrowComponent* ShootLocation = nullptr;

	UPROPERTY(VisibleAnywhere)
		FWeaponInfo WeaponSetting;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapon Info")
		FAdditionalWeaponInfo AdditionalWeaponInfo;

	//Timers
	float FireTimer = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReloadLogic")
		float ReloadTimer = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReloadLogic Debug")	//Remove !!! Debug
		float ReloadTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireLogic")
		FName IdWeaponName;
	//flags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireLogic")
		bool WeaponFiring = false;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ReloadLogic")
		bool WeaponReloading = false;
	bool WeaponAiming = false;

	bool BlockFire = false;
	//Dispersion
	bool ShouldReduceDispersion = false;
	float CurrentDispersion = 0.0f;
	float CurrentDispersionMax = 1.0f;
	float CurrentDispersionMin = 0.1f;
	float CurrentDispersionRecoil = 0.1f;
	float CurrentDispersionReduction = 0.1f;

	//Timer Drop Magazine on reload
	bool DropClipFlag = false;
	float DropClipTimer = -1.0;

	//shell flag
	bool DropShellFlag = false;
	float DropShellTimer = -1.0f;
	UPROPERTY(Replicated)
	FVector ShootEndLocation = FVector(0);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Tick func End
	void FireTick(float DeltaTime);
	void ReloadTick(float DeltaTime);
	void DispersionTick(float DeltaTime);
	void ClipDropTick(float DeltaTime);
	void ShellDropTick(float DeltaTime);
	void WeaponInit();

	UFUNCTION()
		void Fire();
public:	
	// Tick func
	virtual void Tick(float DeltaTime) override;


	UFUNCTION(Server, Reliable, BlueprintCallable)
	void SetWeaponStateFire_OnServer(bool bIsFire);

	bool CheckWeaponCanFire();

	FProjectileInfo GetProjectile();

	UFUNCTION(Server, Reliable)
	void UpdateStateWeapon_OnServer(EMovementState NewMovementState);
	void ChangeDispersionByShot();
	float GetCurrentDispersion() const;
	FVector ApplyDispersionToShoot(FVector DirectionShoot)const;

	FVector GetFireEndLocation()const;
	int8 GetNumberProjectileByShot() const;

	UFUNCTION(BlueprintCallable)
	int32 GetWeaponRound();

	UFUNCTION()
	void InitReload();
	void FinishReload();
	void CancelReload();

	bool CheckCanWeaponReload();
	int8 GetAviableAmmoForReload();

	UFUNCTION(Server, Reliable)
	void InitDropMesh_OnServer (UStaticMesh* DropMesh, FTransform Offset, FVector DropImpulseDirection, float LifeTimeMesh, float ImpulseRandomDispersion, float PowerImpulse, float CustomMass);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		float SizeVectorToChangeShootDirectionLogic = 100.0f;

	//Net
	UFUNCTION(Server, Unreliable)
	void UpdateWeaponByCharacterMovementState_OnServer(FVector NewShootEndLocation, bool NewShouldReduceDispersion);

	UFUNCTION(NetMulticast, Unreliable)
		void AnimWeaponStart_Multicast(UAnimMontage* Anim);
	UFUNCTION(NetMulticast, Unreliable)
		void ShellDropFire_Multicast(UStaticMesh* DropMesh, FTransform Offset, FVector DropImpulseDirection, float LifeTimeMesh, float ImpulseRandomDispersion, float PowerImpulse, float CustomMass, FVector LocalDir);

	UFUNCTION(NetMulticast, Unreliable)
		void FXWeaponFire_Multicast(UParticleSystem* FxFire, USoundBase* SoundFire);

	UFUNCTION(NetMulticast, Reliable)
		void SpawnTraceHitDecal_Multicast(UMaterialInterface* DecalMaterial, FHitResult HitResult);
	UFUNCTION(NetMulticast, Reliable)
		void SpawnTraceHitFX_Multicast(UParticleSystem* FxTemplate, FHitResult HitResult);
	UFUNCTION(NetMulticast, Reliable)
		void SpawnTraceHitSound_Multicast(USoundBase* HitSound, FHitResult HitResult);
};
