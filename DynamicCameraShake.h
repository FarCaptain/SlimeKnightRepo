// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MatineeCameraShake.h"
#include "DynamicCameraShake.generated.h"

/**
 * 
 */
UCLASS()
class RCT_API UDynamicCameraShake : public UMatineeCameraShake
{
	GENERATED_BODY()

public:
	UDynamicCameraShake();

	// UFUNCTION(BlueprintCallable)
	static void UpdateCamShake(TSubclassOf<UDynamicCameraShake> shakeClass, FVector2D shakeVector);
};
