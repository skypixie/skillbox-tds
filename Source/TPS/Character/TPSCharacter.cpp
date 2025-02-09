// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TPSCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Game/TPSGameInstance.h"
#include "Weapons/Projectiles/ProjectileDefault.h"
#include "TPS.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/ActorChannel.h"


ATPSCharacter::ATPSCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	
	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	InventoryComponent = CreateDefaultSubobject<UTPSInventoryComponent>(TEXT("InventoryComponent"));
	CharHealthComponent = CreateDefaultSubobject<UTPSCharacterHealthComponent>(TEXT("HealthComponent"));

	if (CharHealthComponent)
	{
		CharHealthComponent->OnDead.AddDynamic(this, &ATPSCharacter::CharDead);
	}
	if (InventoryComponent)
	{
		InventoryComponent->OnSwitchWeapon.AddDynamic(this, &ATPSCharacter::InitWeapon);
	}
	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//NetWork 
	bReplicates = true;
}

void ATPSCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

	if (CurrentCursor)
	{
		APlayerController* myPC = Cast<APlayerController>(GetController());
		if (myPC && myPC->IsLocalPlayerController())
		{
			FHitResult TraceHitResult;
			myPC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();

			CurrentCursor->SetWorldLocation(TraceHitResult.Location);
			CurrentCursor->SetWorldRotation(CursorR);
		}
	}

	MovementTick(DeltaSeconds);
}

void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		if (CursorMaterial && GetLocalRole() == ROLE_AutonomousProxy || GetLocalRole() == ROLE_Authority)
		{
			CurrentCursor = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CursorMaterial, CursorSize, FVector(0));
		}
	}	
}

void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* NewInputComponent)
{
	Super::SetupPlayerInputComponent(NewInputComponent);

	NewInputComponent->BindAxis(TEXT("MoveForward"), this, &ATPSCharacter::InputAxisX);
	NewInputComponent->BindAxis(TEXT("MoveRight"), this, &ATPSCharacter::InputAxisY);

	NewInputComponent->BindAction(TEXT("ChangeToSprint"), EInputEvent::IE_Pressed, this, &ATPSCharacter::InputSprintPressed);
	NewInputComponent->BindAction(TEXT("ChangeToWalk"), EInputEvent::IE_Pressed, this, &ATPSCharacter::InputWalkPressed);
	NewInputComponent->BindAction(TEXT("AimEvent"), EInputEvent::IE_Pressed, this, &ATPSCharacter::InputAimPressed);
	NewInputComponent->BindAction(TEXT("ChangeToSprint"), EInputEvent::IE_Released, this, &ATPSCharacter::InputSprintReleased);
	NewInputComponent->BindAction(TEXT("ChangeToWalk"), EInputEvent::IE_Released, this, &ATPSCharacter::InputWalkReleased);
	NewInputComponent->BindAction(TEXT("AimEvent"), EInputEvent::IE_Released, this, &ATPSCharacter::InputAimReleased);

	NewInputComponent->BindAction(TEXT("FireEvent"), EInputEvent::IE_Pressed, this, &ATPSCharacter::InputAttackPressed);
	NewInputComponent->BindAction(TEXT("FireEvent"), EInputEvent::IE_Released, this, &ATPSCharacter::InputAttackReleased);
	NewInputComponent->BindAction(TEXT("ReloadEvent"), EInputEvent::IE_Released, this, &ATPSCharacter::TryReloadWeapon);

	NewInputComponent->BindAction(TEXT("SwitchNextWeapon"), EInputEvent::IE_Pressed, this, &ATPSCharacter::TrySwitchNextWeapon);
	NewInputComponent->BindAction(TEXT("SwitchPreviosWeapon"), EInputEvent::IE_Pressed, this, &ATPSCharacter::TrySwitchPreviosWeapon);

	NewInputComponent->BindAction(TEXT("AblityAction"), EInputEvent::IE_Pressed, this, &ATPSCharacter::TryAbilityEnabled);
	
	NewInputComponent->BindAction(TEXT("DropCurrentWeapon"), EInputEvent::IE_Pressed, this, &ATPSCharacter::DropCurrentWeapon);

	TArray<FKey> HotKeys;
	HotKeys.Add(EKeys::One);
	HotKeys.Add(EKeys::Two);
	HotKeys.Add(EKeys::Three);
	HotKeys.Add(EKeys::Four);
	HotKeys.Add(EKeys::Five);
	HotKeys.Add(EKeys::Six);
	HotKeys.Add(EKeys::Seven);
	HotKeys.Add(EKeys::Eight);
	HotKeys.Add(EKeys::Nine);
	HotKeys.Add(EKeys::Zero);

	NewInputComponent->BindKey(HotKeys[1], IE_Pressed, this, &ATPSCharacter::TKeyPressed<1>);
	NewInputComponent->BindKey(HotKeys[2], IE_Pressed, this, &ATPSCharacter::TKeyPressed<2>);
	NewInputComponent->BindKey(HotKeys[3], IE_Pressed, this, &ATPSCharacter::TKeyPressed<3>);
	NewInputComponent->BindKey(HotKeys[4], IE_Pressed, this, &ATPSCharacter::TKeyPressed<4>);
	NewInputComponent->BindKey(HotKeys[5], IE_Pressed, this, &ATPSCharacter::TKeyPressed<5>);
	NewInputComponent->BindKey(HotKeys[6], IE_Pressed, this, &ATPSCharacter::TKeyPressed<6>);
	NewInputComponent->BindKey(HotKeys[7], IE_Pressed, this, &ATPSCharacter::TKeyPressed<7>);
	NewInputComponent->BindKey(HotKeys[8], IE_Pressed, this, &ATPSCharacter::TKeyPressed<8>);
	NewInputComponent->BindKey(HotKeys[9], IE_Pressed, this, &ATPSCharacter::TKeyPressed<9>);
	NewInputComponent->BindKey(HotKeys[0], IE_Pressed, this, &ATPSCharacter::TKeyPressed<0>);
}

