// Fill out your copyright notice in the Description page of Project Settings.

#include "SCharacter.h"
#include "CoopGame.h"
#include "SWeapon.h"
#include "CCEngineUtility.h"
#include "Components/SHealthComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Camera/CameraComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"

#include "Engine/World.h"

#include "Net/UnrealNetwork.h"


// Sets default values
ASCharacter::ASCharacter() {
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Add components
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	
	// Disable weapon collision blocking on capsule
	GetCapsuleComponent()->SetCollisionResponseToChannel(CC_COLLISION_WEAPON, ECR_Ignore);

	// Enable crouching.
	auto navProperties = GetMovementComponent()->GetNavAgentPropertiesRef();
	navProperties.bCanCrouch = true;
	navProperties.bCanJump = true;

	// Set the default variables
	WeaponAttachSocketName = "Socket_Weapon";

	ZoomInterpSpeed = 20;
	FOVZoomed = 65.0;

	bDied = false;
}

// Called when the game starts or when spawned
void ASCharacter::BeginPlay() {
	Super::BeginPlay();

	// Set the Default FOV.
	FOVDefault = CameraComp->FieldOfView;

	// If our zoomed FOV is not set, Set it to default - 30.
	if (FOVZoomed == 0)
		FOVZoomed = FOVDefault - 30;

	if (GetLocalRole() == ROLE_Authority) {
		// Spawn a default weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = static_cast<ASWeapon*>(GetWorld()->SpawnActor<AActor>(StarterWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams));
		if (CurrentWeapon) {
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName);
		}
	}

	// Attach our OnHealthChanged event
	HealthComp->OnHealthChanged.AddDynamic(this, &ASCharacter::OnHealthChanged);
}

void ASCharacter::InputMoveForward(float Value) {
	AddMovementInput(GetActorForwardVector() * Value);
}

void ASCharacter::InputMoveRight(float Value) {
	AddMovementInput(GetActorRightVector() * Value);
}

void ASCharacter::InputBeginCrouch() {
	Crouch();
}

void ASCharacter::InputEndCrouch() {
	UnCrouch();
}

void ASCharacter::InputBeginZoom() {
	bWantsToZoom = true;
}

void ASCharacter::InputEndZoom() {
	bWantsToZoom = false;
}

void ASCharacter::InputStartFire() {
	if (CurrentWeapon)
		CurrentWeapon->StartFire();
}

void ASCharacter::InputEndFire() {
	if (CurrentWeapon)
		CurrentWeapon->StopFire();
}

// Called every frame
void ASCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	const float TargetFOV = bWantsToZoom ? FOVZoomed : FOVDefault;
	const float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);

	CameraComp->SetFieldOfView(NewFOV);
}

// Called to bind functionality to input
void ASCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASCharacter::InputMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASCharacter::InputMoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ASCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookRight", this, &ASCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASCharacter::InputBeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASCharacter::InputEndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);

	PlayerInputComponent->BindAction("Zoom", IE_Pressed, this, &ASCharacter::InputBeginZoom);
	PlayerInputComponent->BindAction("Zoom", IE_Released, this, &ASCharacter::InputEndZoom);

	PlayerInputComponent->BindAction("Fire", IE_Pressed,  this, &ASCharacter::InputStartFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ASCharacter::InputEndFire);
}

FVector ASCharacter::GetPawnViewLocation() const {
	if (CameraComp)
		return CameraComp->GetComponentLocation();

	return Super::GetPawnViewLocation();
}

void ASCharacter::OnHealthChanged(USHealthComponent* HealthComponent, float Health, float HealthDelta,
		const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser) {
	//UE_LOG(LogTemp, Warning, TEXT("ASCharacter::OnHealthChanged(%f, %f) -> bDied = %d"), Health, HealthDelta, bDied);
	//CCEngineUtility::AddOnScreenDebugMessage(-1, 0.1f, FColor::Red,
	//	FString::Printf(TEXT("ASCharacter::OnHealthChanged(%f, %f) -> bDied = %d"), Health, HealthDelta, bDied));
	
	// If the player is dead.
	if (!bDied && Health <= 0.0f) {
		bDied = true;
		
		// Stop all movement.
		GetMovementComponent()->StopMovementImmediately();

		// Disable all collisions.
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Detach the camera after death.
		DetachFromControllerPendingDestroy();

		// Destroy the pawn after 10 seconds.
		SetLifeSpan(10.0f);

		// Invoke the death event
		OnCharacterDied(DamageType, InstigatedBy, DamageCauser);
		this->OnCharacterDiedEvent.Broadcast(this, DamageType, InstigatedBy, DamageCauser);
	}
}

void ASCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASCharacter, CurrentWeapon);
	DOREPLIFETIME(ASCharacter, bDied);
}