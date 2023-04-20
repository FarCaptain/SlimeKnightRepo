// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/EnemyBase.h"
#include "Enemies/EnemyStatData.h"
#include "Engine/DataTable.h"
#include "Objects/LevelTransitionVolumeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>

#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AEnemyBase::AEnemyBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ConstructorHelpers::FObjectFinder<UDataTable> enemyStatsObject(TEXT("DataTable'/Game/DataFiles/EnemyDataTable.EnemyDataTable'"));
	if (enemyStatsObject.Succeeded())
	{
		enemyStatsTable = enemyStatsObject.Object;
	}
}

// Called when the game starts or when spawned
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	SetupStats();

	auto alevelTransitionVolume = UGameplayStatics::GetActorOfClass(GetWorld(), ALevelTransitionVolumeBase::StaticClass());
	auto levelTransitionVolume = Cast<ALevelTransitionVolumeBase>(alevelTransitionVolume);

	if (levelTransitionVolume) 
	{
		levelTransitionVolume -> enemyCount++;
		// deathEvent.AddUObject(levelTransitionVolume, &ALevelTransitionVolumeBase::EnemyDied);
		deathEvent.AddDynamic(levelTransitionVolume, &ALevelTransitionVolumeBase::EnemyDied);
	}

	health = maxHealth;
}

// Called every frame
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AEnemyBase::IsDead()
{
	return bIsDead;
}

float AEnemyBase::TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser)
{
	ModifyHealth(-damageAmount);
	
	if (health == 0 && !bIsDead)
	{
		bIsDead = true;
		deathEvent.Broadcast();

		if(ensure(GetController()))
		{
			AAIController* AIC = Cast<AAIController>(GetController());
			if(ensure(AIC))
			{
				AIC->GetBrainComponent()->StopLogic("EnemyDied");
			}
		}

		//RootComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2);

		UCapsuleComponent* caps = GetCapsuleComponent();
		// caps->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Ignore);
		caps->SetCollisionProfileName("Ragdoll");

		GetWorldTimerManager().SetTimer(deathTimerHandle, this, &AEnemyBase::DelayedDestroy, deathTimeBeforeRespawn, false);
	}
	return Super::TakeDamage(damageAmount, damageEvent, eventInstigator, damageCauser);
}

// Called to bind functionality to input
void AEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyBase::SetupStats()
{
	FString contextString = FString(TEXT("Enemy Stats Init"));
	FEnemyStatData* enemyStatData = enemyStatsTable->FindRow<FEnemyStatData>(enemyName, contextString);
	if (enemyStatData == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to find enemy stats for %s"), *GetName());
		return;
	}
	/*maxHealth = enemyStatData->HP;
	damage = enemyStatData->Attack;*/
	attackSpeed = enemyStatData->AttackCooldown;
	pursueRadius = enemyStatData->PursuitRadius;
	attackRange = enemyStatData->AttackRange;
	bIsGrabbable = enemyStatData->IsGrabbable;

	auto moveComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());

	if (moveComponent)
	{
		moveComponent->MaxWalkSpeed = enemyStatData->Speed;
	}

	// TODO figure out how to determine static or skeletal (or choose one)
	
	/*auto mesh = GetMesh();
	if (mesh)
	{
		FBodyInstance* bodyInst = mesh->GetBodyInstance();
		bodyInst->MassScale = enemyStatData->Mass;
		bodyInst->UpdateMassProperties();
	}*/
}

void AEnemyBase::ApplyArmStayDamage()
{
	// ModifyHealth(armStayDamage);
	AActor* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	TakeDamage(armStayDamage, FPointDamageEvent(), Player->GetInstigatorController(), Player);
}

float AEnemyBase::GetMaxHealth() const
{
	return maxHealth;
}

float AEnemyBase::GetHealth() const
{
	return health;
}

float AEnemyBase::GetPursueRadius() const
{
	return pursueRadius;
}

FGameplayTagContainer AEnemyBase::GetAbilityTags() const
{
	FGameplayTag tag = FGameplayTag::RequestGameplayTag(FName(TEXT("PlayerAbility")), true);
	FGameplayTagContainer cont(tag);

	FGameplayTagContainer abilities = gameplayTags.Filter(cont);

	return abilities;
}

void AEnemyBase::ModifyHealth(float modifier)
{
	health += modifier;
	//DrawDebugString(GetWorld(), GetTransform().GetLocation(), FString::Printf(TEXT("Damage: %.3f"), -modifier), nullptr, FColor::Red, 2.0f, false);
	health = FMath::Clamp(health, 0, maxHealth);
	if (health == 0)
	{
		hpZeroEvent.Broadcast();
		hpZeroEvent.Clear();
	}
}

void AEnemyBase::Stun_Implementation()
{}

void AEnemyBase::Attack_Implementation(float DeltaTime, AActor* target)
{}

bool AEnemyBase::AttackHasFinished() const
{
	return bAttackFinished;
}

void AEnemyBase::FinishAttack()
{
	bAttackFinished = true;
}

void AEnemyBase::ResetAttack()
{
	bAttackFinished = false;
}

void AEnemyBase::TakePeriodicDamage(float amount, float timeInterval)
{
	armStayDamage = amount;
	GetWorldTimerManager().SetTimer(periodicDamageHandle, this, &AEnemyBase::ApplyArmStayDamage, timeInterval, true, timeInterval);
}

void AEnemyBase::StopPeriodicDamage()
{
		GetWorldTimerManager().ClearTimer(periodicDamageHandle);
}

void AEnemyBase::DelayedDestroy() {
	Destroy();
}