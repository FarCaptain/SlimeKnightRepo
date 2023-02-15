// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter/SkeleArmComponent.h"
#include "Kismet/KismetMathLibrary.h"

void USkeleArmComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RotateArm();
}

void USkeleArmComponent::RotateArm()
{
	AActor* owner = GetOwner();
	FRotator HandRot = UKismetMathLibrary::FindLookAtRotation(this->GetRelativeLocation(), FVector::Zero());
	SetRelativeRotation(HandRot);
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Rotator: %f %f %f"), HandRot.Pitch, HandRot.Yaw, HandRot.Roll));
}