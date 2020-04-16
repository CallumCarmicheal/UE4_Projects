// Fill out your copyright notice in the Description page of Project Settings.


#include "SWeapon.h"

#include "CoopGame.h"

#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "CCEngineUtility.h"

static int32 DebugWeaponDrawing = 0;
FAutoConsoleVariableRef CVAR_DebugWeaponDrawing (
	TEXT(CC_CONSOLE_PREFIX "Debug.WeaponDrawing"),
	DebugWeaponDrawing,
	TEXT("Draw Debug Lines for Weapons"),
	ECVF_Cheat);

// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "Socket_Weapon_Muzzle";
	TracerTargetName = "Target";

	BaseDamage = 20.0f;
	RateOfFire = 600.0f;

	AutomaticFiring = true;
}

void ASWeapon::BeginPlay() {
	Super::BeginPlay();

	fTimeBetweenShots = 60 / RateOfFire;
}


void ASWeapon::Fire() {
	const float HIT_TRACE_LENGTH = 10000.0f;
	
	// Trace the world from the pawn eye's to the cross-hair location
	AActor* pOwner = GetOwner();
	if (pOwner) {
		FVector EyeLocation;
		FRotator EyeRotation;
		pOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();
		FVector TraceEnd = EyeLocation + (ShotDirection * HIT_TRACE_LENGTH);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(pOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bReturnPhysicalMaterial = true;
		QueryParams.bTraceComplex = true;

		// Particle "Target" Parameter
		FVector TracerEndPoint = TraceEnd;
		
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, CC_COLLISION_WEAPON, QueryParams)) {
			// Hit object, Process Damage.
			AActor* HitActor = Hit.GetActor();

			EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			float ActualDamage = BaseDamage;
			if (SurfaceType == CC_SURFACE_FLESHVULNERABLE) 
				ActualDamage *= 4.0f;
			
			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, pOwner->GetInstigatorController(), this, DamageType);

			// Select a particle effect to play
			UParticleSystem* SelectedEffect = nullptr;
			switch(SurfaceType) {
			case CC_SURFACE_FLESHDEFAULT: 
			case CC_SURFACE_FLESHVULNERABLE:
				SelectedEffect = FleshImpactEffect;
				break;
				
			default:
				SelectedEffect = DefaultImpactEffect;
				break;
			}
			
			// Play the effect
			if (SelectedEffect)
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());

			
			TracerEndPoint = Hit.ImpactPoint;
		}

		if (DebugWeaponDrawing > 0)	
			DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::White, false, 1.0f, 0, 1.0f);

		//UE_LOG(LogTemp, Warning, TEXT("Firing, DebugWeaponDrawing == %d"), DebugWeaponDrawing);
		
		// 
		PlayFireEffects(TracerEndPoint);

		fLastFireTime = GetWorld()->TimeSeconds;

		// 
		if (AutomaticFiring == false) {
			GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
		}
	}
}

void ASWeapon::PlayFireEffects(const FVector TracerEndPoint) const {
	if (MuzzleEffect)
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);

	if (TracerEffect) {
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* pTracerComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);
		if (pTracerComponent)
			pTracerComponent->SetVectorParameter(TracerTargetName, TracerEndPoint);
	}

	APawn* pOwner = Cast<APawn>(GetOwner());
	APlayerController* PC = pOwner ? Cast<APlayerController>(pOwner->GetController()) : nullptr;

	if (PC) {
		PC->ClientPlayCameraShake(FireCamShake);
	}
}

void ASWeapon::StartFire() {
	float FirstDelay = FMath::Max(fLastFireTime + fTimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	//CCEngineUtility::AddOnScreenDebugMessage(-1, 1, FColor::Yellow,
	//	FString::Printf(TEXT("SWeapon: {IsAutomatic=%d, LastFireTime=%f, TimeBetweenShots=%f, WorldTimeSeconds=%f, FirstDelay=%f}!"), 
	//		AutomaticFiring, fLastFireTime, fTimeBetweenShots, GetWorld()->TimeSeconds, FirstDelay));
	
	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ASWeapon::Fire, fTimeBetweenShots, true, FirstDelay);
}

void ASWeapon::StopFire() {
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