void ATPSCharacter::InputAxisY(float Value)
{
	AxisY = Value;
}

void ATPSCharacter::InputAxisX(float Value)
{
	AxisX = Value;	
}

void ATPSCharacter::InputAttackPressed()
{
	if (CharHealthComponent && CharHealthComponent->GetIsAlive())
	{
		AttackCharEvent(true);
	}	
}

void ATPSCharacter::InputAttackReleased()
{
	AttackCharEvent(false);
}

void ATPSCharacter::InputWalkPressed()
{
	WalkEnabled = true;
	ChangeMovementState();
}

void ATPSCharacter::InputWalkReleased()
{
	WalkEnabled = false;
	ChangeMovementState();
}

void ATPSCharacter::InputSprintPressed()
{
	SprintRunEnabled = true;
	ChangeMovementState();
}

void ATPSCharacter::InputSprintReleased()
{
	SprintRunEnabled = false;
	ChangeMovementState();
}

void ATPSCharacter::InputAimPressed()
{
	AimEnabled = true;
	ChangeMovementState();
}

void ATPSCharacter::InputAimReleased()
{
	AimEnabled = false;
	ChangeMovementState();
}

void ATPSCharacter::MovementTick(float DeltaTime)
{
	if (CharHealthComponent && CharHealthComponent->GetIsAlive())
	{
		if (GetController() && GetController()->IsLocalPlayerController())
		{
			AddMovementInput(FVector(1.0f, 0.0f, 0.0f), AxisX);
			AddMovementInput(FVector(0.0f, 1.0f, 0.0f), AxisY);

			if (MovementState == EMovementState::SprintRun_State)
			{
				FVector myRotationVector = FVector(AxisX, AxisY, 0.0f);
				FRotator myRotator = myRotationVector.ToOrientationRotator();

				SetActorRotation((FQuat(myRotator)));
				SetActorRotationByYaw_OnServer(myRotator.Yaw);
			}
			else
			{
				APlayerController* myController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
				if (myController)
				{
					FHitResult ResultHit;
					//myController->GetHitResultUnderCursorByChannel(ETraceTypeQuery::TraceTypeQuery6, false, ResultHit);// bug was here Config\DefaultEngine.Ini
					myController->GetHitResultUnderCursor(ECC_GameTraceChannel1, true, ResultHit);

					float FindRotaterResultYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ResultHit.Location).Yaw;

					SetActorRotation(FQuat(FRotator(0.0f, FindRotaterResultYaw, 0.0f)));
					SetActorRotationByYaw_OnServer(FindRotaterResultYaw);

					if (CurrentWeapon)
					{
						FVector Displacement = FVector(0);
						bool bIsReduceDispersion = false;
						switch (MovementState)
						{
						case EMovementState::Aim_State:
							Displacement = FVector(0.0f, 0.0f, 160.0f);
							//CurrentWeapon->ShouldReduceDispersion = true;
							bIsReduceDispersion = true;
							break;
						case EMovementState::AimWalk_State:							
							Displacement = FVector(0.0f, 0.0f, 160.0f);
							//CurrentWeapon->ShouldReduceDispersion = true;
							bIsReduceDispersion = true;
							break;
						case EMovementState::Walk_State:
							Displacement = FVector(0.0f, 0.0f, 120.0f);
							//CurrentWeapon->ShouldReduceDispersion = false;
							break;
						case EMovementState::Run_State:
							Displacement = FVector(0.0f, 0.0f, 120.0f);
							//CurrentWeapon->ShouldReduceDispersion = false;
							break;
						case EMovementState::SprintRun_State:
							break;
						default:
							break;
						}

						//CurrentWeapon->ShootEndLocation = ResultHit.Location + Displacement;
						CurrentWeapon->UpdateWeaponByCharacterMovementState_OnServer(ResultHit.Location + Displacement, bIsReduceDispersion);
						//aim cursor like 3d Widget?
					}
				}
			}
		}		
	}	
}

