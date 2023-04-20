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
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Enemies/GrabableEnemy.h"
#include "Math/UnrealMathUtility.h"
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>
#include "Blueprint/UserWidget.h"
#include "RCTGameModeBase.h"
#include <Objects/GrabableObject.h>
#include <SlimeKnightGameInstance.h>
#include <SlimeKnightSaveGame.h>
#include "Objects/GrabableLoreObject.h"

#include "AIController.h"
#include "ArmSplineComponent.h"
#include "BrainComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/BlockingVolume.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
ARCTCharacter::ARCTCharacter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	handTarget = CreateDefaultSubobject<USkeleArmComponent>(TEXT("ArmComponent"));
	realHand = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandComponent"));
	holdPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HoldPoint"));
	cameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	isoCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("IsoCamera"));
	handHolder = CreateDefaultSubobject<USceneComponent>(TEXT("HandHolder"));
	handCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HandCollision"));
	punchCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("PunchCollision"));
	grabRangeCollision = CreateDefaultSubobject<USphereComponent>(TEXT("GrabRangeSphere"));
	
	handTarget->SetupAttachment(RootComponent);
	handHolder->SetupAttachment(RootComponent);
	realHand->SetupAttachment(handHolder);
	holdPoint->SetupAttachment(realHand);
	handCollision->SetupAttachment(realHand);
	punchCollision->SetupAttachment(realHand);
	grabRangeCollision->SetupAttachment(RootComponent);
	grabRangeCollision->SetCollisionProfileName("ArmRangeCollision");
	
	
	cameraBoom->SetupAttachment(RootComponent);
	cameraBoom->bUsePawnControlRotation = false;
	cameraBoom->TargetArmLength = 2000.f;
	cameraBoom->SetRelativeRotation(FRotator(-30,-45,0));
	
	isoCamera->SetupAttachment(cameraBoom, USpringArmComponent::SocketName);
	isoCamera->bUsePawnControlRotation = false;

	armSplineComp = CreateDefaultSubobject<UArmSplineComponent>(TEXT("ArmSplineComponent"));
	armSplineComp->SetupAttachment(RootComponent);

	grabRangeCollision->SetSphereRadius(maxArmLength + grabRangeExtension);
	originalMaxArmLength = maxArmLength;

	ability = nullptr;
	grabbedActor = nullptr;
}

// Called when the game starts or when spawned
void ARCTCharacter::BeginPlay()
{
	Super::BeginPlay();
	OnTakeAnyDamage.AddDynamic(this, &ARCTCharacter::PlayerDamage);
}

void ARCTCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	grabRangeCollision->OnComponentBeginOverlap.AddDynamic(this, &ARCTCharacter::OnActorEnterArmRange);
	grabRangeCollision->OnComponentEndOverlap.AddDynamic(this, &ARCTCharacter::OnActorExitArmRange);

	handTarget->OnComponentHit.AddDynamic(this, &ARCTCharacter::OnHandTargetHit);
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

	if(!grabbing)
	{
		UpdateGrabTarget();
	}

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

bool ARCTCharacter::CanDamage(UPrimitiveComponent* hitComponent)
{
	if(Cast<UArmSplineComponent>(hitComponent))
	{
		return false;
	}
	// a bit hacky, we are counting on the capsule collision for the body
	if(Cast<UBoxComponent>(hitComponent))
	{
		return false;
	}
	return true;
}

void ARCTCharacter::UpdateGrabTarget()
{
	float currentMinAngle = 361.0f;// max Angle
	if(grabTarget)
	{
		if(grabTarget->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()))
		{
			IGrabableInterface::Execute_SetHilighting(grabTarget, false);
		}
		grabTarget = nullptr;
	}

	auto findClosestActorInRange = [&] (TSet<AActor*>& actorsInRange)
	{
		for (AActor* actor : actorsInRange)
		{
			if (actor->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()) && IGrabableInterface::Execute_CanBeGrabbed(actor))
			{
				float gazeYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), GetHandTargetLocation()).Yaw;
				float enemyYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), actor->GetActorLocation()).Yaw;
				float angleDistance = abs(gazeYaw - enemyYaw);

				if (IsValid(actor) && angleDistance < grabAngleTolerance && angleDistance < currentMinAngle)
				{
					currentMinAngle = angleDistance;
					grabTarget = actor;
				}
			}
		}
	};

	findClosestActorInRange(grabableEnemiesInRange);
	if(grabTarget == nullptr)
	{
		// no luck with the enemies, try with object
		findClosestActorInRange(grabableObjectsInRange);
	}

	if(grabTarget && grabTarget->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()))
	{
		IGrabableInterface::Execute_SetHilighting(grabTarget, true);
	}
}

#pragma region Events and prototypes

