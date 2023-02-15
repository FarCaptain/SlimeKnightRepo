// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter/RCTCharacter.h"
#include "RCT/PlayerCharacter/SkeleArmComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "RCT/Enemies/GrabableEnemy.h"
#include "Engine/EngineTypes.h"
#include "Interfaces/GrabableInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "GameplayTagContainer.h"
#include "Enemies/GrabableEnemy.h"
#include "Math/UnrealMathUtility.h"
#include "RCTGameModeBase.h"
#include <Objects/GrabableObject.h>
#include <SlimeKnightGameInstance.h>
#include <SlimeKnightSaveGame.h>


// Sets default values
ARCTCharacter::ARCTCharacter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	handTarget = CreateDefaultSubobject<USkeleArmComponent>(TEXT("ArmComponent"));
	realHand = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandComponent"));
	cameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	isoCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("IsoCamera"));
	handHolder = CreateDefaultSubobject<USceneComponent>(TEXT("HandHolder"));
	handCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HandCollision"));

	handTarget->SetupAttachment(RootComponent);
	handHolder->SetupAttachment(RootComponent);
	realHand->SetupAttachment(handHolder);
	handCollision->SetupAttachment(realHand);
	
	cameraBoom->SetupAttachment(RootComponent);
	cameraBoom->bUsePawnControlRotation = false;
	cameraBoom->TargetArmLength = 2000.f;
	cameraBoom->SetRelativeRotation(FRotator(-30,-45,0));
	
	isoCamera->SetupAttachment(cameraBoom, USpringArmComponent::SocketName);
	isoCamera->bUsePawnControlRotation = false;

	ability = nullptr;
	grabbedActor = nullptr;
}

// Called when the game starts or when spawned
void ARCTCharacter::BeginPlay()
{
	Super::BeginPlay();

	OnTakeAnyDamage.AddDynamic(this, &ARCTCharacter::PlayerDamage);
}

// Called every frame
void ARCTCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
		
	if (ability)
	{
		ability->OnUpdate(DeltaTime);
	}

	curInvincibilityDuration -= DeltaTime;


	// Grab stamina cost
	if (grabbing) {
		currentStamina -= periodicStaminaCost * DeltaTime;

		if (currentStamina <= 0)
			LetGo();
	}
	// Replenish stamina if not grabbing
	else { 
		currentStamina += staminaRechargePerSecond * DeltaTime;
		currentStamina = (currentStamina >= maximumStamina) ? maximumStamina : currentStamina; // Clamp to maximum
	}

	UpdateHandTargetLocation();

	if (bShouldAutomateHand)
	{
		AutomateHand();
	}
}


#pragma region Events and prototypes

float ARCTCharacter::GetDevourDamage() const
{
	return devourDamage;
}

float ARCTCharacter::GetDevourDamageInterval() const
{
	return devourDamageInterval;
}

float ARCTCharacter::GetThrowKnockbackImpulse() const
{
	return throwKnockbackImpulse;
}

float ARCTCharacter::GetThrowTimeToExplode() const
{
	return throwTimeToExplode;
}

float ARCTCharacter::GetThrowExplosionRadius() const
{
	return throwExplosionRadius;
}

float ARCTCharacter::GetThrowExplosionDamage() const
{
	return throwExplosionDamage;
}

UNiagaraSystem* ARCTCharacter::GetThrowExplosionParticleEffect() const
{
	return throwExplosionParticleEffect;
}

ARCTCharacter::FPromptDevourEvent& ARCTCharacter::OnPromptDevour() {
	return promptDevourEvent;
}

ARCTCharacter::FEndPromptDevourEvent& ARCTCharacter::OnEndPromptDevour() {
	return endPromptDevourEvent;
}

ARCTCharacter::FPlayerGrabEvent& ARCTCharacter::OnGrab()
{
	return playerGrabEvent;
}

ARCTCharacter::FPlayerLetGoEvent& ARCTCharacter::OnLetGo()
{
	return playerLetGoEvent;
}
#pragma endregion

#pragma region Health & Stamina

void ARCTCharacter::PlayerDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (curInvincibilityDuration <= 0) {
		health -= Damage;
		OnHealthChange(-Damage);

		curInvincibilityDuration = hitInvincibilityDuration;
	}
}

void ARCTCharacter::AbsorbAfterDamaging(float damageDealt) {
	health += damageDealt * DevourHPStealPercent/100.f;
	OnHealthChange(damageDealt * DevourHPStealPercent / 100.f);
}

void ARCTCharacter::OnHealthChange_Implementation(float modifier) {};

#pragma endregion

#pragma region Abilities & Devour
void ARCTCharacter::PromptForDevour() {
	readyToDevour = true;
	promptDevourEvent.Broadcast();
}

void ARCTCharacter::EndPromptForDevour() {
	readyToDevour = false;
	endPromptDevourEvent.Broadcast();
}