EMovementState ATPSCharacter::GetMovementState()
{
	return MovementState;
}

TArray<UTPS_StateEffect*> ATPSCharacter::GetCurrentEffectsOnChar()
{
	return Effects;
}

int32 ATPSCharacter::GetCurrentWeaponIndex()
{
	return CurrentIndexWeapon;
}

bool ATPSCharacter::GetIsAlive()
{
	bool result = false;
	if (CharHealthComponent)
	{
		result = CharHealthComponent->GetIsAlive();
	}
	return result;
}

void ATPSCharacter::AttackCharEvent(bool bIsFiring)
{
	AWeaponDefault* myWeapon = nullptr;
	myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		myWeapon->SetWeaponStateFire_OnServer(bIsFiring);
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("ATPSCharacter::AttackCharEvent - CurrentWeapon -NULL"));
}

void ATPSCharacter::CharacterUpdate()
{
	float ResSpeed = 600.0f;
	switch (MovementState)
	{
	case EMovementState::Aim_State:
		ResSpeed = MovementSpeedInfo.AimSpeedNormal;
		break;
	case EMovementState::AimWalk_State:
		ResSpeed = MovementSpeedInfo.AimSpeedWalk;
		break;
	case EMovementState::Walk_State:
		ResSpeed = MovementSpeedInfo.WalkSpeedNormal;
		break;
	case EMovementState::Run_State:
		ResSpeed = MovementSpeedInfo.RunSpeedNormal;
		break;
	case EMovementState::SprintRun_State:
		ResSpeed = MovementSpeedInfo.SprintRunSpeedRun;
		break;
	default:
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = ResSpeed;
}

void ATPSCharacter::ChangeMovementState()
{
	EMovementState NewState = EMovementState::Run_State;
	if (!WalkEnabled && !SprintRunEnabled && !AimEnabled)
	{
		NewState = EMovementState::Run_State;
	}
	else
	{
		if (SprintRunEnabled)
		{
			WalkEnabled = false;
			AimEnabled = false;
			NewState = EMovementState::SprintRun_State;
		}
		else
		{
			if (WalkEnabled && !SprintRunEnabled && AimEnabled)
			{
				NewState = EMovementState::AimWalk_State;
			}
			else
			{
				if (WalkEnabled && !SprintRunEnabled && !AimEnabled)
				{
					NewState = EMovementState::Walk_State;
				}
				else
				{
					if (!WalkEnabled && !SprintRunEnabled && AimEnabled)
					{
						NewState = EMovementState::Aim_State;
					}
				}
			}
		}		
	}	

	SetMovementState_OnServer(NewState);
	
	//CharacterUpdate();

	//Weapon state update
	AWeaponDefault* myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		myWeapon->UpdateStateWeapon_OnServer(NewState);
	}

	//FString SEnum = UEnum::GetValueAsString(NewState);
	//UE_LOG(LogTPS_Net, Warning, TEXT("Movement state on SERVER - %s"), *SEnum);
}

