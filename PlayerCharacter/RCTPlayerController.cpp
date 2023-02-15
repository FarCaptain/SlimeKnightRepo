// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter/RCTPlayerController.h"
#include "RCT/PlayerCharacter/RCTCharacter.h"

#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"

#include "Kismet/GameplayStatics.h"

void ARCTPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	myPawn = Cast<ARCTCharacter>(InPawn);
	myPawn->OnDestroyed.AddDynamic(this, &ARCTPlayerController::PossessedPawnDestroyed);
}

void ARCTPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction("Grab", IE_Pressed, this, &ARCTPlayerController::Grab);
	InputComponent->BindAction("Grab", IE_Released, this, &ARCTPlayerController::LetGo);
	InputComponent->BindAction("Devour", IE_Pressed, this, &ARCTPlayerController::Devour);

	InputComponent->BindAxis("XRightStick", this, &ARCTPlayerController::ArmX);
	InputComponent->BindAxis("YRightStick", this, &ARCTPlayerController::ArmY);
	InputComponent->BindAxis("XMovement", this, &ARCTPlayerController::MoveX);
	InputComponent->BindAxis("YMovement", this, &ARCTPlayerController::MoveY);
}

void ARCTPlayerController::ArmX(float value)
{
	myPawn->UpdateArmX(value);
}

void ARCTPlayerController::ArmY(float value)
{
	myPawn->UpdateArmY(value);
}

void ARCTPlayerController::MoveX(float value)
{
	myPawn->UpdateMoveX(value);
}

void ARCTPlayerController::MoveY(float value)
{
	myPawn->UpdateMoveY(value);
}

void ARCTPlayerController::Grab()
{
	myPawn->Grab();
}

void ARCTPlayerController::LetGo()
{
	myPawn->LetGo();
}

void ARCTPlayerController::Devour() {
	myPawn->Devour();
}

void ARCTPlayerController::PossessedPawnDestroyed(AActor* act)
{
	if (bRestartOnDestroy)
	{
		FName currentLevelName(UGameplayStatics::GetCurrentLevelName(GetWorld()));
		UGameplayStatics::OpenLevel(GetWorld(), currentLevelName);
	}
}
