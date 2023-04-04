// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter/PlayerAttributes.h"

// Sets default values for this component's properties
UPlayerAttributes::UPlayerAttributes()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	attackModifier = 1;
	attackPower = 1;
	armOverlapDamagePercentage = 1;
	// ...
}


// Called when the game starts
void UPlayerAttributes::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UPlayerAttributes::SetAttackPower(int power)
{
	attackPower = power;
}

// Called every frame
void UPlayerAttributes::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UPlayerAttributes::OnAttackModifierChanged_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Mod Changed!!"));
}

void UPlayerAttributes::SetAttackModifier(float mod)
{
	attackModifier = mod;
}

float UPlayerAttributes::GetAttackModifier()
{
	return attackModifier;
}

void UPlayerAttributes::OnPlayerLevelChanged_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Lv Changed!!"));
}