void ARCTCharacter::OnPromptDevour_Implementation() {}
void ARCTCharacter::OnEndPromptDevour_Implementation() {}
void ARCTCharacter::OnLetGo_Implementation() {}
void ARCTCharacter::OnGrab_Implementation() {}

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

#pragma endregion

#pragma region Health & Stamina

void ARCTCharacter::PlayerDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (curInvincibilityDuration <= 0) {
		health -= Damage;
		OnHealthChange(-Damage);

		curInvincibilityDuration = hitInvincibilityDuration;
	}

	if (health <= 0 && !bIsDead) 
	{
		HandleDeathCase();
	}
}

void ARCTCharacter::AbsorbAfterDamaging(float damageDealt) 
{
	health += damageDealt * DevourHPStealPercent / 100.f;
	health = FMath::Clamp(health, 0, maxHealth);
	OnHealthChange(damageDealt * DevourHPStealPercent / 100.f);
}

void ARCTCharacter::OnHealthChange_Implementation(float modifier) {};

#pragma endregion

#pragma region Abilities & Devour
void ARCTCharacter::PromptForDevour() {
	if (grabbedActor == nullptr)
		return;

	readyToDevour = true;
	OnPromptDevour();
}

void ARCTCharacter::EndPromptForDevour() {
	readyToDevour = false;
	OnEndPromptDevour();
}

void ARCTCharacter::Devour()
{
	if (!readyToDevour || grabbedActor == nullptr) {
		OnEndPromptDevour();
		return;
	}

	FString grabbedName = grabbedActor->GetClass()->GetName();

	AGrabableObject* go = Cast<AGrabableObject>(grabbedActor);

	playerDevourEvent.Broadcast();

	if (go) {
		AGrabableLoreObject* glo = Cast<AGrabableLoreObject>(go);
		if (glo)
		{
			grabbedName = glo->GetLoreKey();
			loreUpdateWidgetEvent.Broadcast();
		}
		grabbedActor->Destroy();
		grabbedActor = nullptr;
	}

	else {
		armSplineComp->StopDevourBulgeTimeline();

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

		UGameplayStatics::ApplyDamage(grabbedActor, 10, GetController(), this, UDamageType::StaticClass());
		//grabbedActor->Destroy();
		grabbedActor = nullptr;
	}

	USlimeKnightGameInstance* gameInstance = Cast<USlimeKnightGameInstance>(GetGameInstance());
	if (gameInstance) {
		USlimeKnightSaveGame* save = gameInstance->GetSaveGame();

		save->IncrementBestiaryData(grabbedName);
		gameInstance->SaveGame();
	}

	// Make sure that we no longer display the devour prompt
	OnEndPromptDevour();
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

	float moveTime = handMoveTime;
	if(IsValid(grabTarget))
	{
		moveTime *= grabMoveTimeScale;
	}
		
	UKismetSystemLibrary::MoveComponentTo(handHolder, handTarget->GetRelativeLocation(), handTarget->GetRelativeRotation(), false, false, moveTime, true, EMoveComponentAction::Move, info);
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
		inputLocation = armRestingLocation;
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

	inputLocation *= maxArmLength;

	if(maxArmLength != originalMaxArmLength)
	{
		//update range sphere
		grabRangeCollision->SetSphereRadius(maxArmLength + grabRangeExtension);
		originalMaxArmLength = maxArmLength;
	}

	if(grabTarget && grabbing)
	{
		if(!IsValid(grabTarget))
		{
			LetGo();
			return;
		}

		handTarget->SetWorldLocation(grabTarget->GetActorLocation());
		if(FVector::Dist(handHolder->GetComponentLocation(), GetHandTargetLocation()) < grabRangeExtension + grabDistTolerance)
		{
			IGrabableInterface::Execute_Grab(grabTarget, this);
			grabbedActor = grabTarget;

			AGrabableEnemy* enemy = Cast<AGrabableEnemy>(grabTarget);
			if(enemy)
			{
				// bulge effect
				armSplineComp->StartDevourBulgeTimeline();
			}

			if(grabTarget->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()))
			{
				IGrabableInterface::Execute_SetHilighting(grabTarget, false);
			}

			grabTarget = nullptr;

			realHand->SetCollisionProfileName(FName("BlockAllDynamic"));
			punchCollision->SetCollisionProfileName(FName("BlockAllDynamic"));
			punchCollision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ARCTCharacter::OnFistBeginOverlap);
		}
	}
	else
	{
		// relative to RCTCharacter
		handTarget->SetRelativeLocation(FVector(0, 0, 0));
		handTarget->SetRelativeLocation(inputLocation, true);

		armSplineComp->UpdateHandInput(FVector2D(inputLocation.X, inputLocation.Y));
	}
}

