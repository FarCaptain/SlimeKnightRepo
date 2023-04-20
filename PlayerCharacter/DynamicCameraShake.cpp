// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicCameraShake.h"

#include "Kismet/GameplayStatics.h"

UDynamicCameraShake::UDynamicCameraShake()
{
	OscillationDuration = 0.17f;
	OscillationBlendInTime = 0.05f;
	OscillationBlendOutTime = 0.15f;

	RotOscillation.Pitch.Amplitude = 0;
	RotOscillation.Yaw.Amplitude = 0;
	RotOscillation.Roll.Amplitude = 0;

	FOVOscillation.Amplitude = 0;

	LocOscillation.X.Amplitude = 1;
	LocOscillation.X.Frequency = 1;

	LocOscillation.Y.Amplitude = 0;
	LocOscillation.Y.Frequency = 10;

	LocOscillation.Z.Amplitude = 0;
	LocOscillation.Z.Frequency = 10;
}

void UDynamicCameraShake::UpdateCamShake(TSubclassOf<UDynamicCameraShake> shakeClass, FVector2D shakeVector)
{
	if (UDynamicCameraShake* CDO = Cast<UDynamicCameraShake>(shakeClass->GetDefaultObject()))
	{
		shakeVector = shakeVector.GetSafeNormal() * 20.0f;
		CDO->LocOscillation.Y.Amplitude = shakeVector.X;
		CDO->LocOscillation.Z.Amplitude = shakeVector.Y;
	}
}