// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataTable.h"
#include "CoreMinimal.h"
#include "EnemyStatData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct RCT_API FEnemyStatData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EnemyStats")
	float HP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyStats")
	float Attack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyStats")
	float AttackCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyStats")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyStats")
	bool IsGrabbable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyStats")
	bool IsDevourable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyStats")
	float PursuitRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyStats")
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EnemyStats")
	float Mass;
};