void ARCTCharacter::OnHandTargetHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if(rightStickInputLocation.Length() < 0.001f)
		return;
	ABlockingVolume* BlockingVolume = Cast<ABlockingVolume>(OtherActor);
	if(BlockingVolume)// && bBFromSweep
	{
		//reposition
		AddMovementInput(Hit.ImpactNormal,0.9f);
	}
}

void ARCTCharacter::AttachToArm(AActor* ActorToAttach, FName SocketName)
{
	ActorToAttach->SetActorLocation(holdPoint->GetComponentLocation());
	// Socket on the holdPoint if appliable
	FAttachmentTransformRules rules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
	ActorToAttach->AttachToComponent(holdPoint, rules, SocketName);
}

void ARCTCharacter::MultiplyHandSize(float scale)
{
	FVector size = realHand->GetRelativeScale3D();
	realHand->SetRelativeScale3D(size * scale);
}

bool pushing = false;
void ARCTCharacter::UpdateArmX(float value)
{
	pushing = true;
	rightStickInputLocation = FVector2D(value, rightStickInputLocation.Y);
}

void ARCTCharacter::UpdateArmY(float value)
{
	pushing = true;
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


	grabbing = true;
	currentStamina -= initialGrabCost;

	if(grabTarget)
	{
		AGrabableEnemy* enemy = Cast<AGrabableEnemy>(grabTarget);
		if(enemy)
		{
			AAIController* controller = Cast<AAIController>(enemy->GetController());
			if(controller)
			{
				// doesn't have to chase him
				controller->GetBrainComponent()->PauseLogic("GrabTargeted");
			}
		}
	}
	else
	{
		// not targeting any enemy feel free to punch
		realHand->SetCollisionProfileName(FName("BlockAllDynamic"));
		punchCollision->SetCollisionProfileName(FName("BlockAllDynamic"));
		punchCollision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ARCTCharacter::OnFistBeginOverlap);
	}
	
	OnGrab();
}

void ARCTCharacter::OnActorEnterArmRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bBFromSweep, const FHitResult& SweepResult)
{
	AGrabableEnemy* grabableEnemy = Cast<AGrabableEnemy>(OtherActor);
	AGrabableObject* grabableObject = Cast<AGrabableObject>(OtherActor);
	if(grabableEnemy)
	{
		// ignore grabable object for now
		grabableEnemiesInRange.Add(OtherActor);
	}
	else if(grabableObject)
	{
		grabableObjectsInRange.Add(OtherActor);
	}
}

void ARCTCharacter::OnActorExitArmRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AGrabableEnemy* grabableEnemy = Cast<AGrabableEnemy>(OtherActor);
	AGrabableObject* grabableObject = Cast<AGrabableObject>(OtherActor);
	if(grabableEnemy)
	{
		// ignore grabable object for now
		grabableEnemiesInRange.Remove(OtherActor);
	}
	else if(grabableObject)
	{
		grabableObjectsInRange.Remove(OtherActor);
	}
}

void ARCTCharacter::LetGo()
{
	OnLetGo();

	readyToDevour = false;
	grabbing = false;

	if(grabTarget)
	{
		AGrabableEnemy* enemy = Cast<AGrabableEnemy>(grabTarget);
		if(enemy)
		{
			AAIController* controller = Cast<AAIController>(enemy->GetController());
			if(controller)
			{
				controller->GetBrainComponent()->ResumeLogic("UnGrabTargeted");
			}
		}
		if(grabTarget->GetClass()->ImplementsInterface(UGrabableInterface::StaticClass()))
		{
			IGrabableInterface::Execute_SetHilighting(grabTarget, false);
		}
		grabTarget = nullptr;
	}

	if (grabbedActor)
	{
		armSplineComp->StopDevourBulgeTimeline();
		IGrabableInterface::Execute_LetGo(grabbedActor, this);

		// make sure we recalculate collision
		armSplineComp->RemoveOverlapRecord(Cast<AEnemyBase>(grabbedActor));
		grabbedActor = nullptr;
	}
	realHand->SetCollisionProfileName(FName("NoCollision"));
	punchCollision->SetCollisionProfileName(FName("NoCollision"));
	punchCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ARCTCharacter::OnFistBeginOverlap);
}

void ARCTCharacter::HandleDeathCase()
{
	bIsDead = true;
	playerDeathEvent.Broadcast();
}

void ARCTCharacter::OnFistBeginOverlap(UPrimitiveComponent * overlappedComp, AActor * otherActor, UPrimitiveComponent * otherComp
                                       , int32 otherBodyIndex, bool bFromSweep, const FHitResult & sweepResult)
{
	auto enemy = Cast<AGrabableEnemy>(otherActor);
	if (enemy != nullptr)
	{
		enemy->HitByFist();
	}
}

#pragma endregion