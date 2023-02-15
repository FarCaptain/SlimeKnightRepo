// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EnemyBase.generated.h"

UCLASS()
class RCT_API AEnemyBase : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AEnemyBase();

	/** Called when player health is <= 0 for the first time */
	DECLARE_EVENT(AEnemyBase, FEnemyDeathEvent)
	FEnemyDeathEvent& OnDeath();

	/** Returns Enemies Max Health */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetMaxHealth() const;

	/** Returns Enemies Current Health */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool AttackHasFinished() const;

	/** Returns the Enemies Pursuit Radius */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetPursueRadius() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Attack(float DeltaTime, AActor* target = nullptr);

	UFUNCTION(BlueprintCallable)
	void ResetAttack();

	/** Returns the Enemies AbilityTags */
	FGameplayTagContainer GetAbilityTags() const;

	/** Adjustst the enemy health by the @modifier variable */
	UFUNCTION(BlueprintCallable)
	virtual void ModifyHealth(float modifier);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Stun();

	virtual float TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser) override;

protected:

	UFUNCTION()
	void SetupStats();

	UFUNCTION(BlueprintCallable)
	void FinishAttack();

	UPROPERTY()
	bool bAttackFinished;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** The Enemies Starting Health and max it can go */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "1.0"))
	float maxHealth;

	UPROPERTY()
	float health;

	/** The Enemies base damage variable */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float damage;

	/** The shortest interval between enemy attacks */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float attackSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float attackRange;

	UPROPERTY()
	bool bIsDead = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="EnemyStats")
	FName enemyName;

	/** The distance the enemy must be from a player in order to begin pursuing them */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float pursueRadius;

	FEnemyDeathEvent deathEvent;

	/** Tags to define behavior and abilities */
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer gameplayTags;

	UPROPERTY(BlueprintReadOnly, Category="EnemyStats")
	class UDataTable* enemyStatsTable;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
