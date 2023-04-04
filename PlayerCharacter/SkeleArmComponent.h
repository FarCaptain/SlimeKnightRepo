// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkeleArmComponent.generated.h"

/**
 * 
 */
UCLASS()
class RCT_API USkeleArmComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void RotateArm();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float PitchMod;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float YawMod;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float RollMod;
};
