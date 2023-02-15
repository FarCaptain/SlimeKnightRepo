// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interfaces/GrabableInterface.h"
#include "Enemies/EnemyBase.h"

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GrabableEnemy.generated.h"

UCLASS(Blueprintable, Category="RCT Enemies")
class RCT_API AGrabableEnemy : public AEnemyBase, public IGrabableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGrabableEnemy();

	void Grab_Implementation(ARCTCharacter* character) override;
	void LetGo_Implementation(ARCTCharacter* character) override;
	bool CanBeGrabbed_Implementation() override;

	virtual float TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite)
	ARCTCharacter* grabbingCharacter = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool collisionDisabledOnGrab;

	UPROPERTY(BlueprintReadWrite)
	float throwTimeToExplode = 0.5f;

	UFUNCTION()
	void Explode();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
