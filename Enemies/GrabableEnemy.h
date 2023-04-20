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
	UFUNCTION()
	void LetGo_Implementation(ARCTCharacter* character) override;
	bool CanBeGrabbed_Implementation() override;
	bool IsGrabbed_Implementation() override;

	virtual void SetHilighting_Implementation(bool bIfHighlight) override;

	UFUNCTION()
	void HitByFist();

	void ThrownSlide();

	virtual float TakeDamage(float damageAmount, FDamageEvent const& damageEvent, AController* eventInstigator, AActor* damageCauser) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite)
	ARCTCharacter* grabbingCharacter = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool collisionDisabledOnGrab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool punchable = true;

	UPROPERTY(BlueprintReadWrite)
	float throwTimeToExplode = 0.5f;

	UPROPERTY(VisibleAnywhere)
	FTimerHandle thrownSlideHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float stopSlidingVelocityThreshold;

	UPROPERTY()
	FName originalCollisionProfileName;

	UFUNCTION()
	void OverlapExplode(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void Explode();

	virtual void FellOutOfWorld(const UDamageType& dmgType) override;

private:
	bool bExploded;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