void ARCTCharacter::Devour()
{
	if (!readyToDevour)
		return;

	FString grabbedName = grabbedActor->GetClass()->GetName();

	AGrabableObject* go = Cast<AGrabableObject>(grabbedActor);

	if (go) {
		grabbedActor->Destroy();
		grabbedActor = nullptr;
	}

	else {
		FGameplayTagContainer abilities = Cast<AEnemyBase>(grabbedActor)->GetAbilityTags();

		// If they have an ability, give the player a random one (or their only one)
		if (abilities.Num() > 0) {
			int randInd = FMath::RandRange(0, abilities.Num() - 1);

			FGameplayTag tag = abilities.GetByIndex(randInd);

			auto world = GetWorld();
			if (world != nullptr) {
				auto gamemode = Cast<ARCTGameModeBase>(world->GetAuthGameMode());

				if (gamemode != nullptr) {
					if (!gamemode->PlayerAbilities.Contains(tag)) {
						UE_LOG(LogTemp, Warning, TEXT("The current gamemode does not contain the following tag: %s"), *(tag.GetTagName().ToString()));
						return;
					}

					if (ability)
						ability->OnExpire();

					TSubclassOf<UBaseAbility> currentAbility = gamemode->PlayerAbilities[tag];

					UBaseAbility* acquiredAbility = NewObject<UBaseAbility>(this, currentAbility);
					acquiredAbility->OnObtain(this);

					ability = acquiredAbility;
				}
			}
		}
	}

	USlimeKnightGameInstance* gameInstance = Cast<USlimeKnightGameInstance>(GetGameInstance());
	if (gameInstance) {
		USlimeKnightSaveGame* save = gameInstance->GetSaveGame();

		save->IncrementBestiaryData(grabbedName);
		gameInstance->SaveGame();
	}
}

void ARCTCharacter::AbilityExpire()
{
	ability = nullptr;
}

void ARCTCharacter::SetAbility(TSubclassOf<UBaseAbility> newAbility) {

	UBaseAbility* acquiredAbility = NewObject<UBaseAbility>(this, newAbility);

	if (ability)
		ability->OnExpire();

	acquiredAbility->OnObtain(this);

	ability = acquiredAbility;
}
#pragma endregion

#pragma region Arm
void ARCTCharacter::SetHandAutomation(bool shouldAutomate)
{
	bShouldAutomateHand = shouldAutomate;
}

void ARCTCharacter::AutomateHand()
{
	FLatentActionInfo info;
	info.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo(handHolder, handTarget->GetRelativeLocation(), handTarget->GetRelativeRotation(), false, false, handMoveTime, false, EMoveComponentAction::Move, info);
}

void ARCTCharacter::UpdateHandTargetLocation()
{
	FVector cameraForward = isoCamera->GetForwardVector();
	cameraForward = FVector(cameraForward.X, cameraForward.Y, 0);
	FVector cameraRight = isoCamera->GetRightVector();
	cameraRight = FVector(cameraRight.X, cameraRight.Y, 0);

	FVector yLocation = cameraForward * rightStickInputLocation.Y;
	FVector xLocation = cameraRight * -(rightStickInputLocation.X);
	FVector inputLocation = yLocation + xLocation;
	
	if (inputLocation.Length() < armDeactivateThreshold)
	{
		inputLocation = armDefaultLocation;
	}
	else
	{
		if (inputLocation.Length() > 1)
		{
			inputLocation.Normalize();
		}
		if (inputLocation.Length() < armClosenessThreshold)
		{
			inputLocation.Normalize();
			inputLocation *= armMinimumDistance;
		}
	}
	
	FRotator characterRotation = GetActorRotation();
	inputLocation = characterRotation.RotateVector(inputLocation);
	inputLocation *= maxArmLength;


	handTarget->SetRelativeLocation(FVector(0, 0, 0));
	handTarget->SetRelativeLocation(inputLocation, true);
}

void ARCTCharacter::AttachToArm(AActor* ActorToAttach)
{
	FAttachmentTransformRules rules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, true);
	ActorToAttach->AttachToComponent(handHolder, rules);
}

void ARCTCharacter::MultiplyHandSize(float scale)
{
	FVector size = realHand->GetRelativeScale3D();
	realHand->SetRelativeScale3D(size * scale);
}

void ARCTCharacter::UpdateArmX(float value)
{
	rightStickInputLocation = FVector2D(value, rightStickInputLocation.Y);
}

void ARCTCharacter::UpdateArmY(float value)
{
	rightStickInputLocation = FVector2D(rightStickInputLocation.X, value);
}

void ARCTCharacter::UpdateMoveX(float value)
{
	if ((Controller) && (value != 0.0f))
	{
		FVector rotator = isoCamera->GetRightVector();

		//FVector direction = FRotationMatrix(rotator).GetUnitAxis(EAxis::X);
		AddMovementInput(rotator, value);
	}
}

void ARCTCharacter::UpdateMoveY(float value)
{
	if ((Controller) && (value != 0.0f))
	{
		FVector rotator = isoCamera->GetForwardVector();

		//FVector direction = FRotationMatrix(rotator).GetUnitAxis(EAxis::Y);
		AddMovementInput(rotator, value);
	}
}

void ARCTCharacter::Grab()
{
	if (currentStamina < initialGrabCost)
		return;


	playerGrabEvent.Broadcast();

	grabbing = true;
	currentStamina -= initialGrabCost;

	TSet<AActor*> actors;
	handCollision->GetOverlappingActors(actors);
	for (AActor* actor : actors)
	{
		if (actor->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()))
		{
			if (actor && IGrabableInterface::Execute_CanBeGrabbed(actor))
			{
				IGrabableInterface::Execute_Grab(actor, this);
				grabbedActor = actor;
				break;
			}
		}
	}
	realHand->SetCollisionProfileName(FName("BlockAllDynamic"));
}

void ARCTCharacter::LetGo()
{
	playerLetGoEvent.Broadcast();

	readyToDevour = false;
	grabbing = false;

	if (grabbedActor)
	{
		IGrabableInterface::Execute_LetGo(grabbedActor, this);
		grabbedActor = nullptr;
	}
	realHand->SetCollisionProfileName(FName("NoCollision"));
}

#pragma endregion