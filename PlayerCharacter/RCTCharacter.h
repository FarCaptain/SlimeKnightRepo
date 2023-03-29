// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NiagaraSystem.h"
#include "Abilities/BaseAbility.h"
#include "CoreMinimal.h"
#include "ArmSplineComponent.h"
#include "SkeleArmComponent.h"
#include "GameFramework/Character.h"
#include "RCTCharacter.generated.h"

class UGrabableInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerDeathEvent);

UCLASS()
class RCT_API ARCTCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARCTCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	void PlayerDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable)
	void AbsorbAfterDamaging(float damageDealt);

	UFUNCTION(BlueprintNativeEvent)
	void OnPromptDevour();

	UFUNCTION(BlueprintNativeEvent)
	void OnEndPromptDevour();
	
	UFUNCTION(BlueprintNativeEvent)
	void OnGrab();

	UFUNCTION(BlueprintNativeEvent)
	void OnLetGo();

	UFUNCTION(BlueprintNativeEvent)
	void OnHealthChange(float modifier);

	UFUNCTION(BlueprintCallable)
	void PromptForDevour();

	UFUNCTION(BlueprintCallable)
	void EndPromptForDevour();

	UFUNCTION(BlueprintCallable)
	void UpdateArmX(float value);

	UFUNCTION(BlueprintCallable)
	void UpdateArmY(float value);

	UFUNCTION(BlueprintCallable)
	void UpdateMoveX(float value);

	UFUNCTION(BlueprintCallable)
	void UpdateMoveY(float value);

	UFUNCTION(BlueprintCallable)
	void MultiplyHandSize(float scale);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetDevourDamage() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetDevourDamageInterval() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetThrowKnockbackImpulse() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetThrowTimeToExplode() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetThrowExplosionRadius() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetThrowExplosionDamage() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	UNiagaraSystem* GetThrowExplosionParticleEffect() const;

	UFUNCTION(BlueprintCallable)
	void Devour();

	UFUNCTION(BlueprintCallable)
	void AttachToArm(AActor* ActorToAttach);

	UFUNCTION(BlueprintCallable)
	void Grab();

	UFUNCTION(BlueprintCallable)
	void LetGo();

	UFUNCTION(BlueprintCallable)
	void AbilityExpire();

	UFUNCTION(BlueprintCallable)
	void SetAbility(TSubclassOf<UBaseAbility> ability);

	UPROPERTY(BlueprintAssignable)
	FPlayerDeathEvent playerDeathEvent;

	UFUNCTION(BlueprintCallable)
	void HandleDeathCase();
	
	FVector GetRealHandJointLocation()
	{
		return realHand->GetSocketLocation("HandSocket");
	}

	FVector GetHandTargetLocation()
	{
		return handTarget->GetComponentLocation();
	}
	
	FVector GetShoulderJointLocation()
	{
		return GetMesh()->GetSocketLocation("ArmSocket");
	}

	FTimerHandle deathCaseHandle;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeleArmComponent* handTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* realHand;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* holdPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* cameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* isoCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* handHolder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spline")
	UArmSplineComponent* armSplineComp;
	
	UPROPERTY(BlueprintReadOnly)
	FVector2D rightStickInputLocation;

	UPROPERTY(BlueprintReadWrite)
	AActor* grabbedActor;

	UPROPERTY(BlueprintReadOnly)
	UBaseAbility* ability = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* handCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccees = "true"))
	UBoxComponent* punchCollision;


#pragma region Throwing
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throw and Explosion")
	float throwKnockbackImpulse = 50000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throw and Explosion")
	float throwTimeToExplode = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throw and Explosion")
	float throwExplosionRadius = 500;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throw and Explosion")
	float throwExplosionDamage = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Throw and Explosion")
	UNiagaraSystem* throwExplosionParticleEffect;
#pragma endregion

#pragma region Devour
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float devourDamage = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DevourHPStealPercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
		float devourDamageInterval = 1.f;
#pragma endregion

#pragma region Health & Stamina

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Stamina")
	float maxHealth = 35.f;

	UPROPERTY(BlueprintReadWrite, Category = "Health & Stamina")
	float health = 35.f;

	UPROPERTY(BlueprintReadWrite, Category = "Health & Stamina")
	float currentStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Stamina")
	float maximumStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Stamina")
	float periodicStaminaCost = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Stamina")
	float initialGrabCost = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Stamina")
	float staminaRechargePerSecond = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Stamina")
	float hitInvincibilityDuration = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Health & Stamina")
	float percentDamageOnOverlap = 0.05f;

#pragma endregion
#pragma region Arm
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arm", meta = (AllowPrivateAccess = "true"))
	float maxArmLength = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arm", meta = (AllowPrivateAccess = "true"))
	float handMoveTime = .1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arm")
	float armClosenessThreshold = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arm")
	float armDeactivateThreshold = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arm")
	FVector armRestingLocation = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Arm")
	float armMinimumDistance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Arm")
	bool grabbing = false;

#pragma endregion
	UFUNCTION(BlueprintCallable)
	void SetHandAutomation(bool shouldAutomate);

	UFUNCTION(BlueprintCallable)
	void AutomateHand();

	UFUNCTION(BlueprintCallable)
	void UpdateHandTargetLocation();

	bool bShouldAutomateHand = true;

private:
	float curInvincibilityDuration = 0.f;
	bool readyToDevour = false; // Whether or not the enemy in the player's hand is ready to be devoured
	bool bIsDead = false;
	
};
