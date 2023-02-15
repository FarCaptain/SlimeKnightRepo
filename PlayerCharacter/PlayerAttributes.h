// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerAttributes.generated.h"

UCLASS(  Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RCT_API UPlayerAttributes : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerAttributes();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "General")
	int playerLevel;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	float attackModifier;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	int attackPower;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attack")
	float armOverlapDamagePercentage;

	UFUNCTION(BlueprintNativeEvent)
	void OnPlayerLevelChanged();

	UFUNCTION(BlueprintNativeEvent)
	void OnAttackModifierChanged();

	UFUNCTION(BlueprintCallable)
	void SetAttackPower(int power);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SetPlayerLevel(int level);

	UFUNCTION(BlueprintCallable)
	int GetPlayerLevel();

	UFUNCTION(BlueprintCallable)
	void SetAttackModifier(float mod);

	UFUNCTION(BlueprintCallable)
	float GetAttackModifier();
};