AWeaponDefault* ATPSCharacter::GetCurrentWeapon()
{
	return CurrentWeapon;
}

void ATPSCharacter::InitWeapon(FName IdWeaponName, FAdditionalWeaponInfo WeaponAdditionalInfo, int32 NewCurrentIndexWeapon)
{
	//Go on server
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	UTPSGameInstance* myGI = Cast<UTPSGameInstance>(GetGameInstance());
	FWeaponInfo myWeaponInfo;
	if (myGI)
	{
		if (myGI->GetWeaponInfoByName(IdWeaponName, myWeaponInfo))
		{
			if (myWeaponInfo.WeaponClass)
			{
				FVector SpawnLocation = FVector(0);
				FRotator SpawnRotation = FRotator(0);

				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = GetInstigator();

				AWeaponDefault* myWeapon = Cast<AWeaponDefault>(GetWorld()->SpawnActor(myWeaponInfo.WeaponClass, &SpawnLocation, &SpawnRotation, SpawnParams));
				if (myWeapon)
				{
					FAttachmentTransformRules Rule(EAttachmentRule::SnapToTarget, false);
					myWeapon->AttachToComponent(GetMesh(), Rule, FName("WeaponSocketRightHand"));
					CurrentWeapon = myWeapon;	
					
					myWeapon->IdWeaponName = IdWeaponName;
					myWeapon->WeaponSetting = myWeaponInfo;

					
					myWeapon->ReloadTime = myWeaponInfo.ReloadTime;
					myWeapon->UpdateStateWeapon_OnServer(MovementState);

					myWeapon->AdditionalWeaponInfo = WeaponAdditionalInfo;

					CurrentIndexWeapon = NewCurrentIndexWeapon;

					myWeapon->OnWeaponReloadStart.AddDynamic(this, &ATPSCharacter::WeaponReloadStart);
					myWeapon->OnWeaponReloadEnd.AddDynamic(this, &ATPSCharacter::WeaponReloadEnd);

					myWeapon->OnWeaponFireStart.AddDynamic(this, &ATPSCharacter::WeaponFireStart);

					// after switch try reload weapon if needed
					if (CurrentWeapon->GetWeaponRound() <=0 && CurrentWeapon->CheckCanWeaponReload())
						CurrentWeapon->InitReload();

					if(InventoryComponent)
						InventoryComponent->OnWeaponAmmoAviable.Broadcast(myWeapon->WeaponSetting.WeaponType);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ATPSCharacter::InitWeapon - Weapon not found in table -NULL"));
		}
	}
}



void ATPSCharacter::TryReloadWeapon()
{
	if (CharHealthComponent && CharHealthComponent->GetIsAlive() && CurrentWeapon && !CurrentWeapon->WeaponReloading)
	{	
		TryReloadWeapon_OnServer();			
	}
}

void ATPSCharacter::WeaponReloadStart(UAnimMontage* Anim)
{
	WeaponReloadStart_BP(Anim);
}

void ATPSCharacter::WeaponReloadEnd(bool bIsSuccess, int32 AmmoTake)
{
	if (InventoryComponent && CurrentWeapon)
	{
		InventoryComponent->AmmoSlotChangeValue(CurrentWeapon->WeaponSetting.WeaponType, AmmoTake);
		InventoryComponent->SetAdditionalInfoWeapon(CurrentIndexWeapon, CurrentWeapon->AdditionalWeaponInfo);
	}
	WeaponReloadEnd_BP(bIsSuccess);
}

void ATPSCharacter::TrySwitchWeaponToIndexByKeyInput_OnServer_Implementation(int32 ToIndex)
{
	bool bIsSuccess = false;
	if (CurrentWeapon && !CurrentWeapon->WeaponReloading && InventoryComponent->WeaponSlots.IsValidIndex(ToIndex))
	{
		if (CurrentIndexWeapon != ToIndex && InventoryComponent)
		{
			int32 OldIndex = CurrentIndexWeapon;
			FAdditionalWeaponInfo OldInfo;

			if (CurrentWeapon)
			{
				OldInfo = CurrentWeapon->AdditionalWeaponInfo;
				if (CurrentWeapon->WeaponReloading)
					CurrentWeapon->CancelReload();
			}

			bIsSuccess = InventoryComponent->SwitchWeaponByIndex(ToIndex, OldIndex, OldInfo);
		}
	}		
}

void ATPSCharacter::DropCurrentWeapon()
{	
	if (InventoryComponent)
	{		
		InventoryComponent->DropWeapobByIndex_OnServer(CurrentIndexWeapon);
	}	
}

void ATPSCharacter::WeaponReloadStart_BP_Implementation(UAnimMontage* Anim)
{
	// in BP
}

void ATPSCharacter::WeaponReloadEnd_BP_Implementation(bool bIsSuccess)
{
	// in BP
}

void ATPSCharacter::WeaponFireStart(UAnimMontage* Anim)
{
	if(InventoryComponent && CurrentWeapon)
		InventoryComponent->SetAdditionalInfoWeapon(CurrentIndexWeapon, CurrentWeapon->AdditionalWeaponInfo);
	WeaponFireStart_BP(Anim);
}

void ATPSCharacter::WeaponFireStart_BP_Implementation(UAnimMontage* Anim)
{
	// in BP
}

UDecalComponent* ATPSCharacter::GetCursorToWorld()
{
	return CurrentCursor;
}

void ATPSCharacter::TrySwitchNextWeapon()
{
	if (CurrentWeapon && !CurrentWeapon->WeaponReloading && InventoryComponent->WeaponSlots.Num() > 1)
	{
		//We have more then one weapon go switch
		int8 OldIndex = CurrentIndexWeapon;
		FAdditionalWeaponInfo OldInfo;
		if (CurrentWeapon)
		{
			OldInfo = CurrentWeapon->AdditionalWeaponInfo;
			if(CurrentWeapon->WeaponReloading)
				CurrentWeapon->CancelReload();
		}
			
		if (InventoryComponent)
		{			
			if (InventoryComponent->SwitchWeaponToIndexByNextPreviosIndex(CurrentIndexWeapon + 1, OldIndex, OldInfo,true))
				{ }
		}
	}	
}

void ATPSCharacter::TrySwitchPreviosWeapon()
{
	if (CurrentWeapon && !CurrentWeapon->WeaponReloading && InventoryComponent->WeaponSlots.Num() > 1)
	{
		//We have more then one weapon go switch
		int8 OldIndex = CurrentIndexWeapon;
		FAdditionalWeaponInfo OldInfo;
		if (CurrentWeapon)
		{
			OldInfo = CurrentWeapon->AdditionalWeaponInfo;
			if (CurrentWeapon->WeaponReloading)
				CurrentWeapon->CancelReload();
		}

		if (InventoryComponent)
		{
			//InventoryComponent->SetAdditionalInfoWeapon(OldIndex, GetCurrentWeapon()->AdditionalWeaponInfo);
			if (InventoryComponent->SwitchWeaponToIndexByNextPreviosIndex(CurrentIndexWeapon - 1, OldIndex, OldInfo, false))
			{
			}
		}
	}
}


void ATPSCharacter::TryAbilityEnabled()
{
	if (AbilityEffect)//TODO Cool down
	{
		UTPS_StateEffect* NewEffect = NewObject<UTPS_StateEffect>(this, AbilityEffect);
		if (NewEffect)
		{
			NewEffect->InitObject(this, NAME_None);
		}
	}
}

EPhysicalSurface ATPSCharacter::GetSurfuceType()
{
	EPhysicalSurface Result = EPhysicalSurface::SurfaceType_Default;
	if (CharHealthComponent)
	{
		if (CharHealthComponent->GetCurrentShield() <= 0)
		{
			if (GetMesh())
			{
				UMaterialInterface* myMaterial = GetMesh()->GetMaterial(0);
				if (myMaterial)
				{
					Result = myMaterial->GetPhysicalMaterial()->SurfaceType;
				}
			}
		}
	}
	return Result;
}

TArray<UTPS_StateEffect*> ATPSCharacter::GetAllCurrentEffects()
{
	return Effects;
}

void ATPSCharacter::RemoveEffect_Implementation(UTPS_StateEffect* RemoveEffect)
{
	Effects.Remove(RemoveEffect);

	if (!RemoveEffect->bIsAutoDestroyParticleEffect)
	{
		SwitchEffect(RemoveEffect, false);
		EffectRemove = RemoveEffect;
	}
}

void ATPSCharacter::AddEffect_Implementation(UTPS_StateEffect* newEffect)
{
	Effects.Add(newEffect);

	if (!newEffect->bIsAutoDestroyParticleEffect)
	{
		SwitchEffect(newEffect, true);
		EffectAdd = newEffect;
	}
	else
	{
		if (newEffect->ParticleEffect)
		{
			ExecuteEffectAdded_OnServer(newEffect->ParticleEffect);
		}
	}
}

void ATPSCharacter::CharDead_BP_Implementation()
{
	//BP
}

void ATPSCharacter::SetActorRotationByYaw_OnServer_Implementation(float Yaw)
{
	SetActorRotationByYaw_Multicast(Yaw);
}

void ATPSCharacter::SetActorRotationByYaw_Multicast_Implementation(float Yaw)
{
	if (Controller && !Controller->IsLocalPlayerController())
	{
		SetActorRotation(FQuat(FRotator(0.0f,Yaw,0.0f)));
	}
}

void ATPSCharacter::SetMovementState_OnServer_Implementation(EMovementState NewState)
{
	SetMovementState_Multicast(NewState);
}

void ATPSCharacter::SetMovementState_Multicast_Implementation(EMovementState NewState)
{
	MovementState = NewState;
	CharacterUpdate();
}

void ATPSCharacter::TryReloadWeapon_OnServer_Implementation()
{
	if (CurrentWeapon->GetWeaponRound() < CurrentWeapon->WeaponSetting.MaxRound && CurrentWeapon->CheckCanWeaponReload())
		CurrentWeapon->InitReload();
}

void ATPSCharacter::PlayAnim_Multicast_Implementation(UAnimMontage* Anim)
{
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(Anim);
	}
}

void ATPSCharacter::EffectAdd_OnRep()
{
	if (EffectAdd)
	{
		SwitchEffect(EffectAdd, true);
	}	
}

void ATPSCharacter::EffectRemove_OnRep()
{
	if (EffectRemove)
	{
		SwitchEffect(EffectRemove, false);
	}
}

void ATPSCharacter::ExecuteEffectAdded_OnServer_Implementation(UParticleSystem* ExecuteFX)
{
	ExecuteEffectAdded_Multicast(ExecuteFX);
}

void ATPSCharacter::ExecuteEffectAdded_Multicast_Implementation(UParticleSystem* ExecuteFX)
{
	UTypes::ExecuteEffectAdded(ExecuteFX, this, FVector(0), FName("Spine_01"));
}

void ATPSCharacter::SwitchEffect(UTPS_StateEffect* Effect, bool bIsAdd)
{
	if (bIsAdd)
	{
		if (Effect && Effect->ParticleEffect)
		{
			FName NameBoneToAttached = Effect->NameBone;
			FVector Loc = FVector(0);

			USkeletalMeshComponent* myMesh = GetMesh();
			if (myMesh)
			{
				UParticleSystemComponent* newParticleSystem = UGameplayStatics::SpawnEmitterAttached(Effect->ParticleEffect, myMesh, NameBoneToAttached, Loc, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false);
				ParticleSystemEffects.Add(newParticleSystem);											
			}
		}
	}
	else
	{		
		if (Effect && Effect->ParticleEffect)
		{
			int32 i = 0;
			bool bIsFind = false;
			if (ParticleSystemEffects.Num() > 0)
			{
				while (i < ParticleSystemEffects.Num() && !bIsFind)
				{
					if (ParticleSystemEffects[i] && ParticleSystemEffects[i]->Template && Effect->ParticleEffect && Effect->ParticleEffect == ParticleSystemEffects[i]->Template)
					{
						bIsFind = true;
						ParticleSystemEffects[i]->DeactivateSystem();
						ParticleSystemEffects[i]->DestroyComponent();
						ParticleSystemEffects.RemoveAt(i);
					}
					i++;
				}
			}			
		}
	}
}

void ATPSCharacter::CharDead()
{
	CharDead_BP();
	if (HasAuthority())
	{
		float TimeAnim = 0.0f;
		int32 rnd = FMath::RandHelper(DeadsAnim.Num());
		if (DeadsAnim.IsValidIndex(rnd) && DeadsAnim[rnd] && GetMesh() && GetMesh()->GetAnimInstance())
		{
			TimeAnim = DeadsAnim[rnd]->GetPlayLength();
			//GetMesh()->GetAnimInstance()->Montage_Play(DeadsAnim[rnd]);
			PlayAnim_Multicast(DeadsAnim[rnd]);
		}

		if (GetController())
		{
			GetController()->UnPossess();
		}

		float DecraeseAnimTimer = FMath::FRandRange(0.2f, 1.0f);

		GetWorldTimerManager().SetTimer(TimerHandle_RagDollTimer, this, &ATPSCharacter::EnableRagdoll_Multicast, TimeAnim - DecraeseAnimTimer, false);

		SetLifeSpan(20.0f);
		if (GetCurrentWeapon())
		{
			GetCurrentWeapon()->SetLifeSpan(20.0f);
		}
	}
	else
	{
		if (GetCursorToWorld())
		{
			GetCursorToWorld()->SetVisibility(false);
		}

		AttackCharEvent(false);
	}

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

void ATPSCharacter::EnableRagdoll_Multicast_Implementation()
{
	if (GetMesh())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Ignore);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_PhysicsBody, ECollisionResponse::ECR_Ignore);

		GetMesh()->SetCollisionObjectType(ECC_PhysicsBody);
		GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Block);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		GetMesh()->SetSimulatePhysics(true);
	}	
}

float ATPSCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (CharHealthComponent && CharHealthComponent->GetIsAlive())
	{
		CharHealthComponent->ChangeHealthValue_OnServer(-DamageAmount);
	}
			
	if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		AProjectileDefault* myProjectile = Cast<AProjectileDefault>(DamageCauser);
		if (myProjectile)
		{
			UTypes::AddEffectBySurfaceType(this, NAME_None, myProjectile->ProjectileSetting.Effect, GetSurfuceType());// to do Name_None - bone for radial damage			
		}
	}

	return ActualDamage;
}

bool ATPSCharacter::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool Wrote = Super::ReplicateSubobjects(Channel,Bunch,RepFlags);

	for (int32 i = 0; i < Effects.Num(); i++)
	{
		if (Effects[i]) {Wrote |= Channel->ReplicateSubobject(Effects[i], *Bunch, *RepFlags);}
	}
	return Wrote;
}


void ATPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATPSCharacter, MovementState);
	DOREPLIFETIME(ATPSCharacter, CurrentWeapon);
	DOREPLIFETIME(ATPSCharacter, CurrentIndexWeapon);
	DOREPLIFETIME(ATPSCharacter, Effects);
	DOREPLIFETIME(ATPSCharacter, EffectAdd);
	DOREPLIFETIME(ATPSCharacter, EffectRemove);
}
