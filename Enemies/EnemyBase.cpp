// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemies/EnemyBase.h"
#include "Enemies/EnemyStatData.h"
#include "Engine/DataTable.h"

#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

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

	health = maxHealth;
}

// Called every frame
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

float AEnemyBase::TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser)
{
	ModifyHealth(-damageAmount);
	
	if (health == 0 && !bIsDead)
	{
		bIsDead = true;
		deathEvent.Broadcast();
		Destroy();
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
#ifdef WITH_EDITOR
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(0, 2.f, FColor::Red
				, TEXT("UNABLE TO FIND Enemy Stats for") + enemyName.ToString());
		}
#endif
		return;
	}
	maxHealth = enemyStatData->HP;
	damage = enemyStatData->Attack;
	attackSpeed = enemyStatData->AttackCooldown;
	pursueRadius = enemyStatData->PursuitRadius;
	attackRange = enemyStatData->AttackRange;
	auto moveComponent = Cast<UFloatingPawnMovement>(GetMovementComponent());

	//TODO Set Movement to work properly for all enemies
	if (moveComponent)
	{
		moveComponent->MaxSpeed = enemyStatData->Speed;
	}

	// TODO figure out how to determine static or skeletal (or choose one)
	auto staticMesh = Cast<UStaticMeshComponent>(GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (staticMesh)
	{
		FBodyInstance* bodyInst = staticMesh->GetBodyInstance();
		if (bodyInst)
		{
			bodyInst->MassScale = enemyStatData->Mass;
			bodyInst->UpdateMassProperties();
		}
	}
}

AEnemyBase::FEnemyDeathEvent& AEnemyBase::OnDeath()
{
	return deathEvent;
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
	health = FMath::Clamp(health, 0, maxHealth);
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
